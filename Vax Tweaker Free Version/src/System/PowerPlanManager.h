
#pragma once

#include <string>
#include <vector>

namespace Vax::System {

    enum class PlanIntensity {
        Low,
        Medium,
        High
    };

    struct VaxPowerPlan {
        std::string displayName;
        std::string description;
        PlanIntensity intensity;
        std::string guid;
        bool isActive = false;
    };

    struct SystemPowerPlan {
        std::string guid;
        std::string name;
        bool isActive = false;
    };

    class PowerPlanManager {
    public:
        static std::vector<VaxPowerPlan> GetVaxPlans();

        static bool InstallAndActivate(int planIndex);

        static std::vector<SystemPowerPlan> ListAllPlans();

        static bool RemovePlan(const std::string& guid);

        static int RemoveUnusedPlans();

        static std::string GetActiveSchemeGuid();

    private:
        PowerPlanManager() = default;

        static constexpr const char* kBalancedGuid      = "381b4222-f694-41f0-9685-ff5bb260df2e";
        static constexpr const char* kHighPerfGuid      = "8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c";
        static constexpr const char* kPowerSaverGuid    = "a1841308-3541-4fab-bc81-f71556f20b4a";

        static constexpr const char* kVaxLaptopGuid     = "daa10f76-1b2c-4a5e-8f3d-7e6a9b0c1d2e";
        static constexpr const char* kVaxBalancedGuid   = "baa20e65-2c3d-5b6f-9e4c-8d5b0a1e2f3a";
        static constexpr const char* kVaxPerfMaxGuid    = "caa30d54-3d4e-6c70-af5b-9c6a1b2f3e4b";

        static bool PlanExists(const std::string& guid);

        static bool PlanExistsInCache(const std::string& guid, const std::string& cachedList);

        static bool ClonePlan(const std::string& baseGuid, const std::string& newGuid,
                              const std::string& name);

        static bool TunePlan(const std::string& guid, PlanIntensity intensity);

        static bool ActivatePlan(const std::string& guid);

        static bool RunPowercfg(const std::string& args);

        static std::string RunPowercfgCapture(const std::string& args);

        static bool IsValidGuid(const std::string& s);

        static std::string ExtractGuid(const std::string& text);

        static std::string EscapeCmdArg(const std::string& arg);

        static constexpr const char* kSubProcessor      = "54533251-82be-4824-96c1-47b60b740d00";
        static constexpr const char* kMinProcState      = "893dee8e-2bef-41e0-89c6-b55d0929964c";
        static constexpr const char* kMaxProcState      = "bc5038f7-23e0-4960-96da-33abaf5935ec";
        static constexpr const char* kProcBoostMode     = "be337238-0d82-4146-a960-4f3749d470c7";
        static constexpr const char* kCoreParking       = "0cc5b647-c1df-4637-891a-dec35c318583";
        static constexpr const char* kIdleDisable       = "5d76a2ca-e8c0-402f-a133-2158492d58ad";
        static constexpr const char* kSubSleep          = "238c9fa8-0aad-41ed-83f4-97be242c8f20";
        static constexpr const char* kHibernateTimeout  = "9d7815a6-7ee4-497e-8888-515a05f02364";
        static constexpr const char* kSleepTimeout      = "29f6c1db-86da-48c5-9fdb-f2b67b1f44da";
        static constexpr const char* kSubVideo          = "7516b95f-f776-4464-8c53-06167f40cc99";
        static constexpr const char* kDisplayTimeout    = "3c0bc021-c8a8-4e07-a973-6b14cbcb2b7e";
    };

}
