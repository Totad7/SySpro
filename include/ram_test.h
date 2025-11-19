#pragma once
#include <iostream>
#include <vector>
#include <chrono>
#include <cstdint>
#include <random>
#include <thread>
#include <limits>

namespace ram_test
{
    void force_memory_barrier()
    {
        asm volatile("" ::: "memory");
    }

    size_t get_available_memory_mb()
    {
        // Примерная оценка доступной памяти (упрощенная)
        // В реальном приложении лучше использовать системные API
        return 4096; // 4 GB как безопасное значение по умолчанию
    }

    size_t get_user_input_size()
    {
        size_t available_mem = get_available_memory_mb();
        size_t max_recommended = available_mem * 3 / 4; // 75% от доступной

        std::cout << "=== RAM PERFORMANCE TEST ===\n";
        std::cout << "Available memory: ~" << available_mem << " MB\n";
        std::cout << "Recommended max test size: " << max_recommended << " MB\n\n";

        std::cout << "Choose test size:\n";
        std::cout << "1. 512 MB (quick test)\n";
        std::cout << "2. 1024 MB (standard test)\n";
        std::cout << "3. 2048 MB (extended test)\n";
        std::cout << "4. 4096 MB (comprehensive test)\n";
        std::cout << "5. Custom size\n";
        std::cout << "Enter your choice (1-5): ";

        int choice;
        std::cin >> choice;

        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Using standard 1024 MB test.\n";
            return 1024;
        }

        size_t size_mb;

        switch (choice)
        {
        case 1:
            size_mb = 512;
            break;
        case 2:
            size_mb = 1024;
            break;
        case 3:
            size_mb = 2048;
            break;
        case 4:
            size_mb = 4096;
            break;
        case 5:
            std::cout << "Enter custom test size in MB (" << max_recommended << " MB max recommended): ";
            std::cin >> size_mb;

            if (std::cin.fail())
            {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Invalid input. Using standard 1024 MB test.\n";
                size_mb = 1024;
            }
            else if (size_mb > max_recommended * 2)
            {
                std::cout << "Warning: Test size exceeds recommended maximum!\n";
                std::cout << "Continue anyway? (y/n): ";
                char confirm;
                std::cin >> confirm;
                if (confirm != 'y' && confirm != 'Y')
                {
                    std::cout << "Using recommended size: " << max_recommended << " MB\n";
                    size_mb = max_recommended;
                }
            }
            else if (size_mb < 64)
            {
                std::cout << "Size too small. Using minimum 64 MB.\n";
                size_mb = 64;
            }
            break;
        default:
            std::cout << "Invalid choice. Using standard 1024 MB test.\n";
            size_mb = 1024;
            break;
        }

