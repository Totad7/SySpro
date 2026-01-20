#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <cstdio>
#include <thread>
#include <limits>

namespace ssd_test
{

    // Функция для получения корректного числового ввода от пользователя
    inline size_t get_valid_input(const std::string &prompt)
    {
        size_t value = 0;
        while (true)
        {
            std::cout << prompt;
            std::cin >> value;

            if (std::cin.fail())
            {
                // Очистка буфера ввода при ошибке
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Invalid input! Please enter a positive number.\n";
            }
            else if (value == 0)
            {
                // Проверка на нулевое значение
                std::cout << "Error! Value must be greater than 0.\n";
            }
            else
            {
                // Успешный ввод, очищаем буфер
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                return value;
            }
        }
    }

    // Получение выбора из меню с валидацией
    inline int get_menu_choice()
    {
        int choice = 0;
        while (true)
        {
            std::cout << "Enter your choice (1-5): ";
            std::cin >> choice;

            if (std::cin.fail())
            {
                // Обработка некорректного ввода (не число)
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Invalid input! Please enter a number 1-5.\n";
            }
            else if (choice < 1 || choice > 5)
            {
                // Проверка диапазона значений
                std::cout << "Error! Please enter a number between 1 and 5.\n";
            }
            else
            {
                // Корректный ввод
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                return choice;
            }
        }
    }

    // Отображение главного меню теста SSD
    inline void show_main_menu()
    {
        std::cout << "=================================\n";
        std::cout << "      SSD PERFORMANCE TEST\n";
        std::cout << "=================================\n";
        std::cout << "Choose test mode:\n";
        std::cout << "1. Quick test (512 MB)\n";
        std::cout << "2. Standard test (1024 MB)\n";
        std::cout << "3. Extended test (2048 MB)\n";
        std::cout << "4. Large test (4096 MB)\n";
        std::cout << "5. Custom test (manual parameters)\n";
        std::cout << "=================================\n";
    }

    // Основная функция тестирования производительности SSD
    inline void perform_ssd_test(size_t file_size_mb, size_t block_size_kb)
    {
        // Временный файл для тестирования
        std::string file_path = "ssd_test_temp.dat";
        size_t total_bytes = file_size_mb * 1024 * 1024;
        size_t block_bytes = block_size_kb * 1024;

        // Проверка корректности параметров
        if (block_bytes > total_bytes)
        {
            std::cout << "Error! Block size cannot be larger than file size.\n";
            return;
        }

        // Вывод параметров теста
        std::cout << "\n=== Test Configuration ===\n";
        std::cout << "File size: " << file_size_mb << " MB\n";
        std::cout << "Block size: " << block_size_kb << " KB\n";
        std::cout << "Total data: " << (total_bytes / (1024.0 * 1024.0)) << " MB\n";
        std::cout << "===========================\n\n";

        // Буфер для операций чтения/записи
        std::vector<char> buffer(block_bytes, 'A');

        // ====== ТЕСТ ЗАПИСИ ======
        std::cout << "Starting write operation...\n";
        auto write_start = std::chrono::high_resolution_clock::now();

        // Открытие файла для записи в бинарном режиме
        std::ofstream ofs(file_path, std::ios::binary);
        if (!ofs)
        {
            std::cerr << "Error creating file for writing!\n";
            return;
        }

        // Расчет количества блоков для записи
        size_t blocks_count = total_bytes / block_bytes;
        for (size_t i = 0; i < blocks_count; i++)
        {
            // Запись блока данных
            ofs.write(buffer.data(), block_bytes);
            if (!ofs)
            {
                // Обработка ошибки записи
                std::cerr << "\nWrite error at block " << i << "\n";
                ofs.close();
                std::remove(file_path.c_str());
                return;
            }

            // Отображение прогресса записи
            if (blocks_count >= 10 && (i % (blocks_count / 10) == 0 || i == blocks_count - 1))
            {
                int progress = static_cast<int>((i * 100) / blocks_count);
                std::cout << "\rWriting: " << progress << "%";
                std::cout.flush();
            }
        }
        ofs.close();

        // Расчет скорости записи
        auto write_end = std::chrono::high_resolution_clock::now();
        double write_time_s = std::chrono::duration<double>(write_end - write_start).count();
        double write_speed = static_cast<double>(file_size_mb) / write_time_s;
        std::cout << "\rWriting: 100% - Completed!\n";

        // Пауза между операциями записи и чтения
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // ====== ТЕСТ ЧТЕНИЯ ======
        std::cout << "Starting read operation...\n";
        auto read_start = std::chrono::high_resolution_clock::now();

        // Открытие файла для чтения в бинарном режиме
        std::ifstream ifs(file_path, std::ios::binary);
        if (!ifs)
        {
            std::cerr << "Error opening file for reading!\n";
            std::remove(file_path.c_str());
            return;
        }

        // Чтение данных блоками
        for (size_t i = 0; i < blocks_count; i++)
        {
            ifs.read(buffer.data(), block_bytes);
            if (!ifs)
            {
                // Обработка ошибки чтения
                std::cerr << "\nRead error at block " << i << "\n";
                ifs.close();
                std::remove(file_path.c_str());
                return;
            }

            // Отображение прогресса чтения
            if (blocks_count >= 10 && (i % (blocks_count / 10) == 0 || i == blocks_count - 1))
            {
                int progress = static_cast<int>((i * 100) / blocks_count);
                std::cout << "\rReading: " << progress << "%";
                std::cout.flush();
            }
        }
        ifs.close();

        // Расчет скорости чтения
        auto read_end = std::chrono::high_resolution_clock::now();
        double read_time_s = std::chrono::duration<double>(read_end - read_start).count();
        double read_speed = static_cast<double>(file_size_mb) / read_time_s;
        std::cout << "\rReading: 100% - Completed!\n";

        // Удаление временного файла
        std::remove(file_path.c_str());

        // ====== ВЫВОД РЕЗУЛЬТАТОВ ======
        std::cout << "\n=== SSD Test Results ===\n";
        std::cout << "Test size: " << file_size_mb << " MB\n";
        std::cout << "Block size: " << block_size_kb << " KB\n";
        std::cout << "---------------------------------\n";
        std::cout << "Write: " << write_speed << " MB/s (" << write_time_s << " s)\n";
        std::cout << "Read:  " << read_speed << " MB/s (" << read_time_s << " s)\n";
        std::cout << "---------------------------------\n";

        // Анализ производительности
        std::cout << "\n=== Performance Analysis ===\n";

        // Оценка скорости записи
        if (write_speed < 100)
        {
            std::cout << "Write: SLOW (may indicate low free space or drive issues)\n";
        }
        else if (write_speed < 300)
        {
            std::cout << "Write: NORMAL for filled SSD\n";
        }
        else if (write_speed < 800)
        {
            std::cout << "Write: GOOD\n";
        }
        else
        {
            std::cout << "Write: EXCELLENT\n";
        }

        // Оценка скорости чтения
        if (read_speed < 500)
        {
            std::cout << "Read:  SLOW for NVMe (check drive health)\n";
        }
        else if (read_speed < 1500)
        {
            std::cout << "Read:  NORMAL\n";
        }
        else if (read_speed < 2500)
        {
            std::cout << "Read:  GOOD\n";
        }
        else
        {
            std::cout << "Read:  EXCELLENT\n";
        }

        std::cout << "=============================\n";
    }

