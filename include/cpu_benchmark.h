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

// Структура для хранения результатов бенчмарка
struct BenchmarkResult
{
    double duration_seconds;              // Длительность теста в секундах
    long long total_operations;           // Общее количество операций
    double total_gflops;                  // Общая производительность в GFLOPS
    double max_single_core_gflops;        // Максимальная производительность одного ядра
    double min_single_core_gflops;        // Минимальная производительность одного ядра
    double average_gflops;                // Средняя производительность на ядро
    std::vector<double> core_performance; // Производительность каждого ядра
};

// Класс для тестирования производительности CPU
class CPUBenchmark
{
private:
    std::atomic<bool> running;                  // Атомарный флаг для управления выполнением теста
    std::vector<long long> operations_counters; // Счетчики операций для каждого потока
    std::vector<double> performance_counters;   // Счетчики производительности для каждого потока

public:
    // Конструктор и деструктор
    CPUBenchmark();
    ~CPUBenchmark();

    // Основные методы
    BenchmarkResult run_benchmark(int duration_sec);           // Запуск бенчмарка на указанное время
    void print_detailed_report(const BenchmarkResult &result); // Вывод детального отчета

    // Методы для работы с потоками
    void worker_thread(int thread_id); // Функция рабочего потока
    void stop_benchmark();             // Остановка бенчмарка

    // Методы доступа к счетчикам операций
    size_t get_operations_counters_size() const { return operations_counters.size(); } // Количество счетчиков

    // Получение значения счетчика по индексу
    long long get_operations_counter(size_t index) const
    {
        return (index < operations_counters.size()) ? operations_counters[index] : 0;
    }

    // Сброс счетчика по индексу
    void reset_operations_counter(size_t index)
    {
        if (index < operations_counters.size())
        {
            operations_counters[index] = 0;
        }
    }

    // Добавление значения к счетчику
    void add_to_operations_counter(size_t index, long long value)
    {
        if (index < operations_counters.size())
        {
            operations_counters[index] += value;
        }
    }
};

#endif