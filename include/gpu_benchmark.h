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

// OpenCL includes
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/opencl.h>

namespace gpu_benchmark_final
{

    // Float4 structure for SIMD operations
    struct float4
    {
        float x, y, z, w;
        float4(float x = 0, float y = 0, float z = 0, float w = 0) : x(x), y(y), z(z), w(w) {}
    };

    class UniversalGPUTester
    {
    private:
        cl_context context;
        cl_device_id device;
        cl_command_queue queue;
        bool opencl_available;
        std::string device_name;
        bool is_gpu_device;
        size_t max_work_group_size;
        size_t compute_units;
        cl_ulong global_mem_size;
        cl_uint clock_frequency;

        struct TestResult
        {
            std::string test_name;
            double score;
            std::string unit;
            std::string rating;
            double theoretical_max;
            double percentage;
        };

        std::vector<TestResult> results;

        bool init_opencl()
        {
            cl_platform_id platform;
            cl_int ret;

            std::cout << "Searching for OpenCL devices...\n";
            ret = clGetPlatformIDs(1, &platform, NULL);
            if (ret != CL_SUCCESS)
            {
                std::cout << "OpenCL not available\n";
                return false;
            }

            // Try GPU first, then CPU
            ret = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
            if (ret == CL_SUCCESS)
            {
                is_gpu_device = true;
                std::cout << "GPU device detected\n";
            }
            else
            {
                std::cout << "Using CPU for computation\n";
                ret = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL);
                if (ret != CL_SUCCESS)
                {
                    std::cout << "No OpenCL devices found\n";
                    return false;
                }
                is_gpu_device = false;
            }

            // Get device information
            char name[128];
            clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(name), name, NULL);
            device_name = name;

