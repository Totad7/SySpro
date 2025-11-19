#include "../include/cpu_benchmark.h"
#include <iostream>
#include <windows.h>
#include <process.h>
#include <thread>
#include <algorithm>
#include <cmath>
#include <vector>
#include <iomanip>
#include <random>
#include <atomic>
#include <chrono>
using namespace std;

static atomic<bool> g_running(false);
static CPUBenchmark *g_benchmark = nullptr;

unsigned __stdcall thread_wrapper(void *param)
{
    int thread_id = *(int *)param;
    delete (int *)param;

    if (g_benchmark)
    {
        g_benchmark->worker_thread(thread_id);
    }
    return 0;
}

void CPUBenchmark::worker_thread(int thread_id)
{
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist(1.0, 1000.0);

    long long operations = 0;
    double total_result = 0.0;

    while (g_running.load())
    {
        for (int i = 0; i < 1000 && g_running.load(); i++)
        {
            double a = dist(gen);
            double b = dist(gen);
            double c = dist(gen);
            double d = dist(gen);

            double result1 = sqrt(a) * sin(b) + cos(c) * tan(d);
            double result2 = log(a + 1.0) * exp(b) + pow(c, 1.5) * atan(d);
            double result3 = sinh(a) * cosh(b) + tanh(c) * log1p(d);
            double result4 = erf(a) * tgamma(b) + lgamma(c) * ceil(d);

            total_result += result1 + result2 + result3 + result4;
            operations += 20;
        }

        if (operations > 0)
        {
            add_to_operations_counter(thread_id, operations);
            operations = 0;
        }
    }

    if (total_result < 0)
    {
        volatile double prevent_optimization = total_result;
        (void)prevent_optimization;
    }
}

CPUBenchmark::CPUBenchmark() : running(false)
{
    unsigned int num_threads = thread::hardware_concurrency();
    if (num_threads == 0)
        num_threads = 4;

    operations_counters = vector<long long>(num_threads, 0);
    performance_counters = vector<double>(num_threads, 0.0);
}

CPUBenchmark::~CPUBenchmark()
{
    stop_benchmark();
}

void CPUBenchmark::stop_benchmark()
{
    running.store(false);
    g_running.store(false);
}

