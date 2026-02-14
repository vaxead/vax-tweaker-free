
#include "PowerPlanManager.h"
#include "Logger.h"
#include "ProcessUtils.h"
#include <windows.h>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

namespace {

    constexpr DWORD kProcessTimeoutMs = 15000;

    char SafeToLower(char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

}

namespace Vax::System {

    bool PowerPlanManager::IsValidGuid(const std::string& s) {
        if (s.size() != 36) return false;
        for (size_t i = 0; i < 36; ++i) {
            if (i == 8 || i == 13 || i == 18 || i == 23) {
                if (s[i] != '-') return false;
            } else {
                char c = static_cast<char>(::tolower(static_cast<unsigned char>(s[i])));
                if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
                    return false;
            }
        }
        return true;
    }

    std::string PowerPlanManager::ExtractGuid(const std::string& text) {
        for (size_t i = 0; i + 36 <= text.size(); ++i) {
            std::string candidate = text.substr(i, 36);
            if (IsValidGuid(candidate)) return candidate;
        }
        return "";
    }

    std::string PowerPlanManager::EscapeCmdArg(const std::string& arg) {
        std::string safe;
        safe.reserve(arg.size() + 2);
        safe += '"';
        for (char c : arg) {
            if (c != '"') safe += c;
        }
        safe += '"';
        return safe;
    }

    bool PowerPlanManager::RunPowercfg(const std::string& args) {
        DWORD exitCode = 0;
        bool ok = Vax::System::RunSilentCommand(args, kProcessTimeoutMs, &exitCode);
        if (!ok) {
            if (exitCode == static_cast<DWORD>(-1)) {
                Logger::Error("PowerPlan: Failed to launch: " + args);
            } else if (exitCode == static_cast<DWORD>(-2)) {
                Logger::Error("PowerPlan: Timed out: " + args);
            } else {
                Logger::Warning("PowerPlan: Exit " + std::to_string(exitCode) + ": " + args);
            }
        }
        return ok;
    }

    std::string PowerPlanManager::RunPowercfgCapture(const std::string& args) {
        return Vax::System::RunCommandCapture(args);
    }

    std::vector<VaxPowerPlan> PowerPlanManager::GetVaxPlans() {
        std::string activeGuid = GetActiveSchemeGuid();
        std::string cachedList = RunPowercfgCapture("powercfg /list");

        std::string activeLower = activeGuid;
        std::transform(activeLower.begin(), activeLower.end(), activeLower.begin(), SafeToLower);

        auto makeEntry = [&](const char* displayName, const char* desc,
                             PlanIntensity intensity, const char* guid) -> VaxPowerPlan {
            VaxPowerPlan p;
            p.displayName = displayName;
            p.description = desc;
            p.intensity = intensity;

            std::string gLower = guid;
            std::transform(gLower.begin(), gLower.end(), gLower.begin(), SafeToLower);

            p.guid = PlanExistsInCache(guid, cachedList) ? guid : "";
            p.isActive = (!p.guid.empty() && gLower == activeLower);
            return p;
        };

        return {
            makeEntry(
                "Vax Power Plan \xe2\x80\x94 Laptop Boost",
                "Optimized for laptops: responsive performance with thermal/battery limits",
                PlanIntensity::Low,
                kVaxLaptopGuid),
            makeEntry(
                "Vax Power Plan \xe2\x80\x94 Balanced (Recommended)",
                "Best all-round: fast responsiveness without aggressive power draw",
                PlanIntensity::Medium,
                kVaxBalancedGuid),
            makeEntry(
                "Vax Power Plan \xe2\x80\x94 Performance Max",
                "Maximum performance: all cores active, boost unrestricted, high power draw",
                PlanIntensity::High,
                kVaxPerfMaxGuid)
        };
    }

