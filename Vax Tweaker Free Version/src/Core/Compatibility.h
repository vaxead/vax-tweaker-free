
#pragma once

#include "Types.h"
#include <string>
#include <vector>

namespace Vax {

    class Compatibility {
    public:
        static std::string GetWarning(const std::string& tweakId);

        static bool HasAnyWarning(const std::vector<TweakInfo>& tweaks);

        static std::string GetBatchWarning(const std::vector<TweakInfo>& tweaks);

    private:
        Compatibility() = default;
    };

}
