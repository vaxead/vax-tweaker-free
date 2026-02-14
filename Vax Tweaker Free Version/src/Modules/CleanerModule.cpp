
#include "CleanerModule.h"
#include "../System/Logger.h"
#include "../System/ProcessUtils.h"
#include "../UI/Theme.h"
#include <shellapi.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <windows.h>

#pragma comment(lib, "shell32.lib")

namespace {

struct CleanResult {
  int filesDeleted = 0;
  int dirsDeleted = 0;
  int skipped = 0;
  ULONGLONG bytesFreed = 0;
};

std::string ToLongPath(const std::string &path) {
  if (path.size() >= 4 && path.substr(0, 4) == "\\\\?\\")
    return path;
  return "\\\\?\\" + path;
}

CleanResult ClearDirectoryContents(const std::string &path) {
  CleanResult result;
  std::string lp = ToLongPath(path);
  DWORD attr = GetFileAttributesA(lp.c_str());
  if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
    return result;
  }

  WIN32_FIND_DATAA fd;
  HANDLE hFind = FindFirstFileA((lp + "\\*").c_str(), &fd);
  if (hFind == INVALID_HANDLE_VALUE)
    return result;

  do {
    std::string name(fd.cFileName);
    if (name == "." || name == "..")
      continue;
    std::string fullPath = path + "\\" + name;
    std::string fullLp = ToLongPath(fullPath);

    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      CleanResult sub = ClearDirectoryContents(fullPath);
      result.filesDeleted += sub.filesDeleted;
      result.dirsDeleted += sub.dirsDeleted;
      result.skipped += sub.skipped;
      result.bytesFreed += sub.bytesFreed;
      if (RemoveDirectoryA(fullLp.c_str()))
        ++result.dirsDeleted;
      else
        ++result.skipped;
    } else {
      ULONGLONG fileSize =
          (static_cast<ULONGLONG>(fd.nFileSizeHigh) << 32) | fd.nFileSizeLow;
      if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        SetFileAttributesA(fullLp.c_str(), FILE_ATTRIBUTE_NORMAL);
      if (DeleteFileA(fullLp.c_str())) {
        ++result.filesDeleted;
        result.bytesFreed += fileSize;
      } else
        ++result.skipped;
    }
  } while (FindNextFileA(hFind, &fd));

  FindClose(hFind);
  return result;
}

void LogClean(const std::string &label, const CleanResult &r) {
  Vax::System::Logger::Info(label + ": deleted " +
                            std::to_string(r.filesDeleted) + " files, " +
                            std::to_string(r.dirsDeleted) + " dirs, skipped " +
                            std::to_string(r.skipped));
}

template <typename Fn> struct ScopeGuard {
  Fn func;
  bool dismissed = false;
  explicit ScopeGuard(Fn f) : func(std::move(f)) {}
  ~ScopeGuard() {
    if (!dismissed)
      func();
  }
  void Dismiss() { dismissed = true; }
  ScopeGuard(const ScopeGuard &) = delete;
  ScopeGuard &operator=(const ScopeGuard &) = delete;
};
template <typename Fn> ScopeGuard<Fn> MakeScopeGuard(Fn f) {
  return ScopeGuard<Fn>(std::move(f));
}

std::string GetEnvVar(const char *name) {
  char buf[MAX_PATH] = {};
  DWORD len = GetEnvironmentVariableA(name, buf, MAX_PATH);
  if (len == 0 || len >= MAX_PATH)
    return "";
  return std::string(buf);
}

std::string GetLocalAppData() { return GetEnvVar("LOCALAPPDATA"); }
std::string GetAppData() { return GetEnvVar("APPDATA"); }
std::string GetUserProfile() { return GetEnvVar("USERPROFILE"); }

std::string GetWinDir() {
  char buf[MAX_PATH] = {};
  GetWindowsDirectoryA(buf, MAX_PATH);
  return std::string(buf);
}

std::string GetProgramData() {
  char buf[MAX_PATH] = {};
  if (SHGetFolderPathA(nullptr, CSIDL_COMMON_APPDATA, nullptr, 0, buf) ==
      S_OK) {
    return std::string(buf);
  }
  std::string env = GetEnvVar("ProgramData");
  if (!env.empty())
    return env;
  std::string winDir = GetWinDir();
  return std::string(1, winDir[0]) + ":\\ProgramData";
}

std::string GetSteamPath() {
  HKEY hKey;
  if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 0, KEY_READ,
                    &hKey) == ERROR_SUCCESS) {
    char buf[MAX_PATH] = {};
    DWORD size = sizeof(buf);
    DWORD type = 0;
    if (RegQueryValueExA(hKey, "SteamPath", nullptr, &type,
                         reinterpret_cast<LPBYTE>(buf),
                         &size) == ERROR_SUCCESS &&
        (type == REG_SZ || type == REG_EXPAND_SZ) && size > 0) {
      RegCloseKey(hKey);
      std::string path(buf);
      for (char &c : path) {
        if (c == '/')
          c = '\\';
      }
      return path;
    }
    RegCloseKey(hKey);
  }
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Valve\\Steam",
                    0, KEY_READ, &hKey) == ERROR_SUCCESS) {
    char buf[MAX_PATH] = {};
    DWORD size = sizeof(buf);
    DWORD type = 0;
    if (RegQueryValueExA(hKey, "InstallPath", nullptr, &type,
                         reinterpret_cast<LPBYTE>(buf),
                         &size) == ERROR_SUCCESS &&
        (type == REG_SZ || type == REG_EXPAND_SZ) && size > 0) {
      RegCloseKey(hKey);
      return std::string(buf);
    }
    RegCloseKey(hKey);
  }
  return "";
}

