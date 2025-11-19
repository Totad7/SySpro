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

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "iphlpapi.lib")

namespace system_info
{
    // Вспомогательные функции
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

    inline void get_gpu_info_d3d()
    {
        IDXGIFactory *pFactory;
        IDXGIAdapter *pAdapter;

        if (SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&pFactory)))
        {
            for (UINT i = 0; pFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
            {
                DXGI_ADAPTER_DESC desc;
                if (SUCCEEDED(pAdapter->GetDesc(&desc)))
                {
                    std::wstring wname(desc.Description);
                    std::string name(wname.begin(), wname.end());

                    std::cout << "  GPU " << i + 1 << ": " << name << "\n";
                    std::cout << "    VRAM: " << desc.DedicatedVideoMemory / (1024 * 1024 * 1024) << " GB\n";
                    std::cout << "    System RAM: " << desc.SharedSystemMemory / (1024 * 1024 * 1024) << " GB\n";
                }
                pAdapter->Release();
            }
            pFactory->Release();
        }
    }

    inline void get_disk_info()
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
                    { // Only show drives with media
                        double totalGB = totalBytes.QuadPart / (1024.0 * 1024 * 1024);
                        double freeGB = freeBytes.QuadPart / (1024.0 * 1024 * 1024);
                        double usedGB = totalGB - freeGB;
                        double usagePercent = (usedGB / totalGB) * 100;

                        std::cout << "    Drive " << driveLetter << ":\\\n";
                        std::cout << "      Total: " << std::fixed << std::setprecision(1) << totalGB << " GB\n";
                        std::cout << "      Free: " << freeGB << " GB\n";
                        std::cout << "      Used: " << usedGB << " GB (" << std::setprecision(0) << usagePercent << "%)\n";
                    }
                }
            }
            drives >>= 1;
            driveLetter++;
        }
    }

    inline void get_network_info()
    {
        std::cout << "  Network Adapters:\n";

        // Простая версия без сложных API
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
                        pAdapter = pAdapter->Next;
                    }
                }
                free(pAdapterInfo);
            }
        }
    }

    inline void get_process_count()
    {
        DWORD processes[1024];
        DWORD needed;

        if (EnumProcesses(processes, sizeof(processes), &needed))
        {
            DWORD processCount = needed / sizeof(DWORD);
            std::cout << "  Running processes: " << processCount << "\n";
        }
        else
        {
            std::cout << "  Running processes: Unable to determine\n";
        }
    }

    inline void get_motherboard_info()
    {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            char productName[256];
            DWORD bufferSize = sizeof(productName);

            if (RegQueryValueExA(hKey, "BaseBoardProduct", NULL, NULL, (LPBYTE)productName, &bufferSize) == ERROR_SUCCESS)
            {
                std::cout << "  Motherboard: " << productName << "\n";
            }
            else
            {
                std::cout << "  Motherboard: Unknown\n";
            }

            char manufacturer[256];
            bufferSize = sizeof(manufacturer);
            if (RegQueryValueExA(hKey, "BaseBoardManufacturer", NULL, NULL, (LPBYTE)manufacturer, &bufferSize) == ERROR_SUCCESS)
            {
                std::cout << "  Manufacturer: " << manufacturer << "\n";
            }

            RegCloseKey(hKey);
        }
        else
        {
            std::cout << "  Motherboard: Information not available\n";
        }
    }

    inline void get_bios_info()
    {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            char biosVersion[256];
            DWORD bufferSize = sizeof(biosVersion);

            if (RegQueryValueExA(hKey, "BIOSVersion", NULL, NULL, (LPBYTE)biosVersion, &bufferSize) == ERROR_SUCCESS)
            {
                std::cout << "  BIOS Version: " << biosVersion << "\n";
            }

            char releaseDate[256];
            bufferSize = sizeof(releaseDate);
            if (RegQueryValueExA(hKey, "BIOSReleaseDate", NULL, NULL, (LPBYTE)releaseDate, &bufferSize) == ERROR_SUCCESS)
            {
                std::cout << "  BIOS Date: " << releaseDate << "\n";
            }

            RegCloseKey(hKey);
        }
    }

    inline void GetSystemInfo()
    {
        std::cout << "==============================================\n";
        std::cout << "           SYSTEM INFORMATION\n";
        std::cout << "==============================================\n\n";

        // Operating System Information
        std::cout << "OPERATING SYSTEM:\n";
        std::cout << "  " << get_windows_version() << "\n";

        OSVERSIONINFO osvi;
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&osvi);
        std::cout << "  Version: " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion
                  << " (Build " << osvi.dwBuildNumber << ")\n";

        // Computer Name
        char computerName[256];
        DWORD size = sizeof(computerName);
        GetComputerNameA(computerName, &size);
        std::cout << "  Computer: " << computerName << "\n\n";

        // CPU Information
        std::cout << "PROCESSOR:\n";
        std::cout << "  " << get_cpu_name() << "\n";

        SYSTEM_INFO si;
        GetSystemInfo(&si);
        std::cout << "  Cores/Threads: " << si.dwNumberOfProcessors << "\n";
        std::cout << "  Architecture: " << (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "x64" : si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ? "x86"
                                                                                                                                                                          : "Unknown")
                  << "\n\n";

        // Memory Information
        std::cout << "MEMORY:\n";
        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof(statex);
        GlobalMemoryStatusEx(&statex);

        double totalGB = statex.ullTotalPhys / (1024.0 * 1024 * 1024);
        double availableGB = statex.ullAvailPhys / (1024.0 * 1024 * 1024);
        double usedGB = totalGB - availableGB;

        std::cout << "  Total RAM: " << std::fixed << std::setprecision(1) << totalGB << " GB\n";
        std::cout << "  Available: " << availableGB << " GB\n";
        std::cout << "  Used: " << usedGB << " GB\n";
        std::cout << "  Usage: " << statex.dwMemoryLoad << "%\n\n";

        // Motherboard Information
        std::cout << "MOTHERBOARD:\n";
        get_motherboard_info();
        std::cout << "\n";

        // BIOS Information
        std::cout << "BIOS:\n";
        get_bios_info();
        std::cout << "\n";

        // Graphics Information
        std::cout << "GRAPHICS CARDS:\n";
        get_gpu_info_d3d();
        std::cout << "\n";

        // Storage Information
        std::cout << "STORAGE:\n";
        get_disk_info();
        std::cout << "\n";

        // Network Information
        std::cout << "NETWORK:\n";
        get_network_info();
        std::cout << "\n";

        // System Information
        std::cout << "SYSTEM STATUS:\n";
        get_process_count();

        // Uptime
        DWORD uptime = GetTickCount() / 1000;
        int hours = uptime / 3600;
        int minutes = (uptime % 3600) / 60;
        int seconds = uptime % 60;
        std::cout << "  System Uptime: " << hours << "h " << minutes << "m " << seconds << "s\n";

        // User Name
        char userName[256];
        DWORD userNameSize = sizeof(userName);
        GetUserNameA(userName, &userNameSize);
        std::cout << "  Current User: " << userName << "\n";

        std::cout << "\n==============================================\n";
    }

} // namespace system_info