            // Get hardware specifications
            clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);
            clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_work_group_size), &max_work_group_size, NULL);
            clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(global_mem_size), &global_mem_size, NULL);
            clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_frequency), &clock_frequency, NULL);

            // Create OpenCL context and queue
            context = clCreateContext(NULL, 1, &device, NULL, NULL, &ret);
            if (ret != CL_SUCCESS)
            {
                std::cout << "Error creating OpenCL context\n";
                return false;
            }

            queue = clCreateCommandQueue(context, device, 0, &ret);
            if (ret != CL_SUCCESS)
            {
                std::cout << "Error creating command queue\n";
                clReleaseContext(context);
                return false;
            }

            std::cout << "Device: " << device_name << "\n";
            std::cout << "Type: " << (is_gpu_device ? "GPU" : "CPU") << "\n";

            return true;
        }

        void add_result(const std::string &name, double score, const std::string &unit,
                        double theoretical = 0)
        {
            double percentage = theoretical > 0 ? (score / theoretical) * 100 : 0;
            std::string rating;

            // Universal rating system based on percentage of theoretical maximum
            if (percentage > 0)
            {
                if (percentage > 80)
                    rating = "EXCELLENT";
                else if (percentage > 60)
                    rating = "VERY GOOD";
                else if (percentage > 40)
                    rating = "GOOD";
                else if (percentage > 20)
                    rating = "AVERAGE";
                else
                    rating = "LOW";
            }
            else
            {
                // Fallback absolute ratings
                if (name.find("Memory") != std::string::npos)
                {
                    if (score > 300)
                        rating = "EXCELLENT";
                    else if (score > 200)
                        rating = "VERY GOOD";
                    else if (score > 100)
                        rating = "GOOD";
                    else if (score > 50)
                        rating = "AVERAGE";
                    else
                        rating = "LOW";
                }
                else
                {
                    if (score > 3000)
                        rating = "EXCELLENT";
                    else if (score > 2000)
                        rating = "VERY GOOD";
                    else if (score > 1000)
                        rating = "GOOD";
                    else if (score > 500)
                        rating = "AVERAGE";
                    else
                        rating = "LOW";
                }
            }

            results.push_back({name, score, unit, rating, theoretical, percentage});
        }

        void print_device_info()
        {
            if (!opencl_available)
                return;

            cl_ulong local_mem;
            cl_uint vector_width;

            clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(local_mem), &local_mem, NULL);
            clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, sizeof(vector_width), &vector_width, NULL);

            std::cout << "\n=== DEVICE SPECIFICATIONS ===\n";
            std::cout << "Name: " << device_name << "\n";
            std::cout << "Type: " << (is_gpu_device ? "Graphics Card (GPU)" : "Processor (CPU)") << "\n";
            std::cout << "Compute Units: " << compute_units << "\n";
            std::cout << "Memory: " << global_mem_size / (1024 * 1024) << " MB\n";
            std::cout << "Local Memory: " << local_mem / 1024 << " KB\n";
            std::cout << "Max Work Group: " << max_work_group_size << "\n";
            std::cout << "Clock Frequency: " << clock_frequency << " MHz\n";
            std::cout << "Float Vector Width: " << vector_width << "\n";

            // Universal theoretical performance calculation
            if (is_gpu_device)
            {
                double theoretical_gflops = compute_units * 64.0 * 2.0 * (clock_frequency / 1000.0);
                double theoretical_bandwidth = (global_mem_size / (1024.0 * 1024 * 1024)) * 6.0;

                std::cout << "Theoretical GFLOPS: ~" << theoretical_gflops << " GFLOPS\n";
                std::cout << "Theoretical Bandwidth: ~" << theoretical_bandwidth << " GB/s\n";
            }
        }

        // Universal theoretical performance calculation
        double calculate_theoretical_gflops()
        {
            if (!is_gpu_device)
                return 0;

            // Universal calculation for any GPU architecture
            return compute_units * 64.0 * 2.0 * (clock_frequency / 1000.0);
        }

        double calculate_theoretical_bandwidth()
        {
            if (!is_gpu_device)
                return 0;

            // Universal bandwidth estimation
            return (global_mem_size / (1024.0 * 1024 * 1024)) * 6.0;
        }

        void test_memory_bandwidth_universal()
        {
            if (!opencl_available)
                return;

            std::cout << "\n=== MEMORY BANDWIDTH TEST ===\n";
            std::cout << "Testing: " << (is_gpu_device ? "GPU Memory" : "CPU Memory") << "\n";

            // Adaptive memory size for any device
            size_t test_memory = is_gpu_device ? std::min(global_mem_size / 4, static_cast<cl_ulong>(512 * 1024 * 1024)) : 128 * 1024 * 1024;

            size_t element_count = test_memory / sizeof(float);
            const int iterations = 10;

            // Universal memory bandwidth kernel
            const char *kernel_code =
                "__kernel void bandwidth_test(__global float4* input, __global float4* output) {\n"
                "    int gid = get_global_id(0);\n"
                "    int total = get_global_size(0);\n"
                "    \n"
                "    // Multiple memory accesses to test bandwidth\n"
                "    float4 data1 = input[gid];\n"
                "    float4 data2 = input[(gid + 1) % total];\n"
                "    float4 data3 = input[(gid + 2) % total];\n"
                "    float4 data4 = input[(gid + 3) % total];\n"
                "    \n"
                "    // Memory-intensive operations\n"
                "    output[gid] = data1 * 2.0f + data2 * 1.5f - data3 * 0.5f + data4 * 3.0f;\n"
                "}\n";

            cl_int ret;
            cl_program program = clCreateProgramWithSource(context, 1, &kernel_code, NULL, &ret);
            ret = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
            cl_kernel kernel = clCreateKernel(program, "bandwidth_test", &ret);

            size_t float4_count = element_count / 4;
            std::vector<float4> input_data(float4_count);
            std::vector<float4> output_data(float4_count);

            // Initialize test data
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> dis(1.0f, 100.0f);

            for (size_t i = 0; i < float4_count; i++)
            {
                input_data[i] = float4(dis(gen), dis(gen), dis(gen), dis(gen));
            }

            cl_mem input_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                 float4_count * sizeof(float4), input_data.data(), &ret);
            cl_mem output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                                  float4_count * sizeof(float4), NULL, &ret);

            ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buffer);
            ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_buffer);

            size_t global_size = float4_count;
            size_t local_size = std::min(max_work_group_size, static_cast<size_t>(256));

            // Warm-up execution
            clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
            clFinish(queue);

            auto start = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < iterations; i++)
            {
                clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
            }

            clFinish(queue);
            auto end = std::chrono::high_resolution_clock::now();

            double time_seconds = std::chrono::duration<double>(end - start).count();

            // Bandwidth calculation: 4 reads + 1 write per work item
            double bytes_processed = float4_count * 5 * sizeof(float4) * iterations;
            double bandwidth = bytes_processed / (time_seconds * 1024 * 1024 * 1024);

            std::cout << "Memory Speed: " << std::fixed << std::setprecision(2) << bandwidth << " GB/s\n";
            std::cout << "Execution Time: " << time_seconds << " seconds\n";
            std::cout << "Data Processed: " << bytes_processed / (1024 * 1024 * 1024) << " GB\n";
            std::cout << "Work Items: " << global_size << "\n";

            double theoretical_bw = calculate_theoretical_bandwidth();
            add_result("Memory Bandwidth", bandwidth, "GB/s", theoretical_bw);

            clReleaseMemObject(input_buffer);
            clReleaseMemObject(output_buffer);
            clReleaseKernel(kernel);
            clReleaseProgram(program);
        }

        void test_compute_performance_universal()
        {
            if (!opencl_available)
                return;

            std::cout << "\n=== COMPUTE PERFORMANCE TEST ===\n";
            std::cout << "Testing: " << (is_gpu_device ? "GPU" : "CPU") << "\n";

            // Adaptive workload for any device
            size_t work_items = compute_units * max_work_group_size * 64;
            if (is_gpu_device)
            {
                work_items = std::max(work_items, static_cast<size_t>(1024 * 1024));
                work_items = std::min(work_items, static_cast<size_t>(4 * 1024 * 1024));
            }
            else
            {
                work_items = std::min(work_items, static_cast<size_t>(512 * 1024));
            }

            const int iterations = 5;

            // Universal compute kernel
            const char *kernel_code =
                "__kernel void compute_test(__global float4* input, __global float4* output) {\n"
                "    int gid = get_global_id(0);\n"
                "    float4 x = input[gid];\n"
                "    \n"
                "    // Mixed computational operations\n"
                "    float4 a = sin(x) * 2.0f;\n"
                "    float4 b = cos(x) * 3.0f;\n"
                "    float4 c = sqrt(x) * 1.5f;\n"
                "    float4 d = log(x + 1.0f) * 4.0f;\n"
                "    \n"
                "    // Combine operations\n"
                "    float4 result = a + b + c + d;\n"
                "    result = mad(result, x, a);\n"
                "    \n"
                "    output[gid] = result;\n"
                "}\n";

            cl_int ret;
            cl_program program = clCreateProgramWithSource(context, 1, &kernel_code, NULL, &ret);
            ret = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
            cl_kernel kernel = clCreateKernel(program, "compute_test", &ret);

            std::vector<float4> input_data(work_items);
            std::vector<float4> output_data(work_items);

            // Initialize test data
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> dis(0.1f, 5.0f);

            for (size_t i = 0; i < work_items; i++)
            {
                input_data[i] = float4(dis(gen), dis(gen), dis(gen), dis(gen));
            }

            cl_mem input_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                 work_items * sizeof(float4), input_data.data(), &ret);
            cl_mem output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                                  work_items * sizeof(float4), NULL, &ret);

            ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buffer);
            ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_buffer);

            size_t global_size = work_items;
            size_t local_size = std::min(max_work_group_size, static_cast<size_t>(256));

            // Warm-up
            clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
            clFinish(queue);

            auto start = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < iterations; i++)
            {
                clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
            }

            clFinish(queue);
            auto end = std::chrono::high_resolution_clock::now();

            double time_seconds = std::chrono::duration<double>(end - start).count();

            // FLOPS calculation: 4 operations × 4 components × 2 FLOPS each
            double flops_per_workitem = 4 * 4 * 2; // 32 FLOPS per work item
            double total_flops = work_items * flops_per_workitem * iterations;
            double gflops = total_flops / (time_seconds * 1e9);

            std::cout << "Compute Performance: " << gflops << " GFLOPS\n";
            std::cout << "Execution Time: " << time_seconds << " seconds\n";
            std::cout << "Work Items: " << work_items << " (scaled)\n";
            std::cout << "Total Operations: " << total_flops / 1e9 << " GOperations\n";

            double theoretical_gflops = calculate_theoretical_gflops();
            add_result("Compute Performance", gflops, "GFLOPS", theoretical_gflops);

            clReleaseMemObject(input_buffer);
            clReleaseMemObject(output_buffer);
            clReleaseKernel(kernel);
            clReleaseProgram(program);
        }

    public:
        UniversalGPUTester() : context(nullptr), device(nullptr), queue(nullptr),
                               opencl_available(false), is_gpu_device(false),
                               max_work_group_size(0), compute_units(0),
                               global_mem_size(0), clock_frequency(0)
        {
            std::cout << "Initializing Universal GPU Benchmark...\n";
            opencl_available = init_opencl();
            if (opencl_available)
            {
                print_device_info();
            }
        }

        ~UniversalGPUTester()
        {
            if (queue)
                clReleaseCommandQueue(queue);
            if (context)
                clReleaseContext(context);
        }

        bool is_available() const { return opencl_available; }
        bool is_gpu() const { return is_gpu_device; }

        void run_comprehensive_test()
        {
            if (!opencl_available)
            {
                std::cout << "Benchmark not available - OpenCL required\n";
                return;
            }

            std::cout << "\nSTARTING UNIVERSAL PERFORMANCE BENCHMARK\n";
            std::cout << "==========================================\n";
            std::cout << "Device: " << (is_gpu_device ? "GRAPHICS CARD (GPU)" : "PROCESSOR (CPU)") << "\n\n";

            results.clear();

            test_memory_bandwidth_universal();
            test_compute_performance_universal();

            // Display results
            std::cout << "\n"
                      << std::string(70, '=') << "\n";
            std::cout << "BENCHMARK RESULTS\n";
            std::cout << std::string(70, '=') << "\n";
            std::cout << "Device: " << device_name << "\n";
            std::cout << "Type: " << (is_gpu_device ? "GPU" : "CPU") << "\n\n";

            std::cout << std::left << std::setw(25) << "TEST"
                      << std::setw(12) << "SCORE"
                      << std::setw(8) << "UNITS"
                      << std::setw(12) << "RATING"
                      << "THEORETICAL\n";
            std::cout << std::string(70, '-') << "\n";

            for (const auto &result : results)
            {
                std::cout << std::left << std::setw(25) << result.test_name
                          << std::right << std::setw(10) << std::fixed << std::setprecision(2)
                          << result.score << " " << std::setw(6) << result.unit
                          << " [" << std::setw(10) << result.rating << "]";

                if (result.theoretical_max > 0)
                {
                    std::cout << " (" << std::fixed << std::setprecision(1) << result.percentage << "%)";
                }
                std::cout << "\n";
            }
        }
    };

    // Public interface
    inline void run_universal_gpu_test()
    {
        UniversalGPUTester tester;
        if (tester.is_available())
        {
            tester.run_comprehensive_test();
        }
        else
        {
            std::cout << "GPU benchmarking not available on this system\n";
        }
    }

} // namespace gpu_benchmark_final