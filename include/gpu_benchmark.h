#pragma once
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath>
#include <random>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

// Заголовки OpenCL
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/opencl.h>

namespace gpu_benchmark_final
{

    // Структура float4 для SIMD операций (4 числа с плавающей точкой)
    struct float4
    {
        float x, y, z, w;
        float4(float x = 0, float y = 0, float z = 0, float w = 0) : x(x), y(y), z(z), w(w) {}
    };

    class UniversalGPUTester
    {
    private:
        cl_context context;         // Контекст OpenCL
        cl_device_id device;        // Устройство (GPU/CPU)
        cl_command_queue queue;     // Очередь команд
        bool opencl_available;      // Доступность OpenCL
        std::string device_name;    // Имя устройства
        std::string device_vendor;  // Производитель
        bool is_real_gpu;           // Флаг реального GPU (не CPU)
        size_t max_work_group_size; // Максимальный размер рабочей группы
        size_t compute_units;       // Количество вычислительных единиц
        cl_ulong global_mem_size;   // Объем глобальной памяти
        cl_uint clock_frequency;    // Частота тактов
        cl_device_type device_type; // Тип устройства

        // Для валидации
        bool gpu_execution_validated; // Проверка реального исполнения на GPU
        double gpu_validation_score;  // Результат валидации

        // Структура для хранения результатов тестов
        struct TestResult
        {
            std::string test_name; // Название теста
            double score;          // Результат
            std::string unit;      // Единицы измерения
            std::string rating;    // Оценка (EXCELLENT, GOOD и т.д.)
            bool validated;        // Валидность результата
        };

        std::vector<TestResult> results; // Массив результатов

        // Безопасная проверка ошибок OpenCL
        void check_cl_error(cl_int ret, const std::string &operation)
        {
            if (ret != CL_SUCCESS)
            {
                std::string error_msg = "OpenCL error in " + operation + ": " + std::to_string(ret);
                throw std::runtime_error(error_msg);
            }
        }

        // Создание программы с исходным кодом и получением лога сборки
        cl_program create_program_with_source(const char *kernel_code)
        {
            cl_int ret;
            cl_program program = clCreateProgramWithSource(context, 1, &kernel_code, NULL, &ret);
            check_cl_error(ret, "clCreateProgramWithSource");

            // Компиляция программы
            ret = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
            if (ret != CL_SUCCESS)
            {
                // Получение лога сборки при ошибке
                size_t log_size;
                clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
                std::vector<char> build_log(log_size);
                clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, build_log.data(), NULL);

                std::string error_msg = "Program build failed:\n" + std::string(build_log.data());
                clReleaseProgram(program);
                throw std::runtime_error(error_msg);
            }

            return program;
        }

        // Безопасное создание буфера памяти
        cl_mem create_buffer(cl_mem_flags flags, size_t size, void *host_ptr = nullptr)
        {
            cl_int ret;
            cl_mem buffer = clCreateBuffer(context, flags, size, host_ptr, &ret);
            check_cl_error(ret, "clCreateBuffer");
            return buffer;
        }

        // Безопасное создание ядра
        cl_kernel create_kernel(cl_program program, const std::string &kernel_name)
        {
            cl_int ret;
            cl_kernel kernel = clCreateKernel(program, kernel_name.c_str(), &ret);
            check_cl_error(ret, "clCreateKernel");
            return kernel;
        }