std::string FormatBytes(ULONGLONG bytes) {
  if (bytes < 1024ULL)
    return std::to_string(bytes) + " B";
  if (bytes < 1024ULL * 1024)
    return std::to_string(bytes / 1024) + " KB";
  if (bytes < 1024ULL * 1024 * 1024) {
    double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
    char buf[32];
    sprintf_s(buf, "%.1f MB", mb);
    return std::string(buf);
  }
  double gb = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
  char buf[32];
  sprintf_s(buf, "%.2f GB", gb);
  return std::string(buf);
}

}

namespace Vax::Modules {

CleanerModule::CleanerModule()
    : BaseModule(3, "System Cleaner",
                 "Remove junk files, caches, and free disk space",
                 Vax::UI::Icon::Cleaner, ModuleCategory::Maintenance) {
  m_showTweakStatus = false;
  InitializeTweaks();
  InitGroups();
  m_isImplemented = true;
  m_requiresAdmin = true;
}

void CleanerModule::InitializeTweaks() {
  RegisterTweak({"clean_temp", "Clear Temporary Files",
                 "Remove Windows and user temp folder contents",
                 RiskLevel::Safe, TweakStatus::Unknown, false});
  RegisterTweak({"clean_prefetch", "Clear Prefetch Cache",
                 "Delete prefetch data to free space and reset cache",
                 RiskLevel::Moderate, TweakStatus::Unknown, false});
  RegisterTweak({"clean_thumbnails", "Clear Thumbnail Cache",
                 "Remove cached thumbnails to free disk space", RiskLevel::Safe,
                 TweakStatus::Unknown, false});
  RegisterTweak({"clean_winupdate", "Clear Windows Update Cache",
                 "Delete downloaded update files from SoftwareDistribution",
                 RiskLevel::Moderate, TweakStatus::Unknown, false});
  RegisterTweak({"clean_fontcache", "Clear Font Cache",
                 "Reset the Windows font cache to fix rendering issues",
                 RiskLevel::Safe, TweakStatus::Unknown, false});
  RegisterTweak({"clean_iconcache", "Rebuild Icon Cache",
                 "Delete and rebuild the icon cache database", RiskLevel::Safe,
                 TweakStatus::Unknown, false});
  RegisterTweak({"clean_shadercache", "Clear DirectX Shader Cache",
                 "Remove compiled shader cache to free space", RiskLevel::Safe,
                 TweakStatus::Unknown, false});

  RegisterTweak(
      {"clean_logs", "Clear System Logs",
       "Clear Windows event logs (Application, System, Security, Setup)",
       RiskLevel::Moderate, TweakStatus::Unknown, false});
  RegisterTweak({"clean_errorreports", "Clear Error Reports",
                 "Remove Windows Error Reporting data", RiskLevel::Safe,
                 TweakStatus::Unknown, false});
  RegisterTweak({"clean_crashdumps", "Clear Crash Dumps",
                 "Delete minidumps and kernel crash reports", RiskLevel::Safe,
                 TweakStatus::Unknown, false});
  RegisterTweak({"clean_windowsold", "Remove Windows.old",
                 "Delete the previous Windows installation folder",
                 RiskLevel::Advanced, TweakStatus::Unknown, false});
  RegisterTweak({"clean_deliveryopt", "Clear Delivery Optimization",
                 "Remove Windows Update delivery optimization cache",
                 RiskLevel::Moderate, TweakStatus::Unknown, false});
  RegisterTweak({"clean_installer", "Clear Windows Installer Cache",
                 "Remove orphaned MSI installer patches", RiskLevel::Moderate,
                 TweakStatus::Unknown, false});
  RegisterTweak({"clean_searchindex", "Reset Windows Search Index",
                 "Rebuild the Windows Search index database",
                 RiskLevel::Moderate, TweakStatus::Unknown, true});

  RegisterTweak({"clean_chrome", "Clear Chrome Cache",
                 "Remove Google Chrome browsing cache", RiskLevel::Safe,
                 TweakStatus::Unknown, false});
  RegisterTweak({"clean_edge", "Clear Edge Cache",
                 "Remove Microsoft Edge browsing cache", RiskLevel::Safe,
                 TweakStatus::Unknown, false});
  RegisterTweak({"clean_firefox", "Clear Firefox Cache",
                 "Remove Mozilla Firefox browsing cache", RiskLevel::Safe,
                 TweakStatus::Unknown, false});
  RegisterTweak({"clean_steam", "Clear Steam Cache",
                 "Remove Steam download cache and web browser data",
                 RiskLevel::Safe, TweakStatus::Unknown, false});
  RegisterTweak({"clean_nvidia", "Clear NVIDIA Cache",
                 "Remove NVIDIA shader cache and temp files", RiskLevel::Safe,
                 TweakStatus::Unknown, false});
  RegisterTweak({"clean_amd", "Clear AMD Cache",
                 "Remove AMD shader cache and temp files", RiskLevel::Safe,
                 TweakStatus::Unknown, false});
  RegisterTweak({"clean_office", "Clear Office Cache",
                 "Remove Microsoft Office temporary and cache files",
                 RiskLevel::Safe, TweakStatus::Unknown, false});
  RegisterTweak({"clean_teams", "Clear Teams Cache",
                 "Remove Microsoft Teams cache and temp data", RiskLevel::Safe,
                 TweakStatus::Unknown, false});
  RegisterTweak({"clean_discord", "Clear Discord Cache",
                 "Remove Discord cache, code cache, and GPU cache",
                 RiskLevel::Safe, TweakStatus::Unknown, false});

  RegisterTweak({"clean_vscode", "Clear VS Code Cache",
                 "Remove VS Code cache, cached data, and logs", RiskLevel::Safe,
                 TweakStatus::Unknown, false});
  RegisterTweak({"clean_npm", "Clear npm Cache",
                 "Remove npm package manager cache", RiskLevel::Safe,
                 TweakStatus::Unknown, false});
  RegisterTweak({"clean_pip", "Clear Python pip Cache",
                 "Remove pip package download cache", RiskLevel::Safe,
                 TweakStatus::Unknown, false});
  RegisterTweak({"clean_java", "Clear Java Cache",
                 "Remove Java Web Start and plugin cache files",
                 RiskLevel::Safe, TweakStatus::Unknown, false});

  RegisterTweak({"clean_spotify", "Clear Spotify Cache",
                 "Remove Spotify streaming cache (can be very large)",
                 RiskLevel::Safe, TweakStatus::Unknown, false});
  RegisterTweak({"clean_epic", "Clear Epic Games Cache",
                 "Remove Epic Games Launcher web cache", RiskLevel::Safe,
                 TweakStatus::Unknown, false});
  RegisterTweak({"clean_obs", "Clear OBS Logs & Crashes",
                 "Remove OBS Studio logs and crash reports", RiskLevel::Safe,
                 TweakStatus::Unknown, false});
  RegisterTweak({"clean_defender", "Clear Defender Scan History",
                 "Remove Windows Defender scan history files",
                 RiskLevel::Moderate, TweakStatus::Unknown, false});

  RegisterTweak({"clean_recyclebin", "Empty Recycle Bin",
                 "Permanently delete all items in the Recycle Bin",
                 RiskLevel::Safe, TweakStatus::Unknown, false});
  RegisterTweak({"clean_dns", "Flush DNS Cache", "Clear the DNS resolver cache",
                 RiskLevel::Safe, TweakStatus::Unknown, false});
}

void CleanerModule::InitGroups() {
  RegisterGroup(
      {"clean_grp_wincache",
       "Windows Cache",
       "\xf0\x9f\x97\x82",
       "System caches and temporary data",
       {"clean_temp", "clean_prefetch", "clean_thumbnails", "clean_winupdate",
        "clean_fontcache", "clean_iconcache", "clean_shadercache"}});

  RegisterGroup({"clean_grp_sysfiles",
                 "System Files",
                 "\xf0\x9f\x93\x81",
                 "Logs, dumps, and old system data",
                 {"clean_logs", "clean_errorreports", "clean_crashdumps",
                  "clean_windowsold", "clean_deliveryopt", "clean_installer",
                  "clean_searchindex"}});

  RegisterGroup({"clean_grp_appcache",
                 "Application Cache",
                 "\xf0\x9f\x92\xbe",
                 "Browser and application cache files",
                 {"clean_chrome", "clean_edge", "clean_firefox", "clean_steam",
                  "clean_nvidia", "clean_amd", "clean_office", "clean_teams",
                  "clean_discord"}});

  RegisterGroup({"clean_grp_devcache",
                 "Developer Cache",
                 "\xf0\x9f\x92\xbb",
                 "IDE, package manager, and runtime cache",
                 {"clean_vscode", "clean_npm", "clean_pip", "clean_java"}});

  RegisterGroup(
      {"clean_grp_mediacache",
       "Media & Gaming Cache",
       "\xf0\x9f\x8e\xb5",
       "Streaming, launcher, and security scan cache",
       {"clean_spotify", "clean_epic", "clean_obs", "clean_defender"}});

  RegisterGroup({"clean_grp_quick",
                 "Quick Actions",
                 "\xe2\x9a\xa1",
                 "One-click cleanup operations",
                 {"clean_recyclebin", "clean_dns"}});
}

void CleanerModule::RefreshStatus() {
  for (auto &tweak : m_tweaks) {
    tweak.status = TweakStatus::NotApplied;
  }
}

bool CleanerModule::RevertTweak(const std::string &tweakId) {
  System::Logger::Warning("Revert not supported for cleaning operations: " +
                          tweakId);
  return false;
}

bool CleanerModule::ApplyTweak(const std::string &tweakId) {
  if (tweakId == "clean_temp")
    return ClearTempFiles();
  if (tweakId == "clean_prefetch")
    return ClearPrefetch();
  if (tweakId == "clean_thumbnails")
    return ClearThumbnails();
  if (tweakId == "clean_winupdate")
    return ClearWindowsUpdate();
  if (tweakId == "clean_fontcache")
    return ClearFontCache();
  if (tweakId == "clean_iconcache")
    return RebuildIconCache();
  if (tweakId == "clean_shadercache")
    return ClearShaderCache();
  if (tweakId == "clean_logs")
    return ClearSystemLogs();
  if (tweakId == "clean_errorreports")
    return ClearErrorReports();
  if (tweakId == "clean_crashdumps")
    return ClearCrashDumps();
  if (tweakId == "clean_windowsold")
    return RemoveWindowsOld();
  if (tweakId == "clean_deliveryopt")
    return ClearDeliveryOptimization();
  if (tweakId == "clean_installer")
    return ClearInstallerCache();
  if (tweakId == "clean_searchindex")
    return ResetSearchIndex();
  if (tweakId == "clean_chrome")
    return ClearChromeCache();
  if (tweakId == "clean_edge")
    return ClearEdgeCache();
  if (tweakId == "clean_firefox")
    return ClearFirefoxCache();
  if (tweakId == "clean_steam")
    return ClearSteamCache();
  if (tweakId == "clean_nvidia")
    return ClearNvidiaCache();
  if (tweakId == "clean_amd")
    return ClearAmdCache();
  if (tweakId == "clean_office")
    return ClearOfficeCache();
  if (tweakId == "clean_teams")
    return ClearTeamsCache();
  if (tweakId == "clean_discord")
    return ClearDiscordCache();
  if (tweakId == "clean_vscode")
    return ClearVSCodeCache();
  if (tweakId == "clean_npm")
    return ClearNpmCache();
  if (tweakId == "clean_pip")
    return ClearPipCache();
  if (tweakId == "clean_java")
    return ClearJavaCache();
  if (tweakId == "clean_spotify")
    return ClearSpotifyCache();
  if (tweakId == "clean_epic")
    return ClearEpicCache();
  if (tweakId == "clean_obs")
    return ClearObsLogs();
  if (tweakId == "clean_defender")
    return ClearDefenderHistory();
  if (tweakId == "clean_recyclebin")
    return ClearRecycleBin();
  if (tweakId == "clean_dns")
    return FlushDnsCache();
  System::Logger::Error("Unknown cleaner tweak: " + tweakId);
  return false;
}

bool CleanerModule::ClearTempFiles() {
  CleanResult total;
  std::string userTemp = GetEnvVar("TEMP");
  if (!userTemp.empty()) {
    auto r = ClearDirectoryContents(userTemp);
    total.filesDeleted += r.filesDeleted;
    total.dirsDeleted += r.dirsDeleted;
    total.bytesFreed += r.bytesFreed;
  }
  auto r = ClearDirectoryContents(GetWinDir() + "\\Temp");
  total.filesDeleted += r.filesDeleted;
  total.dirsDeleted += r.dirsDeleted;
  total.bytesFreed += r.bytesFreed;
  System::Logger::Success("Clear Temporary Files: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0 || total.dirsDeleted > 0);
}

bool CleanerModule::ClearPrefetch() {
  auto r = ClearDirectoryContents(GetWinDir() + "\\Prefetch");
  System::Logger::Success("Clear Prefetch Cache: deleted " +
                          std::to_string(r.filesDeleted) + " files (" +
                          FormatBytes(r.bytesFreed) + " freed)");
  return (r.filesDeleted > 0);
}

bool CleanerModule::ClearThumbnails() {
  std::string dir = GetLocalAppData() + "\\Microsoft\\Windows\\Explorer";
  int deleted = 0;
  const char *patterns[] = {"\\thumbcache_*.db", "\\iconcache_*.db"};
  for (const char *pat : patterns) {
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA((dir + pat).c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE)
      continue;
    do {
      std::string fp = dir + "\\" + fd.cFileName;
      SetFileAttributesA(fp.c_str(), FILE_ATTRIBUTE_NORMAL);
      if (DeleteFileA(fp.c_str()))
        ++deleted;
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
  }
  System::Logger::Success("Clear Thumbnail Cache: deleted " +
                          std::to_string(deleted) + " files");
  return (deleted > 0);
}

bool CleanerModule::ClearWindowsUpdate() {
  bool stoppedWu = System::RunSilentCommand("net stop wuauserv");
  bool stoppedBits = System::RunSilentCommand("net stop bits");
  auto guard = MakeScopeGuard([&]() {
    if (stoppedBits)
      System::RunSilentCommand("net start bits");
    if (stoppedWu)
      System::RunSilentCommand("net start wuauserv");
  });
  if (!stoppedWu && !stoppedBits) {
    System::Logger::Error("Could not stop update services");
    guard.Dismiss();
    return false;
  }
  auto r =
      ClearDirectoryContents(GetWinDir() + "\\SoftwareDistribution\\Download");
  System::Logger::Success("Clear Windows Update Cache: deleted " +
                          std::to_string(r.filesDeleted) + " files (" +
                          FormatBytes(r.bytesFreed) + " freed)");
  return (r.filesDeleted > 0);
}

bool CleanerModule::ClearFontCache() {
  System::RunSilentCommand("net stop FontCache");
  auto guard =
      MakeScopeGuard([]() { System::RunSilentCommand("net start FontCache"); });
  auto r = ClearDirectoryContents(
      GetWinDir() +
      "\\ServiceProfiles\\LocalService\\AppData\\Local\\FontCache");
  std::string fontCache2 = GetWinDir() + "\\System32\\FNTCACHE.DAT";
  SetFileAttributesA(fontCache2.c_str(), FILE_ATTRIBUTE_NORMAL);
  int extraDeleted = DeleteFileA(fontCache2.c_str()) ? 1 : 0;
  System::Logger::Success("Clear Font Cache: deleted " +
                          std::to_string(r.filesDeleted + extraDeleted) +
                          " items (" + FormatBytes(r.bytesFreed) + " freed)");
  return true;
}

bool CleanerModule::RebuildIconCache() {
  std::string dir = GetLocalAppData() + "\\Microsoft\\Windows\\Explorer";
  WIN32_FIND_DATAA fd;
  int deleted = 0;
  HANDLE hFind = FindFirstFileA((dir + "\\iconcache_*.db").c_str(), &fd);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      std::string fp = dir + "\\" + fd.cFileName;
      SetFileAttributesA(fp.c_str(), FILE_ATTRIBUTE_NORMAL);
      if (DeleteFileA(fp.c_str()))
        ++deleted;
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
  }
  System::RunSilentCommand("ie4uinit.exe -show");
  System::Logger::Success("Rebuild Icon Cache: deleted " +
                          std::to_string(deleted) +
                          " files, rebuild triggered");
  return true;
}

bool CleanerModule::ClearShaderCache() {
  CleanResult total;
  auto r1 = ClearDirectoryContents(GetLocalAppData() + "\\D3DSCache");
  total.filesDeleted += r1.filesDeleted;
  total.dirsDeleted += r1.dirsDeleted;
  total.bytesFreed += r1.bytesFreed;
  auto r2 = ClearDirectoryContents(GetLocalAppData() + "\\NVIDIA\\DXCache");
  total.filesDeleted += r2.filesDeleted;
  total.dirsDeleted += r2.dirsDeleted;
  total.bytesFreed += r2.bytesFreed;
  auto r3 = ClearDirectoryContents(GetLocalAppData() + "\\AMD\\DXCache");
  total.filesDeleted += r3.filesDeleted;
  total.dirsDeleted += r3.dirsDeleted;
  total.bytesFreed += r3.bytesFreed;
  System::Logger::Success("Clear DirectX Shader Cache: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

bool CleanerModule::ClearSystemLogs() {
  int logCleared = 0;
  if (System::RunSilentCommand("wevtutil cl Application"))
    ++logCleared;
  if (System::RunSilentCommand("wevtutil cl System"))
    ++logCleared;
  if (System::RunSilentCommand("wevtutil cl Security"))
    ++logCleared;
  if (System::RunSilentCommand("wevtutil cl Setup"))
    ++logCleared;
  System::Logger::Success("Clear System Logs: cleared " +
                          std::to_string(logCleared) + "/4 event logs");
  return (logCleared > 0);
}

bool CleanerModule::ClearErrorReports() {
  CleanResult total;
  auto r1 = ClearDirectoryContents(GetLocalAppData() +
                                   "\\Microsoft\\Windows\\WER\\ReportQueue");
  total.filesDeleted += r1.filesDeleted;
  total.bytesFreed += r1.bytesFreed;
  auto r2 = ClearDirectoryContents(GetLocalAppData() +
                                   "\\Microsoft\\Windows\\WER\\ReportArchive");
  total.filesDeleted += r2.filesDeleted;
  total.bytesFreed += r2.bytesFreed;
  auto r3 = ClearDirectoryContents(GetProgramData() +
                                   "\\Microsoft\\Windows\\WER\\ReportQueue");
  total.filesDeleted += r3.filesDeleted;
  total.bytesFreed += r3.bytesFreed;
  System::Logger::Success("Clear Error Reports: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

bool CleanerModule::ClearCrashDumps() {
  CleanResult total;
  auto r1 = ClearDirectoryContents(GetLocalAppData() + "\\CrashDumps");
  total.filesDeleted += r1.filesDeleted;
  total.bytesFreed += r1.bytesFreed;
  auto r2 = ClearDirectoryContents(GetWinDir() + "\\Minidump");
  total.filesDeleted += r2.filesDeleted;
  total.bytesFreed += r2.bytesFreed;
  auto r3 = ClearDirectoryContents(GetWinDir() + "\\LiveKernelReports");
  total.filesDeleted += r3.filesDeleted;
  total.bytesFreed += r3.bytesFreed;
  std::string memDmp = GetWinDir() + "\\MEMORY.DMP";
  WIN32_FIND_DATAA dmpFd;
  HANDLE hDmp = FindFirstFileA(memDmp.c_str(), &dmpFd);
  if (hDmp != INVALID_HANDLE_VALUE) {
    ULONGLONG dmpSize = (static_cast<ULONGLONG>(dmpFd.nFileSizeHigh) << 32) |
                        dmpFd.nFileSizeLow;
    FindClose(hDmp);
    SetFileAttributesA(memDmp.c_str(), FILE_ATTRIBUTE_NORMAL);
    if (DeleteFileA(memDmp.c_str())) {
      total.filesDeleted++;
      total.bytesFreed += dmpSize;
    }
  }
  System::Logger::Success("Clear Crash Dumps: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

bool CleanerModule::RemoveWindowsOld() {
  std::string path = std::string(1, GetWinDir()[0]) + ":\\Windows.old";
  DWORD attr = GetFileAttributesA(path.c_str());
  if (attr == INVALID_FILE_ATTRIBUTES) {
    System::Logger::Info("Remove Windows.old: folder not found");
    return true;
  }
  bool ok = System::RunSilentCommand("rd /s /q \"" + path + "\"", 60000);
  if (ok)
    System::Logger::Success("Removed Windows.old");
  else
    System::Logger::Error(
        "Failed to remove Windows.old (may need elevated permissions)");
  return ok;
}

bool CleanerModule::ClearDeliveryOptimization() {
  System::RunSilentCommand("net stop DoSvc");
  auto guard =
      MakeScopeGuard([]() { System::RunSilentCommand("net start DoSvc"); });
  auto r = ClearDirectoryContents(
      GetWinDir() + "\\ServiceProfiles\\NetworkService\\AppData\\Local\\Microso"
                    "ft\\Windows\\DeliveryOptimization\\Cache");
  System::Logger::Success("Clear Delivery Optimization: deleted " +
                          std::to_string(r.filesDeleted) + " files (" +
                          FormatBytes(r.bytesFreed) + " freed)");
  return (r.filesDeleted > 0);
}

bool CleanerModule::ClearInstallerCache() {
  auto r = ClearDirectoryContents(GetWinDir() + "\\Installer\\$PatchCache$");
  System::Logger::Success("Clear Windows Installer Cache: deleted " +
                          std::to_string(r.filesDeleted) + " files (" +
                          FormatBytes(r.bytesFreed) + " freed)");
  return (r.filesDeleted > 0);
}

bool CleanerModule::ResetSearchIndex() {
  bool stopped = System::RunSilentCommand("net stop WSearch");
  auto guard = MakeScopeGuard([&]() {
    if (stopped)
      System::RunSilentCommand("net start WSearch");
  });
  if (!stopped) {
    System::Logger::Error("Could not stop Windows Search service");
    guard.Dismiss();
    return false;
  }
  std::string indexPath =
      GetProgramData() + "\\Microsoft\\Search\\Data\\Applications\\Windows";
  auto r = ClearDirectoryContents(indexPath);
  System::Logger::Success(
      "Reset Windows Search Index: deleted " + std::to_string(r.filesDeleted) +
      " files (" + FormatBytes(r.bytesFreed) + " freed), service restarted");
  return (r.filesDeleted > 0);
}

bool CleanerModule::ClearChromeCache() {
  std::string base = GetLocalAppData() + "\\Google\\Chrome\\User Data\\Default";
  CleanResult total;
  auto r1 = ClearDirectoryContents(base + "\\Cache\\Cache_Data");
  total.filesDeleted += r1.filesDeleted;
  total.bytesFreed += r1.bytesFreed;
  auto r2 = ClearDirectoryContents(base + "\\Code Cache");
  total.filesDeleted += r2.filesDeleted;
  total.bytesFreed += r2.bytesFreed;
  auto r3 = ClearDirectoryContents(base + "\\GPUCache");
  total.filesDeleted += r3.filesDeleted;
  total.bytesFreed += r3.bytesFreed;
  System::Logger::Success("Clear Chrome Cache: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

bool CleanerModule::ClearEdgeCache() {
  std::string base =
      GetLocalAppData() + "\\Microsoft\\Edge\\User Data\\Default";
  CleanResult total;
  auto r1 = ClearDirectoryContents(base + "\\Cache\\Cache_Data");
  total.filesDeleted += r1.filesDeleted;
  total.bytesFreed += r1.bytesFreed;
  auto r2 = ClearDirectoryContents(base + "\\Code Cache");
  total.filesDeleted += r2.filesDeleted;
  total.bytesFreed += r2.bytesFreed;
  auto r3 = ClearDirectoryContents(base + "\\GPUCache");
  total.filesDeleted += r3.filesDeleted;
  total.bytesFreed += r3.bytesFreed;
  System::Logger::Success("Clear Edge Cache: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

bool CleanerModule::ClearFirefoxCache() {
  std::string profiles = GetLocalAppData() + "\\Mozilla\\Firefox\\Profiles";
  CleanResult total;
  WIN32_FIND_DATAA fd;
  HANDLE hFind = FindFirstFileA((profiles + "\\*").c_str(), &fd);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (fd.cFileName[0] == '.')
        continue;
      if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        continue;
      auto r = ClearDirectoryContents(profiles + "\\" + fd.cFileName +
                                      "\\cache2\\entries");
      total.filesDeleted += r.filesDeleted;
      total.bytesFreed += r.bytesFreed;
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
  }
  System::Logger::Success("Clear Firefox Cache: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

bool CleanerModule::ClearSteamCache() {
  std::string steam = GetSteamPath();
  if (steam.empty()) {
    System::Logger::Info("Clear Steam Cache: Steam installation not found");
    return false;
  }
  CleanResult total;
  auto r1 = ClearDirectoryContents(steam + "\\appcache\\httpcache");
  total.filesDeleted += r1.filesDeleted;
  total.bytesFreed += r1.bytesFreed;
  auto r2 = ClearDirectoryContents(steam + "\\appcache\\librarycache");
  total.filesDeleted += r2.filesDeleted;
  total.bytesFreed += r2.bytesFreed;
  System::Logger::Success("Clear Steam Cache: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

bool CleanerModule::ClearNvidiaCache() {
  CleanResult total;
  auto r1 = ClearDirectoryContents(GetLocalAppData() + "\\NVIDIA\\DXCache");
  total.filesDeleted += r1.filesDeleted;
  total.bytesFreed += r1.bytesFreed;
  auto r2 = ClearDirectoryContents(GetLocalAppData() + "\\NVIDIA\\GLCache");
  total.filesDeleted += r2.filesDeleted;
  total.bytesFreed += r2.bytesFreed;
  auto r3 = ClearDirectoryContents(GetEnvVar("TEMP") + "\\NVIDIA Corporation");
  total.filesDeleted += r3.filesDeleted;
  total.bytesFreed += r3.bytesFreed;
  System::Logger::Success("Clear NVIDIA Cache: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

bool CleanerModule::ClearAmdCache() {
  CleanResult total;
  auto r1 = ClearDirectoryContents(GetLocalAppData() + "\\AMD\\DXCache");
  total.filesDeleted += r1.filesDeleted;
  total.bytesFreed += r1.bytesFreed;
  auto r2 = ClearDirectoryContents(GetLocalAppData() + "\\AMD\\GLCache");
  total.filesDeleted += r2.filesDeleted;
  total.bytesFreed += r2.bytesFreed;
  System::Logger::Success("Clear AMD Cache: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

bool CleanerModule::ClearOfficeCache() {
  CleanResult total;
  auto r1 = ClearDirectoryContents(
      GetLocalAppData() + "\\Microsoft\\Office\\16.0\\OfficeFileCache");
  total.filesDeleted += r1.filesDeleted;
  total.bytesFreed += r1.bytesFreed;
  auto r2 = ClearDirectoryContents(GetLocalAppData() +
                                   "\\Microsoft\\Outlook\\RoamCache");
  total.filesDeleted += r2.filesDeleted;
  total.bytesFreed += r2.bytesFreed;
  System::Logger::Success("Clear Office Cache: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

bool CleanerModule::ClearTeamsCache() {
  std::string teamsBase = GetAppData() + "\\Microsoft\\Teams";
  CleanResult total;
  const char *dirs[] = {"\\Cache",    "\\blob_storage", "\\databases",
                        "\\GPUCache", "\\IndexedDB",    "\\Local Storage",
                        "\\tmp"};
  for (const char *d : dirs) {
    auto r = ClearDirectoryContents(teamsBase + d);
    total.filesDeleted += r.filesDeleted;
    total.bytesFreed += r.bytesFreed;
  }
  std::string teams2 =
      GetLocalAppData() +
      "\\Packages\\MSTeams_8wekyb3d8bbwe\\LocalCache\\Microsoft\\MSTeams";
  auto r2 = ClearDirectoryContents(teams2);
  total.filesDeleted += r2.filesDeleted;
  total.bytesFreed += r2.bytesFreed;
  System::Logger::Success("Clear Teams Cache: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

bool CleanerModule::ClearDiscordCache() {
  std::string base = GetAppData() + "\\discord";
  CleanResult total;
  auto r1 = ClearDirectoryContents(base + "\\Cache\\Cache_Data");
  total.filesDeleted += r1.filesDeleted;
  total.bytesFreed += r1.bytesFreed;
  auto r2 = ClearDirectoryContents(base + "\\Code Cache");
  total.filesDeleted += r2.filesDeleted;
  total.bytesFreed += r2.bytesFreed;
  auto r3 = ClearDirectoryContents(base + "\\GPUCache");
  total.filesDeleted += r3.filesDeleted;
  total.bytesFreed += r3.bytesFreed;
  System::Logger::Success("Clear Discord Cache: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

bool CleanerModule::ClearRecycleBin() {
  HRESULT hr = SHEmptyRecycleBinA(nullptr, nullptr,
                                  SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI |
                                      SHERB_NOSOUND);
  if (SUCCEEDED(hr) || hr == E_UNEXPECTED) {
    System::Logger::Success("Applied: Empty Recycle Bin");
    return true;
  }
  System::Logger::Error(
      "Failed: Empty Recycle Bin (HRESULT: " + std::to_string(hr) + ")");
  return false;
}

bool CleanerModule::FlushDnsCache() {
  typedef BOOL(WINAPI * DnsFlushFn)();
  HMODULE hDns = LoadLibraryA("dnsapi.dll");
  if (hDns) {
    auto pFlush = reinterpret_cast<DnsFlushFn>(
        GetProcAddress(hDns, "DnsFlushResolverCache"));
    bool flushed = (pFlush != nullptr && pFlush());
    FreeLibrary(hDns);
    if (flushed) {
      System::Logger::Success("Applied: Flush DNS Cache");
      return true;
    }
  }
  bool ok = System::RunSilentCommand("ipconfig /flushdns");
  if (ok)
    System::Logger::Success("Applied: Flush DNS Cache (via ipconfig)");
  else
    System::Logger::Error("Failed: Flush DNS Cache");
  return ok;
}

bool CleanerModule::ClearVSCodeCache() {
  CleanResult total;
  std::string appData = GetAppData();
  if (!appData.empty()) {
    std::string dirs[] = {
        appData + "\\Code\\Cache", appData + "\\Code\\CachedData",
        appData + "\\Code\\CachedExtensionVSIXs", appData + "\\Code\\logs"};
    for (const auto &d : dirs) {
      auto r = ClearDirectoryContents(d);
      total.filesDeleted += r.filesDeleted;
      total.bytesFreed += r.bytesFreed;
    }
  }
  System::Logger::Success("Clear VS Code Cache: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

bool CleanerModule::ClearNpmCache() {
  CleanResult total;
  std::string npmCache = GetAppData() + "\\npm-cache";
  if (!npmCache.empty()) {
    auto r = ClearDirectoryContents(npmCache);
    total.filesDeleted += r.filesDeleted;
    total.bytesFreed += r.bytesFreed;
  }
  System::Logger::Success("Clear npm Cache: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

bool CleanerModule::ClearPipCache() {
  CleanResult total;
  std::string pipCache = GetLocalAppData() + "\\pip\\cache";
  if (!pipCache.empty()) {
    auto r = ClearDirectoryContents(pipCache);
    total.filesDeleted += r.filesDeleted;
    total.bytesFreed += r.bytesFreed;
  }
  System::Logger::Success("Clear pip Cache: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

bool CleanerModule::ClearJavaCache() {
  CleanResult total;
  std::string javaCache = GetAppData() + "\\Sun\\Java\\Deployment\\cache";
  if (!javaCache.empty()) {
    auto r = ClearDirectoryContents(javaCache);
    total.filesDeleted += r.filesDeleted;
    total.bytesFreed += r.bytesFreed;
  }
  std::string userProfile = GetUserProfile();
  if (!userProfile.empty()) {
    auto r = ClearDirectoryContents(
        userProfile + "\\AppData\\LocalLow\\Sun\\Java\\Deployment\\cache");
    total.filesDeleted += r.filesDeleted;
    total.bytesFreed += r.bytesFreed;
  }
  System::Logger::Success("Clear Java Cache: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

bool CleanerModule::ClearSpotifyCache() {
  CleanResult total;
  std::string spotifyStorage = GetLocalAppData() + "\\Spotify\\Storage";
  if (!spotifyStorage.empty()) {
    auto r = ClearDirectoryContents(spotifyStorage);
    total.filesDeleted += r.filesDeleted;
    total.bytesFreed += r.bytesFreed;
  }
  std::string spotifyData = GetLocalAppData() + "\\Spotify\\Data";
  if (!spotifyData.empty()) {
    auto r = ClearDirectoryContents(spotifyData);
    total.filesDeleted += r.filesDeleted;
    total.bytesFreed += r.bytesFreed;
  }
  System::Logger::Success("Clear Spotify Cache: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

bool CleanerModule::ClearEpicCache() {
  CleanResult total;
  std::string epicCache =
      GetLocalAppData() + "\\EpicGamesLauncher\\Saved\\webcache";
  if (!epicCache.empty()) {
    auto r = ClearDirectoryContents(epicCache);
    total.filesDeleted += r.filesDeleted;
    total.bytesFreed += r.bytesFreed;
  }
  std::string epicLogs = GetLocalAppData() + "\\EpicGamesLauncher\\Saved\\Logs";
  if (!epicLogs.empty()) {
    auto r = ClearDirectoryContents(epicLogs);
    total.filesDeleted += r.filesDeleted;
    total.bytesFreed += r.bytesFreed;
  }
  System::Logger::Success("Clear Epic Games Cache: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

bool CleanerModule::ClearObsLogs() {
  CleanResult total;
  std::string appData = GetAppData();
  if (!appData.empty()) {
    auto r1 = ClearDirectoryContents(appData + "\\obs-studio\\logs");
    total.filesDeleted += r1.filesDeleted;
    total.bytesFreed += r1.bytesFreed;
    auto r2 = ClearDirectoryContents(appData + "\\obs-studio\\crashes");
    total.filesDeleted += r2.filesDeleted;
    total.bytesFreed += r2.bytesFreed;
  }
  System::Logger::Success("Clear OBS Logs: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

bool CleanerModule::ClearDefenderHistory() {
  CleanResult total;
  std::string programData = GetProgramData();
  if (!programData.empty()) {
    auto r = ClearDirectoryContents(
        programData + "\\Microsoft\\Windows Defender\\Scans\\History\\Results");
    total.filesDeleted += r.filesDeleted;
    total.bytesFreed += r.bytesFreed;
  }
  System::Logger::Success("Clear Defender Scan History: deleted " +
                          std::to_string(total.filesDeleted) + " files (" +
                          FormatBytes(total.bytesFreed) + " freed)");
  return (total.filesDeleted > 0);
}

}