    bool PowerPlanManager::InstallAndActivate(int planIndex) {
        struct PlanDef {
            const char* guid;
            const char* baseGuid;
            const char* sysName;
            PlanIntensity intensity;
        };

        const PlanDef defs[] = {
            { kVaxLaptopGuid,   kBalancedGuid,  "Vax Power Plan - Laptop Boost",            PlanIntensity::Low },
            { kVaxBalancedGuid, kBalancedGuid,  "Vax Power Plan - Balanced (Recommended)",   PlanIntensity::Medium },
            { kVaxPerfMaxGuid,  kHighPerfGuid,  "Vax Power Plan - Performance Max",          PlanIntensity::High }
        };

        if (planIndex < 0 || planIndex > 2) return false;
        const auto& def = defs[planIndex];

        if (!PlanExists(def.guid)) {
            Logger::Info("PowerPlan: Creating " + std::string(def.sysName));
            if (!ClonePlan(def.baseGuid, def.guid, def.sysName)) {
                Logger::Error("PowerPlan: Failed to create plan (clone failed) — may require admin");
                return false;
            }
        } else {
            Logger::Info("PowerPlan: " + std::string(def.sysName) + " already exists, reusing");
            RunPowercfg("powercfg /changename " + std::string(def.guid)
                        + " " + EscapeCmdArg(def.sysName) + " "
                        + EscapeCmdArg("Vax Tweaker custom plan"));
        }

        if (!TunePlan(def.guid, def.intensity)) {
            Logger::Warning("PowerPlan: Some tuning parameters failed (best-effort)");
        }

        if (!ActivatePlan(def.guid)) {
            Logger::Error("PowerPlan: Failed to activate plan — may require admin");
            return false;
        }

        Logger::Success("PowerPlan: Activated " + std::string(def.sysName));
        return true;
    }

    std::vector<SystemPowerPlan> PowerPlanManager::ListAllPlans() {
        std::vector<SystemPowerPlan> plans;
        std::string output = RunPowercfgCapture("powercfg /list");
        if (output.empty()) return plans;

        std::string activeGuid = GetActiveSchemeGuid();
        std::string activeLower = activeGuid;
        std::transform(activeLower.begin(), activeLower.end(), activeLower.begin(), SafeToLower);

        std::istringstream ss(output);
        std::string line;
        while (std::getline(ss, line)) {
            std::string guid = ExtractGuid(line);
            if (guid.empty()) continue;

            SystemPowerPlan plan;
            plan.guid = guid;

            size_t guidPos = line.find(guid);
            size_t afterGuid = (guidPos != std::string::npos) ? guidPos + guid.size() : 0;
            size_t pOpen = line.find('(', afterGuid);
            size_t pClose = (pOpen != std::string::npos) ? line.find(')', pOpen) : std::string::npos;
            if (pOpen != std::string::npos && pClose != std::string::npos && pClose > pOpen) {
                plan.name = line.substr(pOpen + 1, pClose - pOpen - 1);
            } else {
                plan.name = guid;
            }

            std::string gLower = guid;
            std::transform(gLower.begin(), gLower.end(), gLower.begin(), SafeToLower);
            plan.isActive = (gLower == activeLower);

            plans.push_back(plan);
        }
        return plans;
    }

    bool PowerPlanManager::RemovePlan(const std::string& guid) {
        if (!IsValidGuid(guid)) {
            Logger::Error("PowerPlan: RemovePlan called with invalid GUID");
            return false;
        }

        std::string active = GetActiveSchemeGuid();
        std::string aLower = active;
        std::transform(aLower.begin(), aLower.end(), aLower.begin(), SafeToLower);
        std::string gLower = guid;
        std::transform(gLower.begin(), gLower.end(), gLower.begin(), SafeToLower);

        if (aLower == gLower) {
            Logger::Warning("PowerPlan: Cannot remove the active power plan");
            return false;
        }

        bool ok = RunPowercfg("powercfg /delete " + guid);
        if (ok) {
            Logger::Success("PowerPlan: Removed plan " + guid);
        }
        return ok;
    }

    int PowerPlanManager::RemoveUnusedPlans() {
        auto plans = ListAllPlans();
        std::string activeGuid = GetActiveSchemeGuid();
        std::string activeLower = activeGuid;
        std::transform(activeLower.begin(), activeLower.end(), activeLower.begin(), SafeToLower);

        const std::string preserve[] = {
            kBalancedGuid,
            kHighPerfGuid,
            kPowerSaverGuid,
            kVaxLaptopGuid,
            kVaxBalancedGuid,
            kVaxPerfMaxGuid
        };

        int removed = 0;
        for (const auto& plan : plans) {
            std::string gLower = plan.guid;
            std::transform(gLower.begin(), gLower.end(), gLower.begin(), SafeToLower);

            if (gLower == activeLower) continue;

            bool skip = false;
            for (const auto& p : preserve) {
                std::string pLower = p;
                std::transform(pLower.begin(), pLower.end(), pLower.begin(), SafeToLower);
                if (gLower == pLower) { skip = true; break; }
            }
            if (skip) continue;

            if (RemovePlan(plan.guid)) {
                ++removed;
            }
        }
        return removed;
    }

    std::string PowerPlanManager::GetActiveSchemeGuid() {
        std::string output = RunPowercfgCapture("powercfg /getactivescheme");
        return ExtractGuid(output);
    }

