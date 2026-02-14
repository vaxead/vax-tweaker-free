
#pragma once

#include <string>

namespace Vax::System {

    class RestorePoint {
    public:
        static bool Create(const std::string& description);

        static bool IsProtectionEnabled();

        static bool EnableProtection();

    private:
        static void DisableFrequencyLimit();

        static void RestoreFrequencyLimit();

        RestorePoint() = default;
    };

}
