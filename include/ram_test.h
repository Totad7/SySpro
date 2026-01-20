#pragma once
/**
 * memory_bandwidth_test.h - УНИВЕРСАЛЬНЫЙ ТЕСТ ПРОПУСКНОЙ СПОСОБНОСТИ ПАМЯТИ
 */

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <thread>
#include <atomic>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <sstream>

// Подключение специфичных для платформы заголовков для выровненного выделения памяти
#if defined(_MSC_VER)
#include <malloc.h>
#elif defined(__MINGW32__) || defined(__MINGW64__)
#include <mm_malloc.h>
#endif

namespace memory_test_internal
{
    // КОНФИГУРАЦИЯ
    static constexpr size_t CONFIG_BUFFER_BYTES = 2ull * 1024 * 1024 * 1024; // Размер буфера
    static constexpr size_t CONFIG_ALIGNMENT_BYTES = 64;                     // Выравнивание памяти
    static constexpr bool CONFIG_VERBOSE = true;                             // Подробный вывод
    static constexpr size_t CONFIG_WARMUP_ITERATIONS = 2;                    // Итерации разогрева
    static constexpr size_t CONFIG_TEST_ITERATIONS = 5;                      // Итерации тестирования

    // Псевдонимы для удобства работы со временем
    using high_res_clock = std::chrono::high_resolution_clock;
    using time_point_t = std::chrono::time_point<high_res_clock>;

    // Барьер компилятора для предотвращения оптимизации
    inline void compiler_barrier()
    {
#if defined(__GNUC__) || defined(__clang__)
        asm volatile("" ::: "memory");
#else
        std::atomic_thread_fence(std::memory_order_seq_cst);
#endif
    }

    // Кросс-платформенное выделение выровненной памяти
    inline void *aligned_alloc_helper(size_t alignment, size_t size)
    {
#if defined(_MSC_VER)
        return _aligned_malloc(size, alignment);
#elif defined(__MINGW32__) || defined(__MINGW64__)
        return _mm_malloc(size, alignment);
#else
        void *p = nullptr;
        if (posix_memalign(&p, alignment, size) == 0)
            return p;
        return nullptr;
#endif
    }

    // Освобождение выровненной памяти
    inline void aligned_free_helper(void *p)
    {
        if (!p)
            return;
#if defined(_MSC_VER)
        _aligned_free(p);
#elif defined(__MINGW32__) || defined(__MINGW64__)
        _mm_free(p);
#else
        free(p);
#endif
    }

    // Конвертация байтов в количество элементов
    inline size_t bytes_to_elements(size_t bytes) { return bytes / sizeof(uint64_t); }

    // Вычисление разницы времени в секундах
    inline double seconds_between(const time_point_t &a, const time_point_t &b)
    {
        return std::chrono::duration<double>(b - a).count();
    }

    // Форматирование байтов в читаемый вид
    inline std::string format_bytes(double bytes)
    {
        static const char *S[] = {"B", "KB", "MB", "GB", "TB"};
        int idx = 0;
        while (bytes >= 1024.0 && idx < 4)
        {
            bytes /= 1024.0;
            ++idx;
        }
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << bytes << ' ' << S[idx];
        return oss.str();
    }

    // Оптимизированная функция записи
    inline void stream_write(uint64_t *buffer, size_t elements, uint64_t value)
    {
        for (size_t i = 0; i < elements; i += 8)
        {
            buffer[i] = value;
            buffer[i + 1] = value;
            buffer[i + 2] = value;
            buffer[i + 3] = value;
            buffer[i + 4] = value;
            buffer[i + 5] = value;
            buffer[i + 6] = value;
            buffer[i + 7] = value;
        }
    }

    // Оптимизированная функция чтения
    inline uint64_t stream_read(const uint64_t *buffer, size_t elements)
    {
        uint64_t sum0 = 0, sum1 = 0, sum2 = 0, sum3 = 0;
        uint64_t sum4 = 0, sum5 = 0, sum6 = 0, sum7 = 0;

        for (size_t i = 0; i < elements; i += 8)
        {
            sum0 += buffer[i];
            sum1 += buffer[i + 1];
            sum2 += buffer[i + 2];
            sum3 += buffer[i + 3];
            sum4 += buffer[i + 4];
            sum5 += buffer[i + 5];
            sum6 += buffer[i + 6];
            sum7 += buffer[i + 7];
        }
        return sum0 + sum1 + sum2 + sum3 + sum4 + sum5 + sum6 + sum7;
    }

} // namespace memory_test_internal