        // ТЕСТ ВАЛИДАЦИИ РЕАЛЬНОГО GPU
        // Проверяет, действительно ли код выполняется на GPU, а не на CPU
        bool validate_gpu_execution()
        {
            try
            {
                // Ядро, которое неэффективно на CPU, но эффективно на GPU
                const char *validation_kernel =
                    "__kernel void gpu_validation(__global float4* data) {\n"
                    "    int gid = get_global_id(0);\n"
                    "    float4 x = data[gid];\n"
                    "    float4 result = x;\n"
                    "    \n"
                    "    // Оптимизировано для GPU: много параллельных операций\n"
                    "    for(int i = 0; i < 1024; i++) {\n"
                    "        result = mad(result, x, (float4)(1.0f));\n"
                    "        result = sin(result) + cos(result);\n"
                    "    }\n"
                    "    \n"
                    "    data[gid] = result;\n"
                    "}\n";

                const size_t work_items = 16384; // Оптимально для GPU
                std::vector<float4> data(work_items);

                // Инициализация случайными значениями
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_real_distribution<float> dis(0.1f, 2.0f);
                for (auto &v : data)
                    v = float4(dis(gen), dis(gen), dis(gen), dis(gen));

                cl_program program = create_program_with_source(validation_kernel);
                cl_kernel kernel = create_kernel(program, "gpu_validation");

                cl_mem buffer = create_buffer(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                              work_items * sizeof(float4), data.data());
                check_cl_error(clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer), "clSetKernelArg");

                size_t global_size = work_items;
                size_t local_size = 256;

                // Измерение времени выполнения
                auto start = std::chrono::high_resolution_clock::now();
                check_cl_error(clEnqueueNDRangeKernel(queue, kernel, 1, NULL,
                                                      &global_size, &local_size, 0, NULL, NULL),
                               "clEnqueueNDRangeKernel");
                check_cl_error(clFinish(queue), "clFinish");
                auto end = std::chrono::high_resolution_clock::now();

                double time_ms = std::chrono::duration<double, std::milli>(end - start).count();

                // Расчет операций
                double operations = work_items * 1024 * 8 * 4; // 8 операций × 4 компонента
                double gflops = operations / (time_ms * 1e6);  // GFLOPS

                gpu_validation_score = gflops;

                // Пороги валидации
                if (device_type == CL_DEVICE_TYPE_GPU)
                {
                    // Реальный GPU должен показывать > 100 GFLOPS в этом тесте
                    gpu_execution_validated = (gflops > 100.0);
                }
                else
                {
                    // CPU будет значительно медленнее
                    gpu_execution_validated = false;
                }

                // Освобождение ресурсов
                clReleaseMemObject(buffer);
                clReleaseKernel(kernel);
                clReleaseProgram(program);

                return gpu_execution_validated;
            }
            catch (...)
            {
                gpu_execution_validated = false;
                return false;
            }
        }

        // Инициализация OpenCL с выбором устройства
        bool init_opencl()
        {
            cl_platform_id platform;
            cl_int ret;

            try
            {
                // Получение платформы
                ret = clGetPlatformIDs(1, &platform, NULL);
                if (ret != CL_SUCCESS)
                    return false;

                // Поиск всех устройств
                cl_uint num_devices = 0;

                // Сначала ищем GPU устройства
                ret = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices);
                std::vector<cl_device_id> gpu_devices(num_devices);

                if (num_devices > 0 && ret == CL_SUCCESS)
                {
                    // Получаем GPU устройства
                    ret = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, num_devices,
                                         gpu_devices.data(), NULL);
                    if (ret == CL_SUCCESS)
                    {
                        // Используем первый найденный GPU
                        device = gpu_devices[0];
                        device_type = CL_DEVICE_TYPE_GPU;
                        is_real_gpu = true;
                    }
                }

                // Если GPU не найден, пробуем CPU
                if (!device)
                {
                    ret = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 0, NULL, &num_devices);
                    if (num_devices > 0 && ret == CL_SUCCESS)
                    {
                        std::vector<cl_device_id> cpu_devices(num_devices);
                        ret = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, num_devices,
                                             cpu_devices.data(), NULL);
                        if (ret == CL_SUCCESS)
                        {
                            device = cpu_devices[0];
                            device_type = CL_DEVICE_TYPE_CPU;
                            is_real_gpu = false;
                        }
                    }
                }

                if (!device)
                    return false;

                // Получение информации об устройстве
                char name[256];
                char vendor[256];
                clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(name), name, NULL);
                clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(vendor), vendor, NULL);
                device_name = name;
                device_vendor = vendor;

                clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);
                clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_work_group_size), &max_work_group_size, NULL);
                clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(global_mem_size), &global_mem_size, NULL);
                clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_frequency), &clock_frequency, NULL);

                // Создание контекста и очереди команд
                context = clCreateContext(NULL, 1, &device, NULL, NULL, &ret);
                check_cl_error(ret, "clCreateContext");

                queue = clCreateCommandQueue(context, device, 0, &ret);
                check_cl_error(ret, "clCreateCommandQueue");

                // Валидация исполнения на GPU
                gpu_execution_validated = validate_gpu_execution();

                return true;
            }
            catch (const std::exception &e)
            {
                // Очистка ресурсов при ошибке
                if (context)
                    clReleaseContext(context);
                if (queue)
                    clReleaseCommandQueue(queue);
                return false;
            }
        }

        // Добавление результата теста
        void add_result(const std::string &name, double score, const std::string &unit, bool validated = true)
        {
            std::string rating;

            if (!validated || !gpu_execution_validated)
            {
                rating = "UNVERIFIED"; // Непроверенный результат
            }
            else if (name.find("Memory") != std::string::npos)
            {
                // Оценка пропускной способности памяти для реального GPU
                if (score > 400)
                    rating = "EXCELLENT";
                else if (score > 250)
                    rating = "VERY GOOD";
                else if (score > 150)
                    rating = "GOOD";
                else if (score > 80)
                    rating = "AVERAGE";
                else if (score > 30)
                    rating = "LIMITED";
                else
                    rating = "LOW";
            }
            else if (name.find("Compute") != std::string::npos)
            {
                // Оценка вычислительной производительности
                if (score > 5000)
                    rating = "EXCELLENT";
                else if (score > 2000)
                    rating = "VERY GOOD";
                else if (score > 800)
                    rating = "GOOD";
                else if (score > 300)
                    rating = "AVERAGE";
                else if (score > 100)
                    rating = "LIMITED";
                else
                    rating = "LOW";
            }
            else
            {
                rating = "TESTED"; // Общий статус
            }

            results.push_back({name, score, unit, rating, validated});
        }

        // Расчет безопасного количества рабочих элементов
        size_t calculate_safe_work_items()
        {
            if (!gpu_execution_validated && is_real_gpu)
            {
                // Консервативный размер если GPU не валидирован
                return 32768;
            }

            size_t base_items = compute_units * std::min(max_work_group_size, size_t(256)) * 32;

            if (is_real_gpu && gpu_execution_validated)
            {
                // Больше элементов для реального GPU
                return std::min(std::max(base_items, size_t(65536)), size_t(1048576));
            }
            else
            {
                // Меньше для CPU или непроверенного GPU
                return std::min(base_items, size_t(32768));
            }
        }

        // Расчет безопасного объема памяти для тестирования
        size_t calculate_safe_memory_size()
        {
            size_t max_safe = global_mem_size / 8; // 12.5% от общей памяти
            size_t reasonable_max;

            if (is_real_gpu && gpu_execution_validated)
            {
                reasonable_max = 128 * 1024 * 1024; // 128MB для GPU
            }
            else
            {
                reasonable_max = 32 * 1024 * 1024; // 32MB для CPU/непроверенного
            }

            return std::min(max_safe, reasonable_max);
        }

        // ТЕСТ ПРОПУСКНОЙ СПОСОБНОСТИ ПАМЯТИ
        void test_memory_bandwidth_real()
        {
            if (!opencl_available)
                return;

            try
            {
                size_t test_memory = calculate_safe_memory_size();
                size_t element_count = test_memory / sizeof(float);

                if (element_count < 4096)
                    throw std::runtime_error("Memory too small");

                // Реальный тест памяти с правильными паттернами доступа
                const char *kernel_code =
                    "__kernel void real_memory_test(__global float4* a, __global float4* b, __global float4* c) {\n"
                    "    int idx = get_global_id(0);\n"
                    "    int stride = get_global_size(0);\n"
                    "    \n"
                    "    // Последовательный доступ (хорошо и для GPU и для CPU)\n"
                    "    float4 sum = (float4)(0.0f);\n"
                    "    for(int i = 0; i < 16; i++) {\n"
                    "        int pos = (idx + i * stride) % (get_global_size(0));\n"
                    "        sum += a[pos] * b[pos];\n"
                    "    }\n"
                    "    c[idx] = sum;\n"
                    "}\n";

                cl_program program = create_program_with_source(kernel_code);
                cl_kernel kernel = create_kernel(program, "real_memory_test");

                size_t float4_count = element_count / 4;
                size_t actual_count = (float4_count / 256) * 256; // Выравнивание по 256

                // Подготовка тестовых данных
                std::vector<float4> a(actual_count);
                std::vector<float4> b(actual_count);
                std::vector<float4> c(actual_count);

                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_real_distribution<float> dis(0.1f, 1.0f);

                for (size_t i = 0; i < actual_count; i++)
                {
                    a[i] = float4(dis(gen), dis(gen), dis(gen), dis(gen));
                    b[i] = float4(dis(gen), dis(gen), dis(gen), dis(gen));
                }

                // Создание буферов в памяти устройства
                cl_mem buf_a = create_buffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                             actual_count * sizeof(float4), a.data());
                cl_mem buf_b = create_buffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                             actual_count * sizeof(float4), b.data());
                cl_mem buf_c = create_buffer(CL_MEM_WRITE_ONLY,
                                             actual_count * sizeof(float4), NULL);

                // Установка аргументов ядра
                check_cl_error(clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_a), "clSetKernelArg");
                check_cl_error(clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_b), "clSetKernelArg");
                check_cl_error(clSetKernelArg(kernel, 2, sizeof(cl_mem), &buf_c), "clSetKernelArg");

                size_t global_size = actual_count;
                size_t local_size = 256;

                // Выравнивание размера рабочей группы
                while (global_size % local_size != 0)
                    local_size /= 2;

                // Прогрев (warm-up)
                for (int i = 0; i < 2; i++)
                {
                    check_cl_error(clEnqueueNDRangeKernel(queue, kernel, 1, NULL,
                                                          &global_size, &local_size, 0, NULL, NULL),
                                   "clEnqueueNDRangeKernel");
                }
                clFinish(queue);

                // Основное измерение
                const int iterations = 10;
                auto start = std::chrono::high_resolution_clock::now();

                for (int i = 0; i < iterations; i++)
                {
                    check_cl_error(clEnqueueNDRangeKernel(queue, kernel, 1, NULL,
                                                          &global_size, &local_size, 0, NULL, NULL),
                                   "clEnqueueNDRangeKernel");
                }
                clFinish(queue);

                auto end = std::chrono::high_resolution_clock::now();
                double time_seconds = std::chrono::duration<double>(end - start).count();

                // Точный расчет пропускной способности
                // Каждая итерация: чтение a, чтение b, запись c = 3 передачи
                // Каждый рабочий элемент обрабатывает 16 элементов
                double bytes_processed = actual_count * 16 * 3 * sizeof(float4) * iterations;
                double bandwidth = bytes_processed / (time_seconds * 1024 * 1024 * 1024); // GB/s

                // Валидация результата
                bool validated = gpu_execution_validated;
                if (is_real_gpu && bandwidth < 20.0) // Нереалистично для GPU
                {
                    validated = false;
                }

                add_result("Memory Bandwidth", bandwidth, "GB/s", validated);

                // Освобождение ресурсов
                clReleaseMemObject(buf_a);
                clReleaseMemObject(buf_b);
                clReleaseMemObject(buf_c);
                clReleaseKernel(kernel);
                clReleaseProgram(program);
            }
            catch (const std::exception &e)
            {
                add_result("Memory Bandwidth", 0, "GB/s", false);
            }
        }

        // ТЕСТ ВЫЧИСЛИТЕЛЬНОЙ ПРОИЗВОДИТЕЛЬНОСТИ
        void test_compute_performance_real()
        {
            if (!opencl_available)
                return;

            try
            {
                size_t work_items = calculate_safe_work_items();

                // Реальный тест вычислений со смешанными операциями
                const char *kernel_code =
                    "__kernel void real_compute_test(__global float4* data) {\n"
                    "    int idx = get_global_id(0);\n"
                    "    float4 x = data[idx];\n"
                    "    \n"
                    "    // Реальная вычислительная нагрузка\n"
                    "    float4 result = x;\n"
                    "    for(int iter = 0; iter < 64; iter++) {\n"
                    "        // Смешанная арифметика\n"
                    "        result = mad(result, x, (float4)(1.0f));\n"
                    "        result = 0.5f * (result + 1.0f / result);\n" // Шаг Ньютона
                    "        // Тригонометрия (дорогая операция)\n"
                    "        result.x = sin(result.x) + cos(result.y);\n"
                    "        result.y = sin(result.y) + cos(result.z);\n"
                    "        result.z = sin(result.z) + cos(result.w);\n"
                    "        result.w = sin(result.w) + cos(result.x);\n"
                    "    }\n"
                    "    \n"
                    "    data[idx] = result;\n"
                    "}\n";

                cl_program program = create_program_with_source(kernel_code);
                cl_kernel kernel = create_kernel(program, "real_compute_test");

                // Подготовка тестовых данных
                std::vector<float4> data(work_items);
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_real_distribution<float> dis(0.5f, 1.5f);

                for (size_t i = 0; i < work_items; i++)
                {
                    data[i] = float4(dis(gen), dis(gen), dis(gen), dis(gen));
                }

                cl_mem buffer = create_buffer(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                              work_items * sizeof(float4), data.data());
                check_cl_error(clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer),
                               "clSetKernelArg");

                size_t global_size = work_items;
                size_t local_size = 256;
                while (global_size % local_size != 0 && local_size > 1)
                    local_size /= 2;

                // Прогрев
                clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size,
                                       &local_size, 0, NULL, NULL);
                clFinish(queue);

                // Основное измерение
                const int iterations = 5;
                auto start = std::chrono::high_resolution_clock::now();

                for (int i = 0; i < iterations; i++)
                {
                    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size,
                                           &local_size, 0, NULL, NULL);
                }
                clFinish(queue);

                auto end = std::chrono::high_resolution_clock::now();
                double time_seconds = std::chrono::duration<double>(end - start).count();

                // Точный подсчет операций с плавающей точкой
                // За итерацию: 2 mad + 3 арифметики + 4 sin/cos = ~9 операций на компонент
                // 64 итерации × 4 компонента × 9 операций = 2304 операций на рабочий элемент
                double total_flops = work_items * 2304 * iterations;
                double gflops = total_flops / (time_seconds * 1e9); // GFLOPS

                // Валидация результата
                bool validated = gpu_execution_validated;
                if (is_real_gpu && gflops < 50.0) // Нереалистично для реального GPU
                {
                    validated = false;
                }

                add_result("Compute Performance", gflops, "GFLOPS", validated);

                // Освобождение ресурсов
                clReleaseMemObject(buffer);
                clReleaseKernel(kernel);
                clReleaseProgram(program);
            }
            catch (const std::exception &e)
            {
                add_result("Compute Performance", 0, "GFLOPS", false);
            }
        }

    public:
        // Конструктор - инициализация OpenCL
        UniversalGPUTester() : context(nullptr), device(nullptr), queue(nullptr),
                               opencl_available(false), is_real_gpu(false),
                               max_work_group_size(0), compute_units(0),
                               global_mem_size(0), clock_frequency(0),
                               gpu_execution_validated(false), gpu_validation_score(0)
        {
            opencl_available = init_opencl();
        }

        // Деструктор - освобождение ресурсов OpenCL
        ~UniversalGPUTester()
        {
            if (queue)
                clReleaseCommandQueue(queue);
            if (context)
                clReleaseContext(context);
        }

        // Методы доступа
        bool is_available() const { return opencl_available; }
        bool is_gpu() const { return is_real_gpu; }
        bool is_validated() const { return gpu_execution_validated; }

        // Запуск комплексного тестирования
        void run_comprehensive_test()
        {
            if (!opencl_available)
            {
                std::cout << "OpenCL not available\n";
                return;
            }

            // Вывод информации об устройстве
            std::cout << "\n=== UNIVERSAL GPU BENCHMARK ===\n";
            std::cout << "Device: " << device_name << "\n";
            std::cout << "Vendor: " << device_vendor << "\n";
            std::cout << "Type: " << (is_real_gpu ? "GPU" : "CPU") << "\n";
            std::cout << "GPU Validated: " << (gpu_execution_validated ? "YES" : "NO") << "\n";

            if (is_real_gpu && !gpu_execution_validated)
            {
                std::cout << "WARNING: GPU detected but execution not validated!\n";
                std::cout << "Validation Score: " << gpu_validation_score << " GFLOPS\n";
            }

            std::cout << "===============================\n\n";

            results.clear();

            // Запуск тестов
            test_memory_bandwidth_real();
            test_compute_performance_real();

            // Отображение результатов в таблице
            std::cout << "\n"
                      << std::string(70, '=') << "\n";
            std::cout << "BENCHMARK RESULTS\n";
            std::cout << std::string(70, '=') << "\n\n";

            std::cout << std::left << std::setw(25) << "TEST"
                      << std::setw(12) << "SCORE"
                      << std::setw(8) << "UNITS"
                      << std::setw(15) << "RATING"
                      << std::setw(10) << "STATUS\n";
            std::cout << std::string(70, '-') << "\n";

            for (const auto &result : results)
            {
                std::cout << std::left << std::setw(25) << result.test_name
                          << std::right << std::setw(10) << std::fixed << std::setprecision(2)
                          << result.score << " " << std::setw(6) << result.unit
                          << " [" << std::setw(12) << result.rating << "] "
                          << (result.validated ? "+" : "!!") << "\n";
            }

            std::cout << std::string(70, '=') << "\n";

            // Сводка
            if (!gpu_execution_validated && is_real_gpu)
            {
                std::cout << "\n   WARNING: Results may not reflect actual GPU performance!\n";
                std::cout << "   Possible reasons:\n";
                std::cout << "   1. Fallback to CPU execution\n";
                std::cout << "   2. Driver emulation\n";
                std::cout << "   3. Power/thermal throttling\n";
            }
            else if (gpu_execution_validated)
            {
                std::cout << "\n   GPU execution validated\n";
            }
        }
    };

    // Внешняя функция для запуска теста
    inline void run_universal_gpu_test()
    {
        try
        {
            UniversalGPUTester tester;
            if (tester.is_available())
            {
                tester.run_comprehensive_test();

                // Финальное предупреждение если необходимо
                if (tester.is_gpu() && !tester.is_validated())
                {
                    std::cout << "\n  IMPORTANT: This test may be running on CPU!\n";
                    std::cout << "   For accurate GPU benchmarking, ensure:\n";
                    std::cout << "   1. Latest GPU drivers installed\n";
                    std::cout << "   2. No power saving modes\n";
                    std::cout << "   3. Discrete GPU selected in system settings\n";
                }
            }
            else
            {
                std::cout << "OpenCL not available. Install GPU drivers.\n";
            }
        }
        catch (const std::exception &e)
        {
            std::cout << "Benchmark error: " << e.what() << "\n";
        }
    }

} // namespace gpu_benchmark_final