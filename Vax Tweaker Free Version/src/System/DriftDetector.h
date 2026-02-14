
#pragma once

#include "../Core/Types.h"
#include "../Modules/IModule.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Vax::System {

    struct DriftEntry {
        std::string moduleName;
        std::string tweakId;
        std::string tweakName;
    };

    class DriftDetector {
    public:
        static void SaveSnapshot(
            const std::vector<std::unique_ptr<Modules::IModule>>& modules);

        static std::vector<DriftEntry> DetectDrift(
            const std::vector<std::unique_ptr<Modules::IModule>>& modules);

        static std::string GetSnapshotPath();

    private:
        static std::unordered_map<std::string, std::vector<std::string>> LoadSnapshot();
    };

}
