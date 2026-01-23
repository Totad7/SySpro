#include "3DTD.h"
#include "cpu_test.h"
#include "gpu_benchmark.h"
#include "ram_test.h"
#include "ssd_test.h"
#include "system_info.h"
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <conio.h>
#include <string>
#include <thread>
#include <chrono>

using namespace std;

// Функции для работы с консолью
void setColor(int color)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void gotoxy(int x, int y)
{
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void clearScreen()
{
    system("cls");
}

void printCentered(const string &text, int y)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int x = (width - text.length()) / 2;
    gotoxy(x, y);
    cout << text;
}

void showHeader()
{
    clearScreen();

    // Установка заголовка окна
    SetConsoleTitle("SySpro v1.8");

    // Верхний баннер
    setColor(14); // Желтый
    cout << "\n";
    cout << "    **************************************************\n";
    cout << "    *                  SySpro v1.8                   *\n";
    cout << "    *              PC DIAGNOSTIC TOOL                *\n";
    cout << "    *         Comprehensive System Testing           *\n";
    cout << "    **************************************************\n";
    setColor(7); // Белый
}

void toggleLibreHardwareMonitor()
{
    clearScreen();
    showHeader();

    cout << "\n";
    setColor(11);
    cout << "    ===== LibreHardwareMonitor CONTROL =====\n\n";
    cout << "\n!!  If LibreHardwareMonitor doesn't start, run the application as an administrator.  !!\n";
    setColor(7);

    // Проверяем, запущен ли LibreHardwareMonitor
    bool isRunning = false;
    DWORD processId = 0;

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        if (Process32First(hSnapshot, &pe32))
        {
            do
            {
                if (_stricmp(pe32.szExeFile, "LibreHardwareMonitor.exe") == 0)
                {
                    isRunning = true;
                    processId = pe32.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }

    if (isRunning)
    {
        // Завершаем процесс
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
        if (hProcess)
        {
            TerminateProcess(hProcess, 0);
            CloseHandle(hProcess);
            setColor(10);
            cout << "  LibreHardwareMonitor terminated.\n";
            setColor(7);
        }
    }
    else
    {
        // Запускаем процесс
        STARTUPINFOA si = {sizeof(si)};
        PROCESS_INFORMATION pi;

        if (CreateProcessA(
                "LibreHardwareMonitor.exe",
                NULL, NULL, NULL, FALSE, 0, NULL, NULL,
                &si, &pi))
        {
            setColor(10);
            cout << "  LibreHardwareMonitor started (PID: " << pi.dwProcessId << ")\n";
            setColor(7);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
        else
        {
            // Попробуем найти в других местах
            const char *paths[] = {
                ".\\LibreHardwareMonitor\\LibreHardwareMonitor.exe",
                "..\\LibreHardwareMonitor.exe",
                "C:\\Program Files\\LibreHardwareMonitor\\LibreHardwareMonitor.exe",
                NULL};

            for (int i = 0; paths[i] != NULL; i++)
            {
                if (CreateProcessA(
                        paths[i],
                        NULL, NULL, NULL, FALSE, 0, NULL, NULL,
                        &si, &pi))
                {
                    setColor(10);
                    cout << "  Found and started from: " << paths[i] << "\n";
                    setColor(7);
                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);
                    break;
                }
            }
        }
    }
}

void showMenu()
{
    showHeader();

    cout << "\n";

    // Опции меню
    setColor(10); // Зеленый
    cout << "    [1] System Information\n";
    cout << "    [2] Memory Test (RAM)\n";
    cout << "    [3] CPU Stress Test\n";
    cout << "    [4] Disk Speed Test (SSD/HDD)\n";
    cout << "    [5] GPU Benchmark Test\n";

    setColor(11); // Голубой
    cout << "    [6] Open Test Data Table\n";

    setColor(14); // Желтый
    cout << "    [7] Run ALL Tests\n";

    setColor(13); // Светло-лиловый
    cout << "    [8] Run 3D test !!\n";

    setColor(12); // Красный
    cout << "    [9] Exit\n";

    setColor(9); // ...
    cout << "    [i] Opening LibreHardwareMonitor\n";

    setColor(15); // Ярко-белый
    cout << "    [?] Feedback & Links\n";

    setColor(7); // Белый

    cout << "\n";
    cout << "    " << string(50, '-') << "\n";

    gotoxy(15, 22);
    setColor(15); // Яркий белый
    cout << "Select option [1-9, i, ?]: ";
}

void showProgress(const string &message, int duration = 1000)
{
    setColor(11);
    cout << "\n    " << message;

    // Анимация точек
    for (int i = 0; i < 3; i++)
    {
        cout << ".";
        cout.flush();
        this_thread::sleep_for(chrono::milliseconds(300));
    }

    cout << " DONE!\n";
    setColor(7);
    this_thread::sleep_for(chrono::milliseconds(500));
}

// Функция для открытия ссылок
void openLinks()
{
    clearScreen();
    showHeader();

    cout << "\n";
    setColor(11);
    cout << "    ===== FEEDBACK & LINKS =====\n\n";
    setColor(7);

    cout << "Clickable links (not work in all terminals):\n\n";

    // Для поддержки VT последовательностей в Windows 10+
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);

    // Гиперссылка для GitHub
    cout << "\033]8;;https://github.com/Totad7/SySpro\033\\";
    setColor(10);
    cout << "     [GitHub] https://github.com/Totad7/SySpro";
    cout << "\033]8;;\033\\";
    cout << "\n\n";

    // Гиперссылка для Telegram
    cout << "\033]8;;https://t.me/Totad7\033\\";
    setColor(11);
    cout << "     [Telegram] https://t.me/Totad7";
    cout << "\033]8;;\033\\";
    cout << "\n\n";

    setColor(7);
    cout << "Alternatively, type:\n";
    cout << "  1 - Open GitHub\n";
    cout << "  2 - Open Telegram\n";
    cout << "  3 - Back to menu\n\n";

    cout << "Your choice: ";

    char input;
    cin >> input;
    cin.ignore(); // Очистка буфера

    // Обработка цифрового ввода
    if (input >= '1' && input <= '3')
    {
        int choice = input - '0';
        switch (choice)
        {
        case 1:
            ShellExecute(NULL, "open", "https://github.com/Totad7/SySpro", NULL, NULL, SW_SHOWNORMAL);
            cout << "\nOpening GitHub...\n";
            break;

        case 2:
            ShellExecute(NULL, "open", "https://t.me/Totad7", NULL, NULL, SW_SHOWNORMAL);
            cout << "\nOpening Telegram...\n";
            break;

        default:
            cout << "\nReturning to menu...\n";
            return;
        }
    }
    else
    {
        cout << "\nInvalid choice, returning to menu...\n";
    }

    this_thread::sleep_for(chrono::seconds(1));
    cout << "\nPress ENTER to continue...";
    cin.get();
}

int main()
{
    // Настройка консоли
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(hConsole, &cursorInfo);

    while (true)
    {
        showMenu();

        char choice = _getch();
        cout << choice << "\n";

        // Обработка специальных символов
        if (choice == '?')
        {
            openLinks();
            continue;
        }

        // Обработка клавиши 'i' для LibreHardwareMonitor
        if (choice == 'i' || choice == 'I')
        {
            toggleLibreHardwareMonitor();

            cout << "\n\nPress ENTER to return to menu...";
            cin.ignore();
            while (true)
            {
                char key = _getch();
                if (key == 13 || key == 10)
                {
                    break;
                }
            }
            continue;
        }

        // Выход
        if (choice == '9')
        {
            clearScreen();
            gotoxy(30, 15);
            setColor(14);
            cout << "Thank you for using PC Diagnostic Tool!";
            setColor(7);
            this_thread::sleep_for(chrono::seconds(2));

            // Восстановить курсор
            cursorInfo.bVisible = true;
            SetConsoleCursorInfo(hConsole, &cursorInfo);
            return 0;
        }

        // Проверка валидности ввода (1-8 или ? или i)
        if (choice < '1' || choice > '8')
        {
            gotoxy(15, 23);
            setColor(12);
            cout << "Invalid selection! Please choose 1-8, 9, i or ?";
            setColor(7);
            this_thread::sleep_for(chrono::milliseconds(1500));
            continue;
        }

        int testNum = choice - '0';

        // Запуск выбранного теста
        clearScreen();
        showHeader();

        switch (testNum)
        {
        case 1:
            cout << "\n";
            setColor(11);
            cout << "    ===== SYSTEM INFORMATION =====\n\n";
            setColor(7);
            system_info::GetSystemInfo();
            break;

        case 2:
            cout << "\n";
            setColor(11);
            cout << "    ===== MEMORY TEST (RAM) =====\n\n";
            setColor(7);
            test_ram_speed();
            break;

        case 3:
            cout << "\n";
            setColor(11);
            cout << "    ===== CPU STRESS TEST =====\n\n";
            setColor(7);
            showProgress("Running CPU stress test");
            run_cpu_stress_test();
            break;

        case 4:
            cout << "\n";
            setColor(11);
            cout << "    ===== DISK SPEED TEST =====\n\n";
            setColor(7);
            showProgress("Testing disk speed");
            ssd_test::run_ssd_test();
            break;

        case 5:
            cout << "\n";
            setColor(11);
            cout << "    ===== GPU BENCHMARK TEST =====\n\n";
            setColor(7);
            showProgress("Running GPU benchmark");
            gpu_benchmark_final::run_universal_gpu_test();
            break;

        case 6:
            setColor(14);
            cout << "\n    Opening test data table in browser...\n\n";
            setColor(7);
            ShellExecute(NULL, "open",
                         "https://docs.google.com/spreadsheets/d/1UJE3ohQ2wus57dcyrBjFGD2sxgHR95LmYquv3D7Z8ic/edit?usp=sharing",
                         NULL, NULL, SW_SHOWNORMAL);
            cout << "      Google Sheets opened in your default browser\n";
            break;

        case 7:
            setColor(14);
            cout << "\n    ===== RUNNING ALL TESTS =====\n\n";
            setColor(7);

            cout << "\n[1/5] ";
            setColor(10);
            cout << "System Information:\n";
            setColor(7);
            system_info::GetSystemInfo();

            cout << "\n[2/5] ";
            setColor(10);
            cout << "RAM Test:\n";
            setColor(7);
            test_ram_speed();

            cout << "\n[3/5] ";
            setColor(10);
            cout << "CPU Test:\n";
            setColor(7);
            run_cpu_stress_test();

            cout << "\n[4/5] ";
            setColor(10);
            cout << "Disk Test:\n";
            setColor(7);
            ssd_test::run_ssd_test();

            cout << "\n[5/5] ";
            setColor(10);
            cout << "GPU Test:\n";
            setColor(7);
            gpu_benchmark_final::run_universal_gpu_test();

            cout << "\n";
            setColor(10);
            cout << "      ALL TESTS COMPLETED SUCCESSFULLY!\n";
            setColor(7);
            break;

        case 8:
            cout << "\n";
            setColor(11);
            cout << "    ===== 3D GPU STRESS TEST =====\n\n";
            setColor(7);
            showProgress("Running 3D GPU stress test");
            run_3D_gpu_stress();
            break;
        }

        // Панель навигации после теста (кроме случаев 6 и 8)
        if (testNum != 6 && testNum != 8) // 6 - таблица, 8 - 3D тест имеют свою систему выхода
        {
            cout << "\n\n    " << string(50, '=') << "\n";
            setColor(8); // Серый
            cout << "    Press ENTER to return to menu...";
            setColor(7);

            // Очистка буфера ввода
            while (_kbhit())
                _getch();

            // Ожидание Enter
            while (true)
            {
                char key = _getch();
                if (key == 13 || key == 10)
                { // Enter
                    break;
                }
            }
        }
    }

    // Восстановить курсор (на всякий случай)
    cursorInfo.bVisible = true;
    SetConsoleCursorInfo(hConsole, &cursorInfo);

    return 0;
}