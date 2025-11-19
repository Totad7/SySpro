#ifndef CPU_BENCHMARK_H
#define CPU_BENCHMARK_H

#include <atomic>
#include <vector>
#include <chrono>
#include <random>
#include <thread>
#include <iostream>
#include <iomanip>
#include <algorithm>

struct BenchmarkResult
{
    double duration_seconds;
    long long total_operations;
    double total_gflops;
    double max_single_core_gflops;
    double min_single_core_gflops;
    double average_gflops;
    std::vector<double> core_performance;
};

class CPUBenchmark
{
private:
    std::atomic<bool> running;
    std::vector<long long> operations_counters;
    std::vector<double> performance_counters;

public:
    CPUBenchmark();
    ~CPUBenchmark();
    BenchmarkResult run_benchmark(int duration_sec);
    void print_detailed_report(const BenchmarkResult &result);

    void worker_thread(int thread_id);
    void stop_benchmark();

    size_t get_operations_counters_size() const { return operations_counters.size(); }
    long long get_operations_counter(size_t index) const
    {
        return (index < operations_counters.size()) ? operations_counters[index] : 0;
    }
    void reset_operations_counter(size_t index)
    {
        if (index < operations_counters.size())
        {
            operations_counters[index] = 0;
        }
    }
    void add_to_operations_counter(size_t index, long long value)
    {
        if (index < operations_counters.size())
        {
            operations_counters[index] += value;
        }
    }
};

#endif