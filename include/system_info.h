#pragma once
#include <iostream>
#include <windows.h>
#include <d3d11.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <dxgi.h>
#include <psapi.h>
#include <iphlpapi.h>
#include <powrprof.h>
#include <chrono>
#include <thread>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "powrprof.lib")

namespace system_info
{
    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████
    // ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████

    inline std::string format_bytes(uint64_t bytes)
    {
        const char* sizes[] = {"B", "KB", "MB", "GB", "TB"};
        int order = 0;
        double size = static_cast<double>(bytes);
        
        while (size >= 1024 && order < 4) {
            order++;
            size /= 1024;
        }
        
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << size << " " << sizes[order];
        return ss.str();
    }

    inline std::string get_timestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████
    // ОС И СИСТЕМА
    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████

    inline std::string get_windows_version()
    {
        HKEY hKey;
        LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey);

        if (result == ERROR_SUCCESS)
        {
            char productName[256];
            DWORD bufferSize = sizeof(productName);

            if (RegQueryValueExA(hKey, "ProductName", NULL, NULL, (LPBYTE)productName, &bufferSize) == ERROR_SUCCESS)
            {
                RegCloseKey(hKey);

                std::string name(productName);

                // Прямая проверка для Windows 11
                if (name.find("Windows 11") != std::string::npos)
                {
                    return "Windows 11";
                }

                // Проверка по номеру сборки для Windows 11
                char currentBuild[256];
                bufferSize = sizeof(currentBuild);
                if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
                {
                    if (RegQueryValueExA(hKey, "CurrentBuild", NULL, NULL, (LPBYTE)currentBuild, &bufferSize) == ERROR_SUCCESS)
                    {
                        int buildNumber = atoi(currentBuild);
                        RegCloseKey(hKey);
                        if (buildNumber >= 22000)
                        {
                            return "Windows 11 (Build " + std::to_string(buildNumber) + ")";
                        }
                    }
                    else
                    {
                        RegCloseKey(hKey);
                    }
                }

                return name;
            }
            RegCloseKey(hKey);
        }

