#pragma once
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath>
#include <random>
#include <algorithm>
#include <iomanip>

// OpenCL includes
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/opencl.h>

namespace gpu_benchmark
{

    class GPUTester
    {
    private:
        cl_context context;
        cl_device_id device;
        cl_command_queue queue;
        bool opencl_available;
        std::string device_name;
        bool is_gpu_device;

        struct TestResult
        {
            std::string test_name;
            double score;
            std::string unit;
            std::string rating;
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
                std::cout << "OpenCL not found\n";
                return false;
            }

            // First try to find GPU
            ret = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
            if (ret == CL_SUCCESS)
            {
                is_gpu_device = true;
                std::cout << "Found GPU device\n";
            }
            else
            {
                // If GPU not found, use CPU
                std::cout << "GPU not found, using CPU\n";
                ret = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL);
                if (ret != CL_SUCCESS)
                {
                    std::cout << "No OpenCL devices found\n";
                    return false;
                }
                is_gpu_device = false;
            }

            // Get detailed device information
            char name[128];
            cl_device_type device_type;

            clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(name), name, NULL);
            clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(device_type), &device_type, NULL);

            device_name = name;

            // Check device type
            std::cout << "Device type: ";
            if (device_type & CL_DEVICE_TYPE_GPU)
            {
                std::cout << "GPU";
                is_gpu_device = true;
            }
            else if (device_type & CL_DEVICE_TYPE_CPU)
            {
                std::cout << "CPU";
                is_gpu_device = false;
            }
            else
            {
                std::cout << "Other";
                is_gpu_device = false;
            }
            std::cout << "\n";

            context = clCreateContext(NULL, 1, &device, NULL, NULL, &ret);
            if (ret != CL_SUCCESS)
            {
                std::cout << "Error creating context\n";
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
            std::cout << "Using: " << (is_gpu_device ? "GPU" : "CPU") << "\n";

            return true;
        }

        void add_result(const std::string &name, double score, const std::string &unit, const std::string &rating)
        {
            results.push_back({name, score, unit, rating});
        }

        std::string get_rating(double score, const std::string &test_type)
        {
            if (test_type == "memory")
            {
                if (is_gpu_device)
                {
                    // Standards for GPU
                    if (score > 200)
                        return "EXCELLENT";
                    if (score > 150)
                        return "VERY GOOD";
                    if (score > 100)
                        return "GOOD";
                    if (score > 50)
                        return "AVERAGE";
                    return "LOW";
                }
                else
                {
                    // Standards for CPU
                    if (score > 40)
                        return "EXCELLENT";
                    if (score > 25)
                        return "VERY GOOD";
                    if (score > 15)
                        return "GOOD";
                    if (score > 8)
                        return "AVERAGE";
                    return "LOW";
                }
            }
            else
            { // compute
                if (is_gpu_device)
                {
                    // Standards for GPU
                    if (score > 3000)
                        return "EXCELLENT";
                    if (score > 2000)
                        return "VERY GOOD";
                    if (score > 1000)
                        return "GOOD";
                    if (score > 500)
                        return "AVERAGE";
                    return "LOW";
                }
                else
                {
                    // Standards for CPU
                    if (score > 200)
                        return "EXCELLENT";
                    if (score > 100)
                        return "VERY GOOD";
                    if (score > 50)
                        return "GOOD";
                    if (score > 20)
                        return "AVERAGE";
                    return "LOW";
                }
            }
        }

        void print_device_info()
        {
            if (!opencl_available)
                return;

            cl_uint compute_units;
            cl_ulong global_mem;
            cl_ulong local_mem;
            size_t max_work_group;
            cl_uint clock_freq;

            clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);
            clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(global_mem), &global_mem, NULL);
            clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(local_mem), &local_mem, NULL);
            clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_work_group), &max_work_group, NULL);
            clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_freq), &clock_freq, NULL);

            std::cout << "\n=== DEVICE INFORMATION ===\n";
            std::cout << "Name: " << device_name << "\n";
            std::cout << "Type: " << (is_gpu_device ? "Graphics Card (GPU)" : "Processor (CPU)") << "\n";
            std::cout << "Compute Units: " << compute_units << "\n";
            std::cout << "Memory: " << global_mem / (1024 * 1024) << " MB\n";
            std::cout << "Local Memory: " << local_mem / 1024 << " KB\n";
            std::cout << "Max Work Group Size: " << max_work_group << "\n";
            std::cout << "Clock Frequency: " << clock_freq << " MHz\n";
        }

    public:
        GPUTester() : context(nullptr), device(nullptr), queue(nullptr),
                      opencl_available(false), is_gpu_device(false)
        {
            std::cout << "Initializing GPU Test...\n";
            opencl_available = init_opencl();
            if (opencl_available)
            {
                print_device_info();
            }
        }

        ~GPUTester()
        {
            if (queue)
                clReleaseCommandQueue(queue);
            if (context)
                clReleaseContext(context);
        }

        bool is_using_gpu() const
        {
            return is_gpu_device;
        }

        void test_memory_speed()
        {
            if (!opencl_available)
            {
                std::cout << "OpenCL not available\n";
                return;
            }

            std::cout << "\n=== MEMORY SPEED TEST ===\n";
            std::cout << "Using: " << (is_gpu_device ? "GPU Memory" : "CPU Memory") << "\n";

            const size_t memory_size = is_gpu_device ? 128 * 1024 * 1024 : 64 * 1024 * 1024;
            const int iterations = is_gpu_device ? 30 : 20;
            cl_int ret;

            // Simple memory copy kernel
            const char *kernel_code =
                "__kernel void copy_test(__global float* input, __global float* output) {\n"
                "    int id = get_global_id(0);\n"
                "    output[id] = input[id] * 2.0f;\n"
                "}\n";

            cl_program program = clCreateProgramWithSource(context, 1, &kernel_code, NULL, &ret);
            ret = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
            cl_kernel kernel = clCreateKernel(program, "copy_test", &ret);

            size_t element_count = memory_size / sizeof(float);
            std::vector<float> input_data(element_count);
            std::vector<float> output_data(element_count);

            for (size_t i = 0; i < element_count; i++)
            {
                input_data[i] = static_cast<float>(i);
            }

            cl_mem input_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                 memory_size, input_data.data(), &ret);
            cl_mem output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, memory_size, NULL, &ret);

            ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buffer);
            ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_buffer);

            size_t global_size = element_count;
            size_t local_size = is_gpu_device ? 256 : 64;

            auto start = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < iterations; i++)
            {
                clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
            }

            clFinish(queue);
            auto end = std::chrono::high_resolution_clock::now();

            double time_seconds = std::chrono::duration<double>(end - start).count();
            double bandwidth = (memory_size * 2 * iterations) / (time_seconds * 1024 * 1024 * 1024);

            std::cout << "Memory Speed: " << bandwidth << " GB/s\n";
            std::cout << "Execution Time: " << time_seconds << " seconds\n";
            std::cout << "Data Processed: " << (memory_size * iterations * 2) / (1024 * 1024 * 1024) << " GB\n";

            add_result("Memory Speed", bandwidth, "GB/s", get_rating(bandwidth, "memory"));

            clReleaseMemObject(input_buffer);
            clReleaseMemObject(output_buffer);
            clReleaseKernel(kernel);
            clReleaseProgram(program);
        }

        void test_compute_performance()
        {
            if (!opencl_available)
            {
                std::cout << "OpenCL not available\n";
                return;
            }

            std::cout << "\n=== COMPUTE PERFORMANCE TEST ===\n";
            std::cout << "Using: " << (is_gpu_device ? "GPU" : "CPU") << "\n";

            const size_t data_size = is_gpu_device ? 2 * 1024 * 1024 : 1 * 1024 * 1024;
            const int iterations = is_gpu_device ? 15 : 10;
            cl_int ret;

            // Math computation kernel
            const char *kernel_code =
                "__kernel void math_test(__global float* input, __global float* output) {\n"
                "    int id = get_global_id(0);\n"
                "    float x = input[id];\n"
                "    \n"
                "    // Complex math operations\n"
                "    float result = sin(x) * cos(x);\n"
                "    result += sqrt(fabs(x));\n"
                "    result *= log(x + 1.0f);\n"
                "    result = exp(result) - pow(x, 2.0f);\n"
                "    \n"
                "    output[id] = result;\n"
                "}\n";

            cl_program program = clCreateProgramWithSource(context, 1, &kernel_code, NULL, &ret);
            ret = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
            cl_kernel kernel = clCreateKernel(program, "math_test", &ret);

            std::vector<float> input_data(data_size);
            std::vector<float> output_data(data_size);

            for (size_t i = 0; i < data_size; i++)
            {
                input_data[i] = static_cast<float>(i) * 0.001f;
            }

            cl_mem input_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                 data_size * sizeof(float), input_data.data(), &ret);
            cl_mem output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                                  data_size * sizeof(float), NULL, &ret);

            ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buffer);
            ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_buffer);

            size_t global_size = data_size;
            size_t local_size = is_gpu_device ? 256 : 128;

            auto start = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < iterations; i++)
            {
                clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
            }

            clFinish(queue);
            auto end = std::chrono::high_resolution_clock::now();

            double time_seconds = std::chrono::duration<double>(end - start).count();
            double operations = data_size * iterations * 8;
            double gflops = operations / (time_seconds * 1e9);

            std::cout << "Compute Performance: " << gflops << " GFLOPS\n";
            std::cout << "Execution Time: " << time_seconds << " seconds\n";

            add_result("Compute Performance", gflops, "GFLOPS", get_rating(gflops, "compute"));

            clReleaseMemObject(input_buffer);
            clReleaseMemObject(output_buffer);
            clReleaseKernel(kernel);
            clReleaseProgram(program);
        }

        void run_complete_test()
        {
            if (!opencl_available)
            {
                std::cout << "Testing not available - OpenCL not accessible\n";
                return;
            }

            std::cout << "\nSTARTING COMPLETE PERFORMANCE TEST\n";
            std::cout << "===================================\n";
            std::cout << "Testing: " << (is_gpu_device ? "GRAPHICS CARD (GPU)" : "PROCESSOR (CPU)") << "\n\n";

            results.clear();

            test_memory_speed();
            test_compute_performance();

            // Display results
            std::cout << "\n"
                      << std::string(50, '=') << "\n";
            std::cout << "TEST RESULTS\n";
            std::cout << std::string(50, '=') << "\n";
            std::cout << "Device: " << (is_gpu_device ? "GPU" : "CPU") << "\n\n";

            for (const auto &result : results)
            {
                std::cout << std::left << std::setw(30) << result.test_name
                          << std::right << std::setw(10) << std::fixed << std::setprecision(2)
                          << result.score << " " << result.unit
                          << " [" << result.rating << "]\n";
            }
        }
    };

    // Menu
    namespace menu
    {
        inline void show_gpu_menu()
        {
            std::cout << "\n=== GPU PERFORMANCE TEST ===\n";
            std::cout << "1. Complete GPU Test\n";
            std::cout << "2. Memory Speed Test\n";
            std::cout << "3. Compute Performance Test\n";
            std::cout << "4. Back to Main Menu\n";
            std::cout << "Choose option: ";
        }

        inline int get_choice()
        {
            int choice;
            std::cin >> choice;
            if (std::cin.fail())
            {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                return -1;
            }
            std::cin.ignore(10000, '\n');
            return choice;
        }
    }

    // Public functions
    inline void run_full_gpu_test()
    {
        GPUTester tester;
        if (tester.is_using_gpu())
        {
            std::cout << "\nTesting on GRAPHICS CARD\n";
        }
        else
        {
            std::cout << "\nTesting on PROCESSOR (GPU not found)\n";
        }
        tester.run_complete_test();
    }

    inline void run_gpu_test_menu()
    {
        int choice;
        do
        {
            menu::show_gpu_menu();
            choice = menu::get_choice();

            GPUTester tester;

            switch (choice)
            {
            case 1:
                std::cout << "\n";
                if (tester.is_using_gpu())
                {
                    std::cout << "Testing GRAPHICS CARD\n";
                }
                else
                {
                    std::cout << "Testing PROCESSOR (GPU not found)\n";
                }
                tester.run_complete_test();
                break;
            case 2:
                std::cout << "\n";
                tester.test_memory_speed();
                break;
            case 3:
                std::cout << "\n";
                tester.test_compute_performance();
                break;
            case 4:
                std::cout << "Returning to main menu...\n";
                break;
            default:
                std::cout << "Invalid choice!\n";
                break;
            }
            std::cout << "\n";
        } while (choice != 4);
    }

} // namespace gpu_benchmark