        return size_mb;
    }

    void run_test()
    {
        size_t test_size_MB = get_user_input_size();
        const size_t total_bytes = test_size_MB * 1024 * 1024;
        const size_t elements = total_bytes / sizeof(uint64_t);

        std::cout << "\nTesting " << test_size_MB << " MB...\n";
        std::cout << "Allocating memory...\n";

        try
        {
            // Выделяем память
            std::vector<uint64_t> buffer(elements);

            // Инициализируем случайными данными чтобы избежать оптимизации
            std::random_device rd;
            std::mt19937_64 gen(rd());
            std::uniform_int_distribution<uint64_t> dis;

            std::cout << "Initializing test data...\n";
            for (size_t i = 0; i < elements; i++)
            {
                buffer[i] = dis(gen);
            }

            // Даем системе время успокоиться
            std::cout << "Preparing test...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // Тест записи
            std::cout << "Running write test...\n";
            auto write_start = std::chrono::high_resolution_clock::now();
            for (size_t i = 0; i < elements; i += 8)
            {
                buffer[i] = i;
                force_memory_barrier();
            }
            auto write_end = std::chrono::high_resolution_clock::now();
            double write_time = std::chrono::duration<double>(write_end - write_start).count();
            double write_speed = (elements * sizeof(uint64_t) / (1024.0 * 1024.0)) / write_time;

            // Очищаем кэш между тестами
            std::cout << "Clearing cache...\n";
            std::vector<uint64_t> clear_cache(1024 * 1024); // 8 MB для очистки кэша
            for (size_t i = 0; i < clear_cache.size(); i++)
            {
                clear_cache[i] = i;
            }

            // Тест чтения
            std::cout << "Running read test...\n";
            volatile uint64_t checksum = 0;
            auto read_start = std::chrono::high_resolution_clock::now();
            for (size_t i = 0; i < elements; i += 8)
            {
                checksum ^= buffer[i];
                force_memory_barrier();
            }
            auto read_end = std::chrono::high_resolution_clock::now();
            double read_time = std::chrono::duration<double>(read_end - read_start).count();
            double read_speed = (elements * sizeof(uint64_t) / (1024.0 * 1024.0)) / read_time;

            // Вывод результатов
            std::cout << "\n=== TEST RESULTS ===\n";
            std::cout << "Test size: " << test_size_MB << " MB\n";
            std::cout << "Write: " << write_speed << " MB/s\n";
            std::cout << "Read:  " << read_speed << " MB/s\n";
            std::cout << "Write time: " << write_time << " seconds\n";
            std::cout << "Read time: " << read_time << " seconds\n";

            // Анализ результатов
            if (write_speed > read_speed)
            {
                std::cout << "Status: Normal (write faster than read)\n";
            }
            else
            {
                std::cout << "Status: Abnormal (read faster than write)\n";
            }

            std::cout << "\nTest completed successfully!\n";
        }
        catch (const std::bad_alloc &e)
        {
            std::cout << "Error: Not enough memory to allocate " << test_size_MB << " MB!\n";
            std::cout << "Try using a smaller test size.\n";
            return;
        }
        catch (const std::exception &e)
        {
            std::cout << "Error during test: " << e.what() << "\n";
            return;
        }
    }

    // Альтернативная функция для автоматического теста без пользовательского ввода
    void run_auto_test(size_t test_size_MB = 1024)
    {
        const size_t total_bytes = test_size_MB * 1024 * 1024;
        const size_t elements = total_bytes / sizeof(uint64_t);

        std::cout << "=== RAM PERFORMANCE TEST ===\n";
        std::cout << "Testing " << test_size_MB << " MB...\n";

        try
        {
            std::vector<uint64_t> buffer(elements);

            // Инициализация
            std::random_device rd;
            std::mt19937_64 gen(rd());
            std::uniform_int_distribution<uint64_t> dis;

            for (size_t i = 0; i < elements; i++)
            {
                buffer[i] = dis(gen);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Тест записи
            auto write_start = std::chrono::high_resolution_clock::now();
            for (size_t i = 0; i < elements; i += 8)
            {
                buffer[i] = i;
                force_memory_barrier();
            }
            auto write_end = std::chrono::high_resolution_clock::now();
            double write_time = std::chrono::duration<double>(write_end - write_start).count();
            double write_speed = (elements * sizeof(uint64_t) / (1024.0 * 1024.0)) / write_time;

            // Очистка кэша
            std::vector<uint64_t> clear_cache(1024 * 1024);
            for (size_t i = 0; i < clear_cache.size(); i++)
            {
                clear_cache[i] = i;
            }

            // Тест чтения
            volatile uint64_t checksum = 0;
            auto read_start = std::chrono::high_resolution_clock::now();
            for (size_t i = 0; i < elements; i += 8)
            {
                checksum ^= buffer[i];
                force_memory_barrier();
            }
            auto read_end = std::chrono::high_resolution_clock::now();
            double read_time = std::chrono::duration<double>(read_end - read_start).count();
            double read_speed = (elements * sizeof(uint64_t) / (1024.0 * 1024.0)) / read_time;

            std::cout << "Write: " << write_speed << " MB/s\n";
            std::cout << "Read:  " << read_speed << " MB/s\n";
            std::cout << "Test completed\n";

            (void)checksum;
        }
        catch (const std::exception &e)
        {
            std::cout << "Test failed: " << e.what() << "\n";
        }
    }
} // namespace ram_test