        return "Windows";
    }

    inline void get_system_uptime()
    {
        DWORD uptime = GetTickCount() / 1000;
        int days = uptime / 86400;
        int hours = (uptime % 86400) / 3600;
        int minutes = (uptime % 3600) / 60;
        int seconds = uptime % 60;
        
        std::cout << "  System Uptime: ";
        if (days > 0) std::cout << days << "d ";
        if (hours > 0) std::cout << hours << "h ";
        if (minutes > 0) std::cout << minutes << "m ";
        std::cout << seconds << "s\n";
    }

    inline void get_timezone_info()
    {
        TIME_ZONE_INFORMATION tzInfo;
        DWORD result = GetTimeZoneInformation(&tzInfo);
        
        if (result != TIME_ZONE_ID_INVALID)
        {
            std::wstring wname(tzInfo.StandardName);
            std::string name(wname.begin(), wname.end());
            std::cout << "  Timezone: " << name << " (UTC";
            
            int bias = -tzInfo.Bias;
            if (bias >= 0)
                std::cout << "+";
            else
                std::cout << "-";
                
            std::cout << std::abs(bias / 60) << ":" << std::setw(2) << std::setfill('0') << std::abs(bias % 60) << ")\n";
        }
    }

    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████
    // ПРОЦЕССОР
    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████

    inline std::string get_cpu_name()
    {
        HKEY hKey;
        LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey);

        if (result == ERROR_SUCCESS)
        {
            char processorName[256];
            DWORD bufferSize = sizeof(processorName);

            if (RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL, (LPBYTE)processorName, &bufferSize) == ERROR_SUCCESS)
            {
                RegCloseKey(hKey);
                // Убираем лишние пробелы в названии процессора
                std::string name(processorName);
                size_t pos;
                while ((pos = name.find("  ")) != std::string::npos)
                {
                    name.replace(pos, 2, " ");
                }
                return name;
            }
            RegCloseKey(hKey);
        }
        return "Unknown Processor";
    }

    inline void get_cpu_usage()
    {
        static ULARGE_INTEGER lastIdleTime, lastKernelTime, lastUserTime;
        FILETIME idleTime, kernelTime, userTime;
        
        if (GetSystemTimes(&idleTime, &kernelTime, &userTime))
        {
            ULARGE_INTEGER idle, kernel, user;
            idle.LowPart = idleTime.dwLowDateTime;
            idle.HighPart = idleTime.dwHighDateTime;
            kernel.LowPart = kernelTime.dwLowDateTime;
            kernel.HighPart = kernelTime.dwHighDateTime;
            user.LowPart = userTime.dwLowDateTime;
            user.HighPart = userTime.dwHighDateTime;
            
            ULONGLONG sys = kernel.QuadPart + user.QuadPart;
            
            if (lastIdleTime.QuadPart != 0)
            {
                ULONGLONG idleDiff = idle.QuadPart - lastIdleTime.QuadPart;
                ULONGLONG sysDiff = sys - (lastKernelTime.QuadPart + lastUserTime.QuadPart);
                
                if (sysDiff > 0)
                {
                    double usage = 100.0 - (100.0 * idleDiff / sysDiff);
                    std::cout << "  Current CPU Usage: " << std::fixed << std::setprecision(1) << usage << "%\n";
                }
            }
            
            lastIdleTime = idle;
            lastKernelTime = kernel;
            lastUserTime = user;
        }
        
        // Задержка для более точного измерения
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████
    // ПАМЯТЬ
    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████

    inline void get_detailed_memory_info()
    {
        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof(statex);
        
        if (GlobalMemoryStatusEx(&statex))
        {
            double totalGB = statex.ullTotalPhys / (1024.0 * 1024 * 1024);
            double availableGB = statex.ullAvailPhys / (1024.0 * 1024 * 1024);
            double usedGB = totalGB - availableGB;
            double usagePercent = (usedGB / totalGB) * 100;

            std::cout << "  Total RAM: " << std::fixed << std::setprecision(1) << totalGB << " GB\n";
            std::cout << "  Available: " << availableGB << " GB\n";
            std::cout << "  Used: " << usedGB << " GB\n";
            std::cout << "  Usage: " << statex.dwMemoryLoad << "%\n";
            
            // Дополнительная информация о памяти
            std::cout << "  Virtual Memory: " << format_bytes(statex.ullTotalPageFile) << "\n";
            std::cout << "  Virtual Available: " << format_bytes(statex.ullAvailPageFile) << "\n";
        }
    }

    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████
    // ГРАФИКА
    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████

    inline void get_gpu_info_d3d()
    {
        IDXGIFactory *pFactory;
        IDXGIAdapter *pAdapter;

        if (SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&pFactory)))
        {
            std::cout << "  Graphics Cards:\n";
            for (UINT i = 0; pFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
            {
                DXGI_ADAPTER_DESC desc;
                if (SUCCEEDED(pAdapter->GetDesc(&desc)))
                {
                    std::wstring wname(desc.Description);
                    std::string name(wname.begin(), wname.end());

                    std::cout << "    GPU " << i + 1 << ": " << name << "\n";
                    std::cout << "      VRAM: " << desc.DedicatedVideoMemory / (1024 * 1024 * 1024) << " GB\n";
                    std::cout << "      System RAM: " << desc.SharedSystemMemory / (1024 * 1024 * 1024) << " GB\n";
                    std::cout << "      Vendor ID: 0x" << std::hex << desc.VendorId << std::dec << "\n";
                    std::cout << "      Device ID: 0x" << std::hex << desc.DeviceId << std::dec << "\n";
                    std::cout << "      Revision: " << desc.Revision << "\n";
                }
                pAdapter->Release();
            }
            pFactory->Release();
        }
    }

    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████
    // ДИСКИ
    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████

    inline void get_detailed_disk_info()
    {
        DWORD drives = GetLogicalDrives();
        char driveLetter = 'A';

        std::cout << "  Storage Devices:\n";
        while (drives)
        {
            if (drives & 1)
            {
                std::string drivePath = std::string(1, driveLetter) + ":\\";
                ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;

                if (GetDiskFreeSpaceExA(drivePath.c_str(), &freeBytes, &totalBytes, &totalFreeBytes))
                {
                    if (totalBytes.QuadPart > 0)
                    {
                        double totalGB = totalBytes.QuadPart / (1024.0 * 1024 * 1024);
                        double freeGB = freeBytes.QuadPart / (1024.0 * 1024 * 1024);
                        double usedGB = totalGB - freeGB;
                        double usagePercent = (usedGB / totalGB) * 100;

                        std::cout << "    Drive " << driveLetter << ":\\\n";
                        std::cout << "      Total: " << std::fixed << std::setprecision(1) << totalGB << " GB\n";
                        std::cout << "      Free: " << freeGB << " GB\n";
                        std::cout << "      Used: " << usedGB << " GB (" << std::setprecision(0) << usagePercent << "%)\n";
                        
                        // Информация о файловой системе
                        char fsName[256];
                        DWORD serialNumber, maxComponentLength, fileSystemFlags;
                        if (GetVolumeInformationA(drivePath.c_str(), NULL, 0, &serialNumber, &maxComponentLength, &fileSystemFlags, fsName, sizeof(fsName)))
                        {
                            std::cout << "      File System: " << fsName << "\n";
                            std::cout << "      Serial: " << std::hex << serialNumber << std::dec << "\n";
                        }
                    }
                }
            }
            drives >>= 1;
            driveLetter++;
        }
    }

    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████
    // СЕТЬ
    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████

    inline void get_detailed_network_info()
    {
        std::cout << "  Network Adapters:\n";

        PIP_ADAPTER_INFO pAdapterInfo;
        PIP_ADAPTER_INFO pAdapter = NULL;
        DWORD dwRetVal = 0;
        ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);

        pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
        if (pAdapterInfo)
        {
            if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
            {
                free(pAdapterInfo);
                pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
            }

            if (pAdapterInfo)
            {
                if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
                {
                    pAdapter = pAdapterInfo;
                    int adapterCount = 0;

                    while (pAdapter)
                    {
                        adapterCount++;
                        std::cout << "    Adapter " << adapterCount << ": " << pAdapter->Description << "\n";
                        std::cout << "      MAC: ";
                        for (UINT i = 0; i < pAdapter->AddressLength; i++)
                        {
                            printf("%02X", pAdapter->Address[i]);
                            if (i < pAdapter->AddressLength - 1)
                                printf(":");
                        }
                        std::cout << "\n";
                        std::cout << "      IP: " << pAdapter->IpAddressList.IpAddress.String << "\n";
                        std::cout << "      Gateway: " << pAdapter->GatewayList.IpAddress.String << "\n";
                        std::cout << "      DHCP: " << (pAdapter->DhcpEnabled ? "Enabled" : "Disabled") << "\n";
                        if (pAdapter->DhcpEnabled)
                        {
                            std::cout << "      DHCP Server: " << pAdapter->DhcpServer.IpAddress.String << "\n";
                        }
                        pAdapter = pAdapter->Next;
                    }
                }
                free(pAdapterInfo);
            }
        }
    }

    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████
    // ПИТАНИЕ И АККУМУЛЯТОР
    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████

    inline void get_power_info()
    {
        SYSTEM_POWER_STATUS powerStatus;
        if (GetSystemPowerStatus(&powerStatus))
        {
            std::cout << "  Power Status: ";
            switch (powerStatus.ACLineStatus)
            {
                case 0: std::cout << "Battery"; break;
                case 1: std::cout << "AC Power"; break;
                case 255: std::cout << "Unknown"; break;
            }
            std::cout << "\n";
            
            if (powerStatus.ACLineStatus == 0) // На батарее
            {
                std::cout << "  Battery Life: " << static_cast<int>(powerStatus.BatteryLifePercent) << "%\n";
                if (powerStatus.BatteryLifeTime != (DWORD)-1)
                {
                    int minutes = powerStatus.BatteryLifeTime / 60;
                    int hours = minutes / 60;
                    minutes = minutes % 60;
                    std::cout << "  Time Remaining: " << hours << "h " << minutes << "m\n";
                }
            }
            
            std::cout << "  Battery Saver: " << (powerStatus.SystemStatusFlag ? "Active" : "Inactive") << "\n";
            std::cout << "  Battery Charging: " << (powerStatus.BatteryFlag & 8 ? "Yes" : "No") << "\n";
        }
        else
        {
            std::cout << "  Power information not available\n";
        }
    }

    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████
    // USB И ПЕРИФЕРИЯ (УПРОЩЕННАЯ ВЕРСИЯ)
    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████

    inline void get_usb_devices()
    {
        std::cout << "  USB Devices: Information unavailable (setupapi required)\n";
        // Временное отключение USB функционала из-за проблем с компиляцией
    }

    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████
    // ДОПОЛНИТЕЛЬНАЯ СИСТЕМНАЯ ИНФОРМАЦИЯ
    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████

    inline void get_system_metrics_info()
    {
        std::cout << "  System Metrics:\n";
        std::cout << "    Screen Resolution: " << GetSystemMetrics(SM_CXSCREEN) << "x" << GetSystemMetrics(SM_CYSCREEN) << "\n";
        std::cout << "    Mouse Buttons: " << GetSystemMetrics(SM_CMOUSEBUTTONS) << "\n";
        
        // Простая проверка типа компьютера через системные метрики
        std::cout << "    Computer Type: ";
        if (GetSystemMetrics(SM_TABLETPC) != 0)
            std::cout << "Tablet";
        else if (GetSystemMetrics(SM_MEDIACENTER) != 0)
            std::cout << "Media Center";
        else
            std::cout << "Desktop/Laptop";
        std::cout << "\n";
    }

    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████
    // ИНФОРМАЦИЯ О ПРОЦЕССАХ В РЕАЛЬНОМ ВРЕМЕНИ
    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████

    inline void get_real_time_process_info()
    {
        DWORD processes[4096];
        DWORD needed;
        
        if (EnumProcesses(processes, sizeof(processes), &needed))
        {
            DWORD processCount = needed / sizeof(DWORD);
            std::cout << "  Running processes: " << processCount << "\n";
            
            // Топ 5 процессов по использованию памяти
            std::cout << "  Top memory-consuming processes:\n";
            
            struct ProcessInfo {
                DWORD pid;
                std::string name;
                SIZE_T memory;
            };
            
            std::vector<ProcessInfo> topProcesses;
            
            for (DWORD i = 0; i < processCount && i < 50; i++) // Проверяем первые 50 процессов
            {
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);
                if (hProcess)
                {
                    PROCESS_MEMORY_COUNTERS pmc;
                    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
                    {
                        char processName[MAX_PATH] = "<unknown>";
                        if (GetModuleBaseNameA(hProcess, NULL, processName, sizeof(processName)))
                        {
                            topProcesses.push_back({processes[i], processName, pmc.WorkingSetSize});
                        }
                    }
                    CloseHandle(hProcess);
                }
            }
            
            // Сортируем по использованию памяти и выводим топ 5
            std::sort(topProcesses.begin(), topProcesses.end(), 
                [](const ProcessInfo& a, const ProcessInfo& b) { return a.memory > b.memory; });
            
            for (int i = 0; i < std::min(5, (int)topProcesses.size()); i++)
            {
                std::cout << "    " << topProcesses[i].name << " - " << format_bytes(topProcesses[i].memory) << "\n";
            }
        }
    }

    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████
    // ГЛАВНАЯ ФУНКЦИЯ
    // █████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████

    inline void GetSystemInfo()
    {
        std::cout << "==============================================\n";
        std::cout << "           COMPREHENSIVE SYSTEM INFORMATION\n";
        std::cout << "==============================================\n\n";
        
        std::cout << "Report generated: " << get_timestamp() << "\n\n";

        // ОС И СИСТЕМА
        std::cout << "OPERATING SYSTEM:\n";
        std::cout << "  " << get_windows_version() << "\n";
        
        OSVERSIONINFO osvi;
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&osvi);
        std::cout << "  Version: " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion
                  << " (Build " << osvi.dwBuildNumber << ")\n";

        char computerName[256];
        DWORD size = sizeof(computerName);
        GetComputerNameA(computerName, &size);
        std::cout << "  Computer: " << computerName << "\n";
        
        get_system_uptime();
        get_timezone_info();
        std::cout << "\n";

        // ПРОЦЕССОР
        std::cout << "PROCESSOR:\n";
        std::cout << "  " << get_cpu_name() << "\n";

        SYSTEM_INFO si;
        ::GetSystemInfo(&si);
        std::cout << "  Cores/Threads: " << si.dwNumberOfProcessors << "\n";
        std::cout << "  Architecture: " << (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "x64" : "x86") << "\n";
        get_cpu_usage();
        std::cout << "\n";

        // ПАМЯТЬ
        std::cout << "MEMORY:\n";
        get_detailed_memory_info();
        std::cout << "\n";

        // ГРАФИКА
        std::cout << "GRAPHICS:\n";
        get_gpu_info_d3d();
        std::cout << "\n";

        // ДИСКИ
        std::cout << "STORAGE:\n";
        get_detailed_disk_info();
        std::cout << "\n";

        // СЕТЬ
        std::cout << "NETWORK:\n";
        get_detailed_network_info();
        std::cout << "\n";

        // ПИТАНИЕ
        std::cout << "POWER:\n";
        get_power_info();
        std::cout << "\n";

        // ПРОЦЕССЫ
        std::cout << "PROCESSES:\n";
        get_real_time_process_info();
        std::cout << "\n";

        // USB УСТРОЙСТВА
        std::cout << "USB DEVICES:\n";
        get_usb_devices();
        std::cout << "\n";

        // СИСТЕМНЫЕ МЕТРИКИ
        std::cout << "SYSTEM METRICS:\n";
        get_system_metrics_info();
        std::cout << "\n";

        std::cout << "==============================================\n";
    }

} // namespace system_info