    // Основная точка входа для теста SSD
    inline void run_ssd_test()
    {
        show_main_menu();
        int choice = get_menu_choice();

        size_t file_size_mb = 0;
        size_t block_size_kb = 0;

        // Обработка выбора пользователя
        switch (choice)
        {
        case 1: // Быстрый тест
            file_size_mb = 512;
            block_size_kb = 512;
            std::cout << "\nSelected: Quick test (512 MB)\n";
            break;

        case 2: // Стандартный тест
            file_size_mb = 1024;
            block_size_kb = 1024;
            std::cout << "\nSelected: Standard test (1024 MB)\n";
            break;

        case 3: // Расширенный тест
            file_size_mb = 2048;
            block_size_kb = 1024;
            std::cout << "\nSelected: Extended test (2048 MB)\n";
            break;

        case 4: // Большой тест
            file_size_mb = 4096;
            block_size_kb = 2048;
            std::cout << "\nSelected: Large test (4096 MB)\n";
            break;

        case 5: // Пользовательский тест
            std::cout << "\n=== Custom Test Parameters ===\n";
            file_size_mb = get_valid_input("Enter test file size (MB): ");
            block_size_kb = get_valid_input("Enter block size (KB): ");
            std::cout << "Selected: Custom test (" << file_size_mb << " MB)\n";
            break;
        }

        // Предупреждение для больших тестов
        if (file_size_mb > 2048)
        {
            std::cout << "\nWarning: This test will use " << file_size_mb << " MB of disk space.\n";
            std::cout << "Make sure you have enough free space.\n";
            std::cout << "Continue? (y/n): ";

            char confirm;
            std::cin >> confirm;
            if (confirm != 'y' && confirm != 'Y')
            {
                std::cout << "Test cancelled.\n";
                return;
            }
        }

        // Запуск теста с выбранными параметрами
        perform_ssd_test(file_size_mb, block_size_kb);
    }

    // Быстрая функция для запуска теста с параметрами по умолчанию
    inline void run_quick_ssd_test(size_t file_size_mb = 1024)
    {
        std::cout << "\n*** Running Quick SSD Test (" << file_size_mb << " MB) ***\n";
        size_t block_size_kb = (file_size_mb >= 2048) ? 2048 : 1024;
        perform_ssd_test(file_size_mb, block_size_kb);
    }

} // namespace ssd_test