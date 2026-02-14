
#include "Compatibility.h"
#include "SystemProfile.h"
#include <unordered_set>

namespace Vax {

    static const std::unordered_set<std::string> s_powerTweaks = {
        "fps_core_parking",
        "fps_power_throttle",
        "fps_gpu_preemption",
        "fps_gpu_scheduling",
        "fps_cstates",
        "game_dynamic_tick",
        "game_platform_tick",
        "game_tsc_sync"
    };

    static const std::unordered_set<std::string> s_win11Tweaks = {
        "fps_snap_layouts"
    };

    static const std::unordered_set<std::string> s_idleImpactTweaks = {
        "fps_core_parking",
        "fps_power_throttle",
        "fps_cstates",
        "game_dynamic_tick",
        "game_platform_tick",
        "game_hibernation",
        "net_nic_power"
    };

    static const std::unordered_set<std::string> s_win11NetTweaks = {
        "net_doh"
    };

    static const std::unordered_set<std::string> s_netConnRiskTweaks = {
        "net_ipv6",
        "net_wlan_overhead"
    };

    static const std::unordered_set<std::string> s_gpuDriverTweaks = {
        "fps_gpu_scheduling",
        "fps_mpo",
        "fps_dwm_vsync",
        "fps_dwm_batch"
    };

    static const std::unordered_set<std::string> s_hwAccessTweaks = {
        "priv_camera",
        "priv_microphone"
    };

    static const std::unordered_set<std::string> s_securityRiskTweaks = {
        "game_spectre"
    };

    static const std::unordered_set<std::string> s_win11PrivacyTweaks = {
        "priv_copilot"
    };

    static const std::unordered_set<std::string> s_win11_24h2PrivacyTweaks = {
        "priv_recall"
    };

    std::string Compatibility::GetWarning(const std::string& tweakId) {
        const auto& prof = SystemProfile::GetCurrent();

        if (prof.batteryPresent && s_powerTweaks.count(tweakId)) {
            return "Laptop detected — this tweak may increase power draw and reduce battery life.";
        }

        if (prof.osBuild < 22000 && s_win11Tweaks.count(tweakId)) {
            return "Windows build < 22000 — this tweak targets Win11 features and may have no effect.";
        }

        if (prof.modernStandby.find("Supported") != std::string::npos &&
            s_idleImpactTweaks.count(tweakId)) {
            return "Modern Standby active — this tweak may reduce idle efficiency and increase heat.";
        }

        if (prof.gpuDriverVersion == "Unknown" && s_gpuDriverTweaks.count(tweakId)) {
            return "GPU driver version unknown — verify driver compatibility before applying.";
        }

        if (s_hwAccessTweaks.count(tweakId)) {
            return "This blocks system-wide hardware access — video-call and voice apps will not work until re-enabled.";
        }

        if (prof.osBuild < 22000 && s_win11PrivacyTweaks.count(tweakId)) {
            return "This feature is only present on Windows 11 — the tweak may have no effect on this build.";
        }

        if (prof.osBuild < 26100 && s_win11_24h2PrivacyTweaks.count(tweakId)) {
            return "Windows Recall requires Win11 24H2 (build 26100+) — the tweak has no effect on this build.";
        }

        if (s_securityRiskTweaks.count(tweakId)) {
            return "This disables CPU security mitigations — improves performance but exposes the system to side-channel attacks.";
        }

        if (prof.osBuild < 22000 && s_win11NetTweaks.count(tweakId)) {
            return "DNS over HTTPS auto-mode requires Windows 11 — may have no effect on this build.";
        }

        if (s_netConnRiskTweaks.count(tweakId)) {
            return "This may cause connectivity issues on some networks — revert if you lose connection.";
        }

        return "";
    }

    bool Compatibility::HasAnyWarning(const std::vector<TweakInfo>& tweaks) {
        for (const auto& t : tweaks) {
            if (!GetWarning(t.id).empty()) return true;
        }
        return false;
    }

    std::string Compatibility::GetBatchWarning(const std::vector<TweakInfo>& tweaks) {
        const auto& prof = SystemProfile::GetCurrent();
        int count = 0;
        for (const auto& t : tweaks) {
            if (!GetWarning(t.id).empty()) ++count;
        }
        if (count == 0) return "";

        std::string msg = std::to_string(count) + " tweak(s) may not be ideal for this system";
        if (prof.isLaptop) msg += " (laptop detected)";
        msg += ".";
        return msg;
    }

}