    bool PowerPlanManager::PlanExists(const std::string& guid) {
        if (!IsValidGuid(guid)) return false;

        std::string output = RunPowercfgCapture("powercfg /list");
        return PlanExistsInCache(guid, output);
    }

    bool PowerPlanManager::PlanExistsInCache(const std::string& guid, const std::string& cachedList) {
        if (!IsValidGuid(guid)) return false;

        std::string gLower = guid;
        std::transform(gLower.begin(), gLower.end(), gLower.begin(), SafeToLower);
        std::string oLower = cachedList;
        std::transform(oLower.begin(), oLower.end(), oLower.begin(), SafeToLower);
        return oLower.find(gLower) != std::string::npos;
    }

    bool PowerPlanManager::ClonePlan(const std::string& baseGuid,
                                     const std::string& newGuid,
                                     const std::string& name) {
        if (!IsValidGuid(baseGuid) || !IsValidGuid(newGuid)) {
            Logger::Error("PowerPlan: ClonePlan called with invalid GUID");
            return false;
        }

        if (!RunPowercfg("powercfg /duplicatescheme " + baseGuid + " " + newGuid)) {
            if (baseGuid != kBalancedGuid) {
                Logger::Warning("PowerPlan: Base plan " + baseGuid + " not found, trying Balanced");
                if (!RunPowercfg("powercfg /duplicatescheme " + std::string(kBalancedGuid) + " " + newGuid)) {
                    return false;
                }
            } else {
                return false;
            }
        }

        RunPowercfg("powercfg /changename " + newGuid + " "
                     + EscapeCmdArg(name) + " " + EscapeCmdArg("Vax Tweaker custom plan"));

        return PlanExists(newGuid);
    }

    bool PowerPlanManager::TunePlan(const std::string& guid, PlanIntensity intensity) {
        if (!IsValidGuid(guid)) {
            Logger::Error("PowerPlan: TunePlan called with invalid GUID");
            return false;
        }

        bool allOk = true;

        auto setValue = [&](const char* subgroup, const char* setting,
                           DWORD acValue, DWORD dcValue) {
            std::string acCmd = "powercfg /setacvalueindex " + guid + " "
                                + subgroup + " " + setting + " " + std::to_string(acValue);
            if (!RunPowercfg(acCmd)) allOk = false;

            std::string dcCmd = "powercfg /setdcvalueindex " + guid + " "
                                + subgroup + " " + setting + " " + std::to_string(dcValue);
            if (!RunPowercfg(dcCmd)) allOk = false;
        };

        switch (intensity) {
            case PlanIntensity::Low:
                setValue(kSubProcessor, kMinProcState,  5,  5);
                setValue(kSubProcessor, kMaxProcState, 100, 80);
                setValue(kSubProcessor, kProcBoostMode, 2, 4);
                setValue(kSubProcessor, kCoreParking, 100, 50);
                setValue(kSubProcessor, kIdleDisable, 0, 0);
                setValue(kSubVideo, kDisplayTimeout, 600, 180);
                setValue(kSubSleep, kSleepTimeout, 1800, 600);
                setValue(kSubSleep, kHibernateTimeout, 0, 3600);
                break;

            case PlanIntensity::Medium:
                setValue(kSubProcessor, kMinProcState, 10, 10);
                setValue(kSubProcessor, kMaxProcState, 100, 100);
                setValue(kSubProcessor, kProcBoostMode, 2, 2);
                setValue(kSubProcessor, kCoreParking, 100, 100);
                setValue(kSubProcessor, kIdleDisable, 0, 0);
                setValue(kSubVideo, kDisplayTimeout, 900, 300);
                setValue(kSubSleep, kSleepTimeout, 0, 900);
                setValue(kSubSleep, kHibernateTimeout, 0, 0);
                break;

            case PlanIntensity::High:
                setValue(kSubProcessor, kMinProcState, 100, 100);
                setValue(kSubProcessor, kMaxProcState, 100, 100);
                setValue(kSubProcessor, kProcBoostMode, 2, 2);
                setValue(kSubProcessor, kCoreParking, 100, 100);
                setValue(kSubProcessor, kIdleDisable, 1, 1);
                setValue(kSubVideo, kDisplayTimeout, 0, 600);
                setValue(kSubSleep, kSleepTimeout, 0, 0);
                setValue(kSubSleep, kHibernateTimeout, 0, 0);
                break;
        }

        return allOk;
    }

    bool PowerPlanManager::ActivatePlan(const std::string& guid) {
        if (!IsValidGuid(guid)) {
            Logger::Error("PowerPlan: ActivatePlan called with invalid GUID");
            return false;
        }
        return RunPowercfg("powercfg /setactive " + guid);
    }

}