BenchmarkResult CPUBenchmark::run_benchmark(int duration_sec)
{
    if (duration_sec <= 0)
    {
        throw invalid_argument("Duration must be positive");
    }

    running.store(true);
    g_running.store(true);
    g_benchmark = this;

    fill(operations_counters.begin(), operations_counters.end(), 0);
    fill(performance_counters.begin(), performance_counters.end(), 0.0);

    unsigned int num_threads = operations_counters.size();

    cout << "\n"
         << string(50, '=') << "\n";
    cout << "STARTING CPU STRESS TEST\n";
    cout << string(50, '=') << "\n";
    cout << "Threads: " << num_threads << "\n";
    cout << "Duration: " << duration_sec << " seconds\n";
    cout << "Status: Loading CPU to maximum capacity...\n\n";

    vector<HANDLE> threads;
    for (unsigned int t = 0; t < num_threads; t++)
    {
        int *thread_id = new int(t);
        HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, thread_wrapper, thread_id, 0, NULL);

        if (thread)
            threads.push_back(thread);
        else
        {
            delete thread_id;
            cerr << "Failed to create thread " << t << endl;
        }
    }

    if (threads.empty())
    {
        throw runtime_error("Failed to create any worker threads");
    }

    auto start_time = chrono::high_resolution_clock::now();
    auto last_update = start_time;
    int elapsed_seconds = 0;

    double current_gflops = 0.0;

    while (elapsed_seconds < duration_sec && running.load())
    {
        this_thread::sleep_for(chrono::milliseconds(100));

        auto now = chrono::high_resolution_clock::now();
        auto total_elapsed = chrono::duration_cast<chrono::milliseconds>(now - start_time);
        elapsed_seconds = total_elapsed.count() / 1000;

        auto time_since_update = chrono::duration_cast<chrono::milliseconds>(now - last_update);
        if (time_since_update.count() >= 1000)
        {
            last_update = now;

            // Суммируем операции всех потоков
            long long total_operations = 0;
            for (const auto &counter : operations_counters)
                total_operations += counter;

            double elapsed_seconds_current = total_elapsed.count() / 1000.0;
            current_gflops = static_cast<double>(total_operations) / (elapsed_seconds_current > 0 ? elapsed_seconds_current * 1e9 : 1.0);

            // Вывод прогресса
            cout << "\rProgress: " << setw(2) << elapsed_seconds << "s / "
                 << duration_sec << "s | "
                 << "Performance: " << fixed << setprecision(2) << current_gflops << " GFLOPS | "
                 << "Threads: " << num_threads << " active" << flush;
        }
    }

    stop_benchmark();

    for (HANDLE thread : threads)
    {
        if (thread)
        {
            WaitForSingleObject(thread, INFINITE);
            CloseHandle(thread);
        }
    }

    auto end_time = chrono::high_resolution_clock::now();
    auto total_duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);

    BenchmarkResult result;
    result.duration_seconds = total_duration.count() / 1000.0;

    double total_ops = 0.0;
    result.core_performance.resize(operations_counters.size());

    for (size_t i = 0; i < operations_counters.size(); i++)
    {
        double core_ops = operations_counters[i];
        result.core_performance[i] = core_ops / (result.duration_seconds * 1e9);
        total_ops += core_ops;
    }

    result.total_operations = static_cast<long long>(total_ops);
    result.total_gflops = total_ops / (result.duration_seconds * 1e9);

    if (!result.core_performance.empty())
    {
        auto [min_it, max_it] = minmax_element(result.core_performance.begin(), result.core_performance.end());
        result.min_single_core_gflops = *min_it;
        result.max_single_core_gflops = *max_it;
    }
    else
    {
        result.min_single_core_gflops = 0.0;
        result.max_single_core_gflops = 0.0;
    }

    result.average_gflops = operations_counters.empty() ? 0.0 : result.total_gflops / operations_counters.size();

    cout << "\n\nTest completed successfully!\n";
    return result;
}

void CPUBenchmark::print_detailed_report(const BenchmarkResult &result)
{
    cout << "\n"
         << string(60, '=') << "\n";
    cout << "DETAILED PERFORMANCE REPORT\n";
    cout << string(60, '=') << "\n";

    cout << fixed << setprecision(3);
    cout << "Test Duration: " << result.duration_seconds << " seconds\n";
    cout << "Total Operations(MOps): " << result.total_operations / 1e9 << " FLOPs\n";
    cout << "Total Performance: " << result.total_gflops << " GFLOPS\n";
    cout << "Max Core Performance: " << result.max_single_core_gflops << " GFLOPS\n";
    cout << "Min Core Performance: " << result.min_single_core_gflops << " GFLOPS\n";
    cout << "Average Core Performance: " << result.average_gflops << " GFLOPS\n";

    cout << "\nPerformance by Core:\n";
    for (size_t i = 0; i < result.core_performance.size(); ++i)
    {
        cout << "  Core " << setw(2) << (i + 1) << ": "
             << setw(8) << result.core_performance[i] << " GFLOPS\n";
    }

    cout << "\nPerformance Analysis:\n";
    double performance_score = result.total_gflops;

    if (performance_score > 100.0)
        cout << "  Outstanding performance! High-end CPU operating at peak capacity.\n";
    else if (performance_score > 50.0)
        cout << "  Excellent performance! CPU is working efficiently.\n";
    else if (performance_score > 20.0)
        cout << "  Good performance. CPU is under significant load.\n";
    else if (performance_score > 10.0)
        cout << "  Average performance. Consider checking for thermal throttling.\n";
    else
        cout << "  Low performance. CPU may be throttling or overloaded.\n";

    if (result.max_single_core_gflops > 0)
    {
        double load_balance = (result.min_single_core_gflops / result.max_single_core_gflops) * 100.0;
        cout << "  Load Balance: " << setprecision(1) << load_balance << "%\n";
    }

    cout << string(60, '=') << "\n";
}