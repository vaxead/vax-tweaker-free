
#pragma once

#include "../Core/Types.h"
#include <string>

namespace Vax::Safety {

    class SafetyGuard {
    public:
        static bool ConfirmTweak(const TweakInfo& tweak);

        static bool ConfirmRevert(const TweakInfo& tweak);

        static bool ConfirmApplyAll(const std::string& moduleName, int tweakCount);

        static void ShowRebootNotice();

        static std::string GetRiskDescription(RiskLevel level);

        static std::string GetRiskColor(RiskLevel level);

    private:
        SafetyGuard() = default;
    };

}
