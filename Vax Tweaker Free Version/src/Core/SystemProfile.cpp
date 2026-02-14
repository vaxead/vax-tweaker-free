
#include "SystemProfile.h"
#include "Admin.h"
#include "Types.h"
#include "../UI/Console.h"
#include "../System/Logger.h"

#include <windows.h>
#include <intrin.h>
#include <dxgi.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "PowrProf.lib")

extern "C" {
    DWORD WINAPI PowerGetActiveScheme(HKEY, GUID**);
    DWORD WINAPI PowerReadFriendlyName(HKEY, const GUID*, const GUID*, const GUID*,
                                        PUCHAR, LPDWORD);
}

namespace Vax {

    SystemProfile& SystemProfile::GetCurrent() {
        static SystemProfile instance;
        return instance;
    }

    void SystemProfile::RunScan(bool adminMode) {
        isAdmin = adminMode;
        DetectOS();
        DetectCPU();
        DetectRAM();
        DetectGPU();
        DetectPower();
        DetectPowerScheme(adminMode);
        DetectModernStandby(adminMode);
        m_scanned = true;
    }

    void SystemProfile::DetectOS() {
        osVersion = UI::Console::GetWindowsVersionString();
        osBuild = UI::Console::GetWindowsBuildNumber();
    }

    void SystemProfile::DetectCPU() {
        int cpuInfo[4] = {};
        char brand[49] = {};

        __cpuid(cpuInfo, 0x80000000);
        unsigned int maxExt = static_cast<unsigned int>(cpuInfo[0]);
        if (maxExt >= 0x80000004) {
            __cpuid(cpuInfo, 0x80000002);
            memcpy(brand, cpuInfo, 16);
            __cpuid(cpuInfo, 0x80000003);
            memcpy(brand + 16, cpuInfo, 16);
            __cpuid(cpuInfo, 0x80000004);
            memcpy(brand + 32, cpuInfo, 16);
            brand[48] = '\0';

            std::string raw(brand);
            size_t start = raw.find_first_not_of(' ');
            cpuBrand = (start != std::string::npos) ? raw.substr(start) : raw;
        } else {
            cpuBrand = "Unknown CPU";
        }
    }

    void SystemProfile::DetectRAM() {
        MEMORYSTATUSEX mem = {};
        mem.dwLength = sizeof(mem);
        if (GlobalMemoryStatusEx(&mem)) {
            totalRamMB = static_cast<int>(mem.ullTotalPhys / (1024 * 1024));
        }
    }

