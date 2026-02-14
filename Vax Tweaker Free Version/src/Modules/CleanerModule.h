
#pragma once

#include "BaseModule.h"

namespace Vax::Modules {

    class CleanerModule : public BaseModule {
    public:
        CleanerModule();
        ~CleanerModule() override = default;

        bool ApplyTweak(const std::string& tweakId) override;
        bool RevertTweak(const std::string& tweakId) override;
        void RefreshStatus() override;

    private:
        void InitializeTweaks();
        void InitGroups();

        bool ClearTempFiles();
        bool ClearPrefetch();
        bool ClearThumbnails();
        bool ClearWindowsUpdate();
        bool ClearFontCache();
        bool RebuildIconCache();
        bool ClearShaderCache();

        bool ClearSystemLogs();
        bool ClearErrorReports();
        bool ClearCrashDumps();
        bool RemoveWindowsOld();
        bool ClearDeliveryOptimization();
        bool ClearInstallerCache();
        bool ResetSearchIndex();

        bool ClearChromeCache();
        bool ClearEdgeCache();
        bool ClearFirefoxCache();
        bool ClearSteamCache();
        bool ClearNvidiaCache();
        bool ClearAmdCache();
        bool ClearOfficeCache();
        bool ClearTeamsCache();
        bool ClearDiscordCache();

        bool ClearVSCodeCache();
        bool ClearNpmCache();
        bool ClearPipCache();
        bool ClearJavaCache();

        bool ClearSpotifyCache();
        bool ClearEpicCache();
        bool ClearObsLogs();
        bool ClearDefenderHistory();

        bool ClearRecycleBin();
        bool FlushDnsCache();
    };

}
