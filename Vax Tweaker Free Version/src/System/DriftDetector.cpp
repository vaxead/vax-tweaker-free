
#include "DriftDetector.h"
#include "Registry.h"
#include "Logger.h"
#include <fstream>
#include <sstream>

namespace Vax::System {

    std::string DriftDetector::GetSnapshotPath() {
        return Registry::GetAppDataDir() + "\\vax_snapshot.dat";
    }

    void DriftDetector::SaveSnapshot(
            const std::vector<std::unique_ptr<Modules::IModule>>& modules) {

        std::string path = GetSnapshotPath();
        std::ofstream file(path, std::ios::out | std::ios::trunc);
        if (!file.is_open()) {
            Logger::Warning("DriftDetector: could not save snapshot to " + path);
            return;
        }

        int totalApplied = 0;
        for (const auto& mod : modules) {
            if (!mod || !mod->IsImplemented()) continue;
            auto info = mod->GetInfo();
            auto tweaks = mod->GetTweaks();

            std::vector<std::string> appliedIds;
            for (const auto& t : tweaks) {
                if (t.status == TweakStatus::Applied) {
                    appliedIds.push_back(t.id);
                }
            }

            if (!appliedIds.empty()) {
                file << "[" << info.name << "]\n";
                for (const auto& id : appliedIds) {
                    file << id << "\n";
                }
                totalApplied += static_cast<int>(appliedIds.size());
            }
        }

        file.close();
        Logger::Info("DriftDetector: snapshot saved (" +
                     std::to_string(totalApplied) + " applied tweaks)");
    }

    std::unordered_map<std::string, std::vector<std::string>>
    DriftDetector::LoadSnapshot() {
        std::unordered_map<std::string, std::vector<std::string>> result;
        std::string path = GetSnapshotPath();

        std::ifstream file(path);
        if (!file.is_open()) return result;

        std::string line;
        std::string currentModule;

        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (line.empty()) continue;

            if (line.front() == '[' && line.back() == ']') {
                currentModule = line.substr(1, line.size() - 2);
            } else if (!currentModule.empty()) {
                result[currentModule].push_back(line);
            }
        }
        return result;
    }

    std::vector<DriftEntry> DriftDetector::DetectDrift(
            const std::vector<std::unique_ptr<Modules::IModule>>& modules) {

        auto snapshot = LoadSnapshot();
        if (snapshot.empty()) return {};

        std::vector<DriftEntry> drifted;

        for (const auto& mod : modules) {
            if (!mod || !mod->IsImplemented()) continue;
            auto info = mod->GetInfo();

            auto it = snapshot.find(info.name);
            if (it == snapshot.end()) continue;

            auto tweaks = mod->GetTweaks();
            std::unordered_map<std::string, const TweakInfo*> tweakMap;
            for (const auto& t : tweaks) {
                tweakMap[t.id] = &t;
            }

            for (const auto& savedId : it->second) {
                auto tweakIt = tweakMap.find(savedId);
                if (tweakIt == tweakMap.end()) continue;

                const TweakInfo* tweak = tweakIt->second;
                if (tweak->status == TweakStatus::NotApplied ||
                    tweak->status == TweakStatus::Partial) {
                    drifted.push_back({
                        info.name,
                        tweak->id,
                        tweak->name
                    });
                }
            }
        }

        if (!drifted.empty()) {
            Logger::Warning("DriftDetector: " + std::to_string(drifted.size()) +
                           " tweak(s) drifted since last snapshot");
        }

        return drifted;
    }

}
