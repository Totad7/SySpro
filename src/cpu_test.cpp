#include "../include/cpu_test.h"
#include "../include/cpu_benchmark.h"
#include <iostream>
#include <windows.h>
#include <atomic>
using namespace std;

static atomic<bool> g_interrupted(false);

BOOL WINAPI ConsoleHandler(DWORD signal)
{
    if (signal == CTRL_C_EVENT)
    {
        g_interrupted.store(true);
        cout << "\n\n*** Test interrupted by user ***\n";
        return TRUE;
    }
    return FALSE;
}

void show_cpu_test_menu()
{
    cout << "\n"
         << string(50, '=') << "\n";
    cout << "CPU STRESS TEST MODULE\n";
    cout << string(50, '=') << "\n";
    cout << "WARNING: This will fully load your CPU!\n";
    cout << "Make sure cooling is adequate.\n\n";
    cout << "1. Quick test (10 seconds)\n";
    cout << "2. Standard test (30 seconds)\n";
    cout << "3. Extended test (60 seconds)\n";
    cout << "4. Custom duration\n";
    cout << "5. Back to main menu\n";
    cout << "Choose option: ";
}

void run_cpu_stress_test()
{
    // Настройка обработчика прерываний
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    CPUBenchmark benchmark;
    int choice;

    cout << "=== CPU Maximum Load Test ===\n";
    cout << "This test will push your CPU to 100% load\n";
    cout << "to measure real performance capabilities.\n";

    do
    {
        show_cpu_test_menu();
        cin >> choice;

        if (cin.fail())
        {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Invalid input. Please enter a number 1-5.\n";
            continue;
        }

        cin.ignore(10000, '\n');

        int duration = 10;

        switch (choice)
        {
        case 1:
            duration = 10;
            break;
        case 2:
            duration = 30;
            break;
        case 3:
            duration = 60;
            break;
        case 4:
            cout << "Enter test duration in seconds: ";
            cin >> duration;
            cin.ignore(10000, '\n');
            if (duration < 5)
            {
                cout << "Minimum duration is 5 seconds. Using 5 seconds.\n";
                duration = 5;
            }
            else if (duration > 300000)
            {
                cout << "Maximum duration is 300000 seconds. Using 300000 seconds.\n";
                duration = 300000;
            }
            break;
        case 5:
            cout << "Returning to main menu...\n";
            return;
        default:
            cout << "Invalid choice! Please select 1-5.\n";
            continue;
        }

        if (choice >= 1 && choice <= 4)
        {
            cout << "\nStarting maximum load test for " << duration << " seconds...\n";
            cout << "Press Ctrl+C to stop test early.\n";
            cout << "CPU will run at 100% load during test.\n\n";

            try
            {
                auto result = benchmark.run_benchmark(duration);
                benchmark.print_detailed_report(result);
            }
            catch (const exception &e)
            {
                cerr << "Test error: " << e.what() << endl;
            }

            cout << "\nTest completed. Press Enter to continue...";
            cin.get();
        }

    } while (choice != 5);
}