    void SystemProfile::DetectGPU() {
        gpuName = "Unknown";
        gpuDriverVersion = "Unknown";

        IDXGIFactory* factory = nullptr;
        HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory));
        if (FAILED(hr) || !factory) {
            System::Logger::Warning("DXGI factory creation failed — GPU detection skipped");
            return;
        }

        IDXGIAdapter* adapter = nullptr;
        if (SUCCEEDED(factory->EnumAdapters(0, &adapter)) && adapter) {
            DXGI_ADAPTER_DESC desc = {};
            if (SUCCEEDED(adapter->GetDesc(&desc))) {
                char nameBuf[128] = {};
                WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1,
                                    nameBuf, sizeof(nameBuf), nullptr, nullptr);
                gpuName = nameBuf;

                LARGE_INTEGER driverVer = {};
                if (SUCCEEDED(adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &driverVer))) {
                    WORD product  = HIWORD(driverVer.HighPart);
                    WORD version  = LOWORD(driverVer.HighPart);
                    WORD subver   = HIWORD(driverVer.LowPart);
                    WORD build    = LOWORD(driverVer.LowPart);
                    gpuDriverVersion = std::to_string(product) + "." +
                                       std::to_string(version) + "." +
                                       std::to_string(subver) + "." +
                                       std::to_string(build);
                }
            }
            adapter->Release();
        }
        factory->Release();
    }

    void SystemProfile::DetectPower() {
        SYSTEM_POWER_STATUS sps = {};
        if (GetSystemPowerStatus(&sps)) {
            batteryPresent = (sps.BatteryFlag != 128);
            onACPower = (sps.ACLineStatus == 1);
        } else {
            batteryPresent = false;
            onACPower = true;
        }
        isLaptop = batteryPresent;
    }

    void SystemProfile::DetectPowerScheme(bool /*adminMode*/) {
        powerScheme = "Unknown";

        GUID* schemeGuid = nullptr;
        DWORD err = PowerGetActiveScheme(NULL, &schemeGuid);
        if (err != ERROR_SUCCESS || !schemeGuid) {
            return;
        }

        DWORD bufSize = 0;
        err = PowerReadFriendlyName(NULL, schemeGuid, NULL, NULL, NULL, &bufSize);
        if (err == ERROR_SUCCESS && bufSize > 0) {
            std::vector<BYTE> buf(bufSize, 0);
            err = PowerReadFriendlyName(NULL, schemeGuid, NULL, NULL, buf.data(), &bufSize);
            if (err == ERROR_SUCCESS) {
                std::wstring wname(reinterpret_cast<wchar_t*>(buf.data()));
                char narrow[256] = {};
                WideCharToMultiByte(CP_UTF8, 0, wname.c_str(), -1,
                                    narrow, sizeof(narrow), nullptr, nullptr);
                powerScheme = narrow;
            }
        }

        if (powerScheme == "Unknown" || powerScheme.empty()) {
            wchar_t guidStr[64] = {};
            if (StringFromGUID2(*schemeGuid, guidStr, 64)) {
                char narrow[128] = {};
                WideCharToMultiByte(CP_UTF8, 0, guidStr, -1,
                                    narrow, sizeof(narrow), nullptr, nullptr);
                powerScheme = narrow;
            }
        }

        LocalFree(schemeGuid);
    }

    void SystemProfile::DetectModernStandby(bool /*adminMode*/) {
        modernStandby = "Unknown";

        HKEY hKey = nullptr;
        LONG res = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\Power",
            0, KEY_READ, &hKey);
        if (res == ERROR_SUCCESS && hKey) {
            DWORD val = 0;
            DWORD size = sizeof(val);
            res = RegQueryValueExA(hKey, "CsEnabled", nullptr, nullptr,
                                   reinterpret_cast<LPBYTE>(&val), &size);
            if (res == ERROR_SUCCESS) {
                modernStandby = (val == 1) ? "Supported (S0 Low Power Idle)"
                                           : "Not supported";
            }
            RegCloseKey(hKey);
        }
    }

    std::string SystemProfile::ToText() const {
        std::ostringstream ss;
        ss << "=== VAX TWEAKER — System Profile ===\n";

        auto now = std::time(nullptr);
        struct tm tmBuf = {};
        localtime_s(&tmBuf, &now);
        ss << "Generated: " << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S") << "\n";
        ss << "App Version: " << APP_VERSION << "\n";
        ss << "\n";

        ss << "[System]\n";
        ss << "  OS:              " << osVersion << " (Build " << osBuild << ")\n";
        ss << "  Admin:           " << (isAdmin ? "Yes" : "No") << "\n";
        ss << "\n";

        ss << "[Hardware]\n";
        ss << "  CPU:             " << cpuBrand << "\n";
        ss << "  RAM:             " << totalRamMB << " MB ("
           << (totalRamMB / 1024) << " GB)\n";
        ss << "  GPU:             " << gpuName << "\n";
        ss << "  GPU Driver:      " << gpuDriverVersion << "\n";
        ss << "\n";

        ss << "[Power]\n";
        ss << "  Form Factor:     " << (isLaptop ? "Laptop" : "Desktop") << "\n";
        ss << "  Battery:         " << (batteryPresent ? "Present" : "Not detected") << "\n";
        ss << "  AC Power:        " << (onACPower ? "Connected" : "On battery") << "\n";
        ss << "  Power Scheme:    " << powerScheme << "\n";
        ss << "  Modern Standby:  " << modernStandby << "\n";

        return ss.str();
    }

}
