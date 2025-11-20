#pragma once
/**
 * ram_test.h - IMPROVED VERSION
 * Better memory bandwidth test with optimizations
 */

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <thread>
#include <atomic>
#include <random>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <sstream>

#if defined(_MSC_VER)
#include <malloc.h>
#elif defined(__MINGW32__) || defined(__MINGW64__)
#include <mm_malloc.h>
#endif

namespace ram_test_internal
{
    // CONFIG: tweak these if needed
    static constexpr size_t CONFIG_BUFFER_BYTES = 2ull * 1024 * 1024 * 1024; // 2 GiB to avoid cache effects
    static constexpr size_t CONFIG_ALIGNMENT_BYTES = 64;
    static constexpr bool CONFIG_VERBOSE = true;
    static constexpr size_t CONFIG_WARMUP_ITERATIONS = 2;
    static constexpr size_t CONFIG_TEST_ITERATIONS = 5;

    using high_res_clock = std::chrono::high_resolution_clock;
    using time_point_t = std::chrono::time_point<high_res_clock>;

    inline void compiler_barrier()
    {
#if defined(__GNUC__) || defined(__clang__)
        asm volatile("" ::: "memory");
#else
        std::atomic_thread_fence(std::memory_order_seq_cst);
#endif
    }

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

    inline size_t bytes_to_elements(size_t bytes) { return bytes / sizeof(uint64_t); }

    inline double seconds_between(const time_point_t &a, const time_point_t &b)
    {
        return std::chrono::duration<double>(b - a).count();
    }

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

    // Optimized memory access patterns
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

} // namespace ram_test_internal

inline void test_ram_speed()
{
    using namespace ram_test_internal;

    const size_t buffer_bytes = CONFIG_BUFFER_BYTES;
    const size_t alignment = CONFIG_ALIGNMENT_BYTES;
    const size_t elements = bytes_to_elements(buffer_bytes);

    if (elements < 1024)
    {
        std::cout << "ram_test: buffer too small\n";
        return;
    }

    const unsigned hw_threads = std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 1u;
    const unsigned mt_threads = std::max(1u, hw_threads);

    if (CONFIG_VERBOSE)
    {
        std::cout << "=== IMPROVED MEMORY BANDWIDTH TEST ===\n";
        std::cout << "Buffer: " << format_bytes(static_cast<double>(buffer_bytes))
                  << " (" << elements << " uint64_t elements)\n";
        std::cout << "Threads: " << mt_threads << "\n\n";
    }

    // Allocate memory
    uint64_t *buffer = static_cast<uint64_t *>(aligned_alloc_helper(alignment, elements * sizeof(uint64_t)));
    std::vector<uint64_t> fallback;
    bool used_fallback = false;

    if (!buffer)
    {
        try
        {
            fallback.resize(elements);
            buffer = fallback.data();
            used_fallback = true;
            if (CONFIG_VERBOSE)
                std::cout << "Using std::vector fallback\n";
        }
        catch (...)
        {
            std::cerr << "Allocation failed\n";
            return;
        }
    }

    // Initialize memory
    if (CONFIG_VERBOSE)
        std::cout << "Initializing memory...\n";
    for (size_t i = 0; i < elements; ++i)
    {
        buffer[i] = static_cast<uint64_t>(i);
    }

    // Test variables
    double best_write = 0.0, best_read = 0.0;
    double best_mt_write = 0.0, best_mt_read = 0.0;
    volatile uint64_t checksum = 0;

    // Single-threaded WRITE test
    if (CONFIG_VERBOSE)
        std::cout << "\nSingle-threaded WRITE...\n";
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

    // Single-threaded READ test
    if (CONFIG_VERBOSE)
        std::cout << "Single-threaded READ...\n";
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

    // Multi-threaded tests
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

    if (CONFIG_VERBOSE)
        std::cout << "\nMulti-threaded tests...\n";
    for (size_t attempt = 0; attempt < 3; ++attempt)
    {
        // MT WRITE
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

        // MT READ
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

    // Results
    auto print_speed = [](double mb_per_s)
    {
        double gb = mb_per_s / 1024.0;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << mb_per_s << " MB/s (" << gb << " GB/s)";
        return oss.str();
    };

    std::cout << "\n=== RESULTS ===\n";
    std::cout << "Single-thread WRITE: " << print_speed(best_write) << '\n';
    std::cout << "Single-thread READ:  " << print_speed(best_read) << '\n';
    std::cout << "Multi-thread WRITE:  " << print_speed(best_mt_write) << '\n';
    std::cout << "Multi-thread READ:   " << print_speed(best_mt_read) << '\n';
    std::cout << "Checksum: 0x" << std::hex << checksum << std::dec << '\n';

    // Check if results are reasonable
    std::cout << "\n=== ASSESSMENT ===\n";
    if (best_mt_read < 15000)
    {
        std::cout << "WARNING: Performance seems low for DDR4 3200\n";
        std::cout << "Expected: 20,000+ MB/s multi-threaded\n";
        std::cout << "Check: XMP profile in BIOS, dual-channel mode\n";
    }
    else
    {
        std::cout << "Performance looks reasonable\n";
    }

    if (!used_fallback)
        aligned_free_helper(buffer);
    std::cout << "=== END ===\n\n";
}