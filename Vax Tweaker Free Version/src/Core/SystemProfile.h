
#pragma once

#include <string>
#include <vector>

namespace Vax {

    struct SystemProfile {
        std::string osVersion;
        int         osBuild = 0;

        bool isAdmin = false;

        std::string cpuBrand;
        int         totalRamMB = 0;

        std::string gpuName;
        std::string gpuDriverVersion;

        bool batteryPresent = false;
        bool onACPower      = true;
        bool isLaptop       = false;

        std::string powerScheme;

        std::string modernStandby;

        static SystemProfile& GetCurrent();

        void RunScan(bool adminMode);

        bool HasScanned() const { return m_scanned; }

        std::string ToText() const;

    private:
        void DetectOS();
        void DetectCPU();
        void DetectRAM();
        void DetectGPU();
        void DetectPower();
        void DetectPowerScheme(bool adminMode);
        void DetectModernStandby(bool adminMode);

        bool m_scanned = false;
    };

}
