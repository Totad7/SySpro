#include "cpu_test.h"
#include "gpu_benchmark.h"
#include "ram_test.h"
#include "ssd_test.h"
#include "system_info.h"
#include <iostream>
using namespace std;

void show_main_menu()
{
    cout << "\n=== PC DIAGNOSTIC TOOL ===\n";
    cout << "1. System Info\n";
    cout << "2. Memory Test (RAM)\n";
    cout << "3. CPU Stress Test\n";
    cout << "4. Disk Speed Test (SSD/HDD)\n";
    cout << "5. GPU Benchmark Test\n";
    cout << "6. Exit\n";
    cout << "Choose option: ";
}

int main()
{
    cout << " PC Diagnostic Tool - Comprehensive System Testing\n";
    cout << "===================================================\n";

    while (true)
    {
        show_main_menu();
        int nomercomand;
        cin >> nomercomand;

        switch (nomercomand)
        {
        case 1:
            cout << "\n=== SYSTEM INFORMATION ===\n";
            system_info::GetSystemInfo();
            break;
        case 2:
            cout << "\n=== MEMORY TEST (RAM) ===\n";
            ram_test::run_test();
            break;
        case 3:
            cout << "\n=== CPU STRESS TEST ===\n";
            run_cpu_stress_test();
            break;
        case 4:
            cout << "\n=== DISK SPEED TEST ===\n";
            ssd_test::run_ssd_test();
            break;
        case 5:
            cout << "\n=== GPU BENCHMARK TEST ===\n";
            gpu_benchmark::run_gpu_test_menu();
            break;
        case 6:
            cout << "Exiting... Goodbye!\n";
            return 0;
        default:
            cout << "Error: Invalid command!\n";
            cin.clear();
            cin.ignore(10000, '\n');
            break;
        }
    }
}