// Основная функция тестирования пропускной способности памяти
inline void test_ram_speed()
{
    using namespace memory_test_internal;

    // Конфигурация теста
    const size_t buffer_bytes = CONFIG_BUFFER_BYTES;
    const size_t alignment = CONFIG_ALIGNMENT_BYTES;
    const size_t elements = bytes_to_elements(buffer_bytes);

    // Проверка минимального размера буфера
    if (elements < 1024)
    {
        std::cout << "memory_test: buffer too small\n";
        return;
    }

    // Определение количества аппаратных потоков
    const unsigned hw_threads = std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 1u;
    const unsigned mt_threads = std::max(1u, hw_threads);

    // Вывод информации о конфигурации теста
    if (CONFIG_VERBOSE)
    {
        std::cout << "=== MEMORY BANDWIDTH TEST ===\n";
        std::cout << "Buffer size: " << format_bytes(static_cast<double>(buffer_bytes))
                  << " (" << elements << " elements)\n";
        std::cout << "Threads: " << mt_threads << "\n";
        std::cout << "Iterations: " << CONFIG_TEST_ITERATIONS << "\n\n";
    }

    // Выделение выровненной памяти
    uint64_t *buffer = static_cast<uint64_t *>(aligned_alloc_helper(alignment, elements * sizeof(uint64_t)));
    std::vector<uint64_t> fallback;
    bool used_fallback = false;

    // Резервное выделение через std::vector при неудаче
    if (!buffer)
    {
        try
        {
            fallback.resize(elements);
            buffer = fallback.data();
            used_fallback = true;
            if (CONFIG_VERBOSE)
                std::cout << "Using std::vector fallback allocation\n";
        }
        catch (...)
        {
            std::cerr << "Memory allocation failed\n";
            return;
        }
    }

    // Инициализация памяти
    if (CONFIG_VERBOSE)
        std::cout << "Initializing memory...\n";
    for (size_t i = 0; i < elements; ++i)
    {
        buffer[i] = static_cast<uint64_t>(i);
    }

    // Переменные для хранения лучших результатов
    double best_write = 0.0, best_read = 0.0;
    double best_mt_write = 0.0, best_mt_read = 0.0;
    volatile uint64_t checksum = 0;

    // Тест записи в однопоточном режиме
    if (CONFIG_VERBOSE)
        std::cout << "\nSingle-threaded WRITE test...\n";
    for (size_t iter = 0; iter < CONFIG_WARMUP_ITERATIONS + CONFIG_TEST_ITERATIONS; ++iter)
    {
        auto t0 = high_res_clock::now();
        stream_write(buffer, elements, 0x123456789ABCDEF0ull);
        compiler_barrier();
        auto t1 = high_res_clock::now();

        if (iter >= CONFIG_WARMUP_ITERATIONS)
        {
            double sec = seconds_between(t0, t1);
            double mb = (double)elements * sizeof(uint64_t) / (1024.0 * 1024.0);
            double speed = mb / sec;
            best_write = std::max(best_write, speed);
        }
    }

    // Тест чтения в однопоточном режиме
    if (CONFIG_VERBOSE)
        std::cout << "Single-threaded READ test...\n";
    for (size_t iter = 0; iter < CONFIG_WARMUP_ITERATIONS + CONFIG_TEST_ITERATIONS; ++iter)
    {
        auto t0 = high_res_clock::now();
        uint64_t sum = stream_read(buffer, elements);
        checksum ^= sum;
        compiler_barrier();
        auto t1 = high_res_clock::now();

        if (iter >= CONFIG_WARMUP_ITERATIONS)
        {
            double sec = seconds_between(t0, t1);
            double mb = (double)elements * sizeof(uint64_t) / (1024.0 * 1024.0);
            double speed = mb / sec;
            best_read = std::max(best_read, speed);
        }
    }

    // Лямбда-функция для многопоточного теста записи
    auto mt_writer = [&](size_t start, size_t end)
    {
        for (size_t i = start; i < end; i += 4)
        {
            buffer[i] = 0xDEADBEEF;
            if (i + 1 < end)
                buffer[i + 1] = 0xDEADBEEF;
            if (i + 2 < end)
                buffer[i + 2] = 0xDEADBEEF;
            if (i + 3 < end)
                buffer[i + 3] = 0xDEADBEEF;
        }
    };

    // Лямбда-функция для многопоточного теста чтения
    auto mt_reader = [&](size_t start, size_t end, std::atomic<uint64_t> &result)
    {
        uint64_t sum = 0;
        for (size_t i = start; i < end; i += 4)
        {
            sum += buffer[i];
            if (i + 1 < end)
                sum += buffer[i + 1];
            if (i + 2 < end)
                sum += buffer[i + 2];
            if (i + 3 < end)
                sum += buffer[i + 3];
        }
        result.fetch_add(sum, std::memory_order_relaxed);
    };

    // Многопоточные тесты
    if (CONFIG_VERBOSE)
        std::cout << "\nMulti-threaded tests...\n";
    for (size_t attempt = 0; attempt < 3; ++attempt)
    {
        // Многопоточная запись
        std::vector<std::thread> threads;
        size_t chunk = elements / mt_threads;
        auto t0 = high_res_clock::now();
        for (unsigned th = 0; th < mt_threads; ++th)
        {
            size_t s = th * chunk;
            size_t e = (th + 1 == mt_threads) ? elements : s + chunk;
            threads.emplace_back(mt_writer, s, e);
        }
        for (auto &th : threads)
            th.join();
        compiler_barrier();
        auto t1 = high_res_clock::now();
        double mb_total = (double)elements * sizeof(uint64_t) / (1024.0 * 1024.0);
        best_mt_write = std::max(best_mt_write, mb_total / seconds_between(t0, t1));

        // Многопоточное чтение
        threads.clear();
        std::atomic<uint64_t> mt_sum(0);
        t0 = high_res_clock::now();
        for (unsigned th = 0; th < mt_threads; ++th)
        {
            size_t s = th * chunk;
            size_t e = (th + 1 == mt_threads) ? elements : s + chunk;
            threads.emplace_back(mt_reader, s, e, std::ref(mt_sum));
        }
        for (auto &th : threads)
            th.join();
        compiler_barrier();
        t1 = high_res_clock::now();
        best_mt_read = std::max(best_mt_read, mb_total / seconds_between(t0, t1));
        checksum ^= mt_sum.load(std::memory_order_relaxed);
    }

    // Форматирование скорости для вывода
    auto print_speed = [](double mb_per_s)
    {
        double gb = mb_per_s / 1024.0;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << mb_per_s << " MB/s (" << gb << " GB/s)";
        return oss.str();
    };

    // Вывод результатов
    std::cout << "\n=== TEST RESULTS ===\n";
    std::cout << "Single-thread WRITE: " << print_speed(best_write) << '\n';
    std::cout << "Single-thread READ:  " << print_speed(best_read) << '\n';
    std::cout << "Multi-thread WRITE:  " << print_speed(best_mt_write) << '\n';
    std::cout << "Multi-thread READ:   " << print_speed(best_mt_read) << '\n';
    std::cout << "Verification checksum: 0x" << std::hex << checksum << std::dec << '\n';

    // Общий анализ результатов
    std::cout << "\n=== PERFORMANCE ASSESSMENT ===\n";
    if (best_mt_read < 15000)
    {
        std::cout << "Note: Memory bandwidth may be lower than expected\n";
        std::cout << "Consider checking memory configuration\n";
    }
    else
    {
        std::cout << "Memory bandwidth is within expected range\n";
    }

    // Освобождение памяти
    if (!used_fallback)
        aligned_free_helper(buffer);
    std::cout << "=== TEST COMPLETED ===\n\n";
}