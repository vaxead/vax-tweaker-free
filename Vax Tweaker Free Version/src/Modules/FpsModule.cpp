
#include "FpsModule.h"
#include "../Core/Admin.h"
#include "../System/Logger.h"
#include "../System/ProcessUtils.h"
#include "../System/Registry.h"
#include "../UI/Console.h"
#include "../UI/Renderer.h"
#include "../UI/Theme.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

namespace {

constexpr const char *kPciExpressSubgroup =
    "501a4d13-42af-4429-9fd1-a8218c268e20";
constexpr const char *kAspmSetting = "ee12f906-d277-404b-b6da-e5fa1a576df5";

static bool IsMicrosoftServicePath(const std::string &imagePath) {
  if (imagePath.empty())
    return true;

  char expanded[2048] = {};
  DWORD expandedLen =
      ExpandEnvironmentStringsA(imagePath.c_str(), expanded, sizeof(expanded));
  std::string lower = (expandedLen > 0 && expandedLen < sizeof(expanded))
                          ? std::string(expanded)
                          : imagePath;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

  if (lower.find("\\windows\\") != std::string::npos)
    return true;
  if (lower.find("%systemroot%") != std::string::npos)
    return true;
  if (lower.find("\\system32\\") != std::string::npos)
    return true;
  if (lower.find("\\syswow64\\") != std::string::npos)
    return true;
  if (lower.find("\\winsxs\\") != std::string::npos)
    return true;
  if (lower.find("\\microsoft") != std::string::npos)
    return true;
  if (lower.find("svchost.exe") != std::string::npos)
    return true;
  if (lower.find("\\drivers\\") != std::string::npos)
    return true;

  return false;
}

static std::string GetDisabledServicesListPath() {
  return Vax::System::Registry::GetAppDataDir() + "\\vax_disabled_services.txt";
}

static bool SaveDisabledServicesList(const std::vector<std::string> &services) {
  std::ofstream f(GetDisabledServicesListPath());
  if (!f.is_open())
    return false;
  for (const auto &svc : services)
    f << svc << "\n";
  return true;
}

static std::vector<std::string> LoadDisabledServicesList() {
  std::vector<std::string> result;
  std::ifstream f(GetDisabledServicesListPath());
  if (!f.is_open())
    return result;
  std::string line;
  while (std::getline(f, line)) {
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
      line.pop_back();
    if (!line.empty())
      result.push_back(line);
  }
  return result;
}

static void DeleteDisabledServicesList() {
  DeleteFileA(GetDisabledServicesListPath().c_str());
}

static std::vector<std::string> EnumerateThirdPartyServices() {
  std::vector<std::string> result;

  HKEY hServicesKey;
  LONG rc =
      RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services",
                    0, KEY_READ | KEY_WOW64_64KEY, &hServicesKey);
  if (rc != ERROR_SUCCESS)
    return result;

  char nameBuffer[256];
  DWORD nameLen;
  DWORD index = 0;

  while (true) {
    nameLen = sizeof(nameBuffer);
    rc = RegEnumKeyExA(hServicesKey, index++, nameBuffer, &nameLen, nullptr,
                       nullptr, nullptr, nullptr);
    if (rc != ERROR_SUCCESS)
      break;

    std::string svcName(nameBuffer, nameLen);
    std::string subKey = "SYSTEM\\CurrentControlSet\\Services\\" + svcName;

    HKEY hSvc;
    rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey.c_str(), 0,
                       KEY_READ | KEY_WOW64_64KEY, &hSvc);
    if (rc != ERROR_SUCCESS)
      continue;

    DWORD svcType = 0, startType = 0;
    DWORD dataSize = sizeof(DWORD);

    RegQueryValueExA(hSvc, "Type", nullptr, nullptr,
                     reinterpret_cast<LPBYTE>(&svcType), &dataSize);
    dataSize = sizeof(DWORD);
    RegQueryValueExA(hSvc, "Start", nullptr, nullptr,
                     reinterpret_cast<LPBYTE>(&startType), &dataSize);

    char pathBuffer[1024] = {};
    DWORD pathSize = sizeof(pathBuffer) - 1;
    DWORD pathType = 0;
    RegQueryValueExA(hSvc, "ImagePath", nullptr, &pathType,
                     reinterpret_cast<LPBYTE>(pathBuffer), &pathSize);

    RegCloseKey(hSvc);

    bool isWin32Service = (svcType & 0x10) || (svcType & 0x20);
    if (!isWin32Service)
      continue;

    if (startType == 4)
      continue;
    if (startType == 0 || startType == 1)
      continue;

    std::string imagePath(pathBuffer);
    if (IsMicrosoftServicePath(imagePath))
      continue;

    result.push_back(svcName);
  }

  RegCloseKey(hServicesKey);
  return result;
}

}

namespace Vax::Modules {

FpsModule::FpsModule()
    : BaseModule(1, "FPS & Rendering",
                 "Graphics performance, latency, system tweaks",
                 Vax::UI::Icon::Fps, ModuleCategory::Performance) {
  InitializeTweaks();
  m_isImplemented = true;
  m_requiresAdmin = true;
  m_showTweakStatus = true;
}

void FpsModule::InitializeTweaks() {

  {
    RegistryTarget t1;
    t1.root = HKEY_CURRENT_USER;
    t1.subKey = "System\\GameConfigStore";
    t1.valueName = "GameDVR_FSEBehavior";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 2;
    t1.expectedDword = 2;
    t1.defaultDword = 0;

    RegistryTarget t2;
    t2.root = HKEY_CURRENT_USER;
    t2.subKey = "System\\GameConfigStore";
    t2.valueName = "GameDVR_FSEBehaviorMode";
    t2.valueType = RegValueType::Dword;
    t2.applyDword = 2;
    t2.expectedDword = 2;
    t2.defaultDword = 0;

    RegisterTweak(
        {"fps_fullscreen_opt",
         "Disable Fullscreen Optimizations",
         "Prevents Windows from applying display optimizations to fullscreen "
         "apps",
         RiskLevel::Safe,
         TweakStatus::Unknown,
         false,
         {t1, t2}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_LOCAL_MACHINE;
    t1.subKey = "SOFTWARE\\Microsoft\\Windows\\Dwm";
    t1.valueName = "OverlayTestMode";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 5;
    t1.expectedDword = 5;
    t1.deleteOnRevert = true;

    RegisterTweak({"fps_mpo",
                   "Disable Multi-Plane Overlay",
                   "Removes DWM MPO which can cause micro-stutters and "
                   "frame-pacing issues",
                   RiskLevel::Moderate,
                   TweakStatus::Unknown,
                   false,
                   {t1}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_CURRENT_USER;
    t1.subKey =
        "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
    t1.valueName = "EnableTransparency";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 0;
    t1.expectedDword = 0;
    t1.defaultDword = 1;

    RegisterTweak({"fps_transparency",
                   "Disable Transparency Effects",
                   "Removes glass/blur transparency effects to reduce GPU "
                   "compositor load",
                   RiskLevel::Safe,
                   TweakStatus::Unknown,
                   false,
                   {t1}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_LOCAL_MACHINE;
    t1.subKey = "SOFTWARE\\Microsoft\\Windows\\Dwm";
    t1.valueName = "EnableFrameServerMode";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 0;
    t1.expectedDword = 0;
    t1.deleteOnRevert = true;

    RegisterTweak({"fps_dwm_batch",
                   "DWM Batch Flushing",
                   "Disables DWM frame server mode to reduce compositor "
                   "latency and improve frame pacing",
                   RiskLevel::Moderate,
                   TweakStatus::Unknown,
                   false,
                   {t1}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_CURRENT_USER;
    t1.subKey =
        "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VisualEffects";
    t1.valueName = "VisualFXSetting";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 2;
    t1.expectedDword = 2;
    t1.defaultDword = 0;

    RegisterTweak({"fps_visual_effects",
                   "Optimize Visual Effects",
                   "Sets visual effects to 'Adjust for best performance' to "
                   "free GPU resources",
                   RiskLevel::Safe,
                   TweakStatus::Unknown,
                   false,
                   {t1}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_CURRENT_USER;
    t1.subKey = "Control Panel\\Desktop\\WindowMetrics";
    t1.valueName = "MinAnimate";
    t1.valueType = RegValueType::String;
    t1.applyString = "0";
    t1.expectedString = "0";
    t1.defaultString = "1";

    RegisterTweak(
        {"fps_animations",
         "Disable Window Animations",
         "Turns off minimize/maximize animations for snappier UI response",
         RiskLevel::Safe,
         TweakStatus::Unknown,
         false,
         {t1}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_LOCAL_MACHINE;
    t1.subKey = "SOFTWARE\\Microsoft\\Windows\\Dwm";
    t1.valueName = "MaxQueuedFrames";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 2;
    t1.expectedDword = 2;
    t1.deleteOnRevert = true;

    RegisterTweak(
        {"fps_dwm_maxqueued",
         "DWM Max Queued Buffers",
         "Reduces DWM frame queue depth to 2 for lower display latency",
         RiskLevel::Moderate,
         TweakStatus::Unknown,
         false,
         {t1}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_CURRENT_USER;
    t1.subKey = "Control Panel\\Desktop";
    t1.valueName = "HungAppTimeout";
    t1.valueType = RegValueType::String;
    t1.applyString = "10000";
    t1.expectedString = "10000";
    t1.defaultString = "5000";

    RegisterTweak({"fps_disable_ghosting",
                   "Disable Window Ghosting",
                   "Extends the hang timeout to prevent 'Not Responding' "
                   "ghosting during heavy GPU loads",
                   RiskLevel::Safe,
                   TweakStatus::Unknown,
                   false,
                   {t1}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_CURRENT_USER;
    t1.subKey = "System\\GameConfigStore";
    t1.valueName = "GameDVR_Enabled";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 0;
    t1.expectedDword = 0;
    t1.defaultDword = 1;

    RegistryTarget t2;
    t2.root = HKEY_CURRENT_USER;
    t2.subKey = "Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR";
    t2.valueName = "AppCaptureEnabled";
    t2.valueType = RegValueType::Dword;
    t2.applyDword = 0;
    t2.expectedDword = 0;
    t2.defaultDword = 1;

    RegistryTarget t3;
    t3.root = HKEY_LOCAL_MACHINE;
    t3.subKey = "SOFTWARE\\Policies\\Microsoft\\Windows\\GameDVR";
    t3.valueName = "AllowGameDVR";
    t3.valueType = RegValueType::Dword;
    t3.applyDword = 0;
    t3.expectedDword = 0;
    t3.deleteOnRevert = true;

    RegisterTweak({"fps_game_bar",
                   "Disable Game Bar / DVR",
                   "Stops the Game Bar overlay and background recording to "
                   "reclaim GPU and CPU cycles",
                   RiskLevel::Safe,
                   TweakStatus::Unknown,
                   false,
                   {t1, t2, t3}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_CURRENT_USER;
    t1.subKey =
        "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
    t1.valueName = "EnableSnapAssistFlyout";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 0;
    t1.expectedDword = 0;
    t1.defaultDword = 1;

    RegisterTweak(
        {"fps_snap_layouts",
         "Disable Snap Layouts",
         "Removes the snap layout overlay when hovering the maximize button",
         RiskLevel::Safe,
         TweakStatus::Unknown,
         false,
         {t1}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_CURRENT_USER;
    t1.subKey =
        "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
    t1.valueName = "DisallowShaking";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 1;
    t1.expectedDword = 1;
    t1.defaultDword = 0;

    RegisterTweak({"fps_aero_shake",
                   "Disable Aero Shake",
                   "Prevents the title-bar shake gesture from minimizing other "
                   "windows",
                   RiskLevel::Safe,
                   TweakStatus::Unknown,
                   false,
                   {t1}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_CURRENT_USER;
    t1.subKey = "Software\\Microsoft\\GameBar";
    t1.valueName = "AutoGameModeEnabled";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 1;
    t1.expectedDword = 1;
    t1.defaultDword = 1;

    RegistryTarget t2;
    t2.root = HKEY_CURRENT_USER;
    t2.subKey = "Software\\Microsoft\\GameBar";
    t2.valueName = "AllowAutoGameMode";
    t2.valueType = RegValueType::Dword;
    t2.applyDword = 1;
    t2.expectedDword = 1;
    t2.defaultDword = 1;

    RegisterTweak(
        {"fps_game_mode",
         "Optimize Game Mode",
         "Ensures Windows Game Mode is active for better CPU/GPU scheduling",
         RiskLevel::Safe,
         TweakStatus::Unknown,
         false,
         {t1, t2}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_LOCAL_MACHINE;
    t1.subKey = "SOFTWARE\\Policies\\Microsoft\\Windows\\EdgeUI";
    t1.valueName = "AllowEdgeSwipe";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 0;
    t1.expectedDword = 0;
    t1.deleteOnRevert = true;

    RegisterTweak({"fps_edge_swipe",
                   "Disable Edge Swipe Gestures",
                   "Prevents accidental edge swipe from interrupting "
                   "fullscreen games",
                   RiskLevel::Safe,
                   TweakStatus::Unknown,
                   false,
                   {t1}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_LOCAL_MACHINE;
    t1.subKey = "SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers";
    t1.valueName = "HwSchMode";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 2;
    t1.expectedDword = 2;
    t1.defaultDword = 1;

    RegisterTweak(
        {"fps_gpu_scheduling",
         "Enable HW GPU Scheduling",
         "Enables hardware-accelerated GPU scheduling to reduce render latency",
         RiskLevel::Safe,
         TweakStatus::Unknown,
         true,
         {t1}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_LOCAL_MACHINE;
    t1.subKey = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File "
                "Execution Options\\dwm.exe";
    t1.valueName = "DpiAwareness";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 1;
    t1.expectedDword = 1;
    t1.deleteOnRevert = true;

    RegisterTweak(
        {"fps_dpi_aware",
         "Force System DPI Awareness",
         "Forces system-level DPI awareness to prevent blurry scaling in games",
         RiskLevel::Safe,
         TweakStatus::Unknown,
         false,
         {t1}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_CURRENT_USER;
    t1.subKey = "Control Panel\\Mouse";
    t1.valueName = "MouseSpeed";
    t1.valueType = RegValueType::String;
    t1.applyString = "0";
    t1.expectedString = "0";
    t1.defaultString = "1";

    RegistryTarget t2;
    t2.root = HKEY_CURRENT_USER;
    t2.subKey = "Control Panel\\Mouse";
    t2.valueName = "MouseThreshold1";
    t2.valueType = RegValueType::String;
    t2.applyString = "0";
    t2.expectedString = "0";
    t2.defaultString = "6";

    RegistryTarget t3;
    t3.root = HKEY_CURRENT_USER;
    t3.subKey = "Control Panel\\Mouse";
    t3.valueName = "MouseThreshold2";
    t3.valueType = RegValueType::String;
    t3.applyString = "0";
    t3.expectedString = "0";
    t3.defaultString = "10";

    RegistryTarget t4;
    t4.root = HKEY_CURRENT_USER;
    t4.subKey = "Control Panel\\Mouse";
    t4.valueName = "SmoothMouseXCurve";
    t4.valueType = RegValueType::Binary;
    t4.applyBinary = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0xC0, 0xCC, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x80, 0x99, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x40, 0x66, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x33, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00};
    t4.defaultBinary = t4.applyBinary;

    RegistryTarget t5;
    t5.root = HKEY_CURRENT_USER;
    t5.subKey = "Control Panel\\Mouse";
    t5.valueName = "SmoothMouseYCurve";
    t5.valueType = RegValueType::Binary;
    t5.applyBinary = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0xA8, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00};
    t5.defaultBinary = t5.applyBinary;

    RegisterTweak(
        {"fps_mouse_accel",
         "Disable Mouse Acceleration",
         "Removes pointer acceleration and resets smooth curves for precise "
         "1:1 aiming",
         RiskLevel::Safe,
         TweakStatus::Unknown,
         false,
         {t1, t2, t3, t4, t5}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_LOCAL_MACHINE;
    t1.subKey = "SOFTWARE\\Microsoft\\DirectX";
    t1.valueName = "MaximumFrameLatency";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 1;
    t1.expectedDword = 1;
    t1.deleteOnRevert = true;

    RegisterTweak(
        {"fps_frame_latency",
         "Reduce Frame Pre-Render Queue",
         "Sets DirectX max pre-rendered frames to 1 for minimum input lag",
         RiskLevel::Moderate,
         TweakStatus::Unknown,
         false,
         {t1}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_CURRENT_USER;
    t1.subKey = "Software\\Microsoft\\DirectX\\UserGpuPreferences";
    t1.valueName = "DirectXUserGlobalSettings";
    t1.valueType = RegValueType::String;
    t1.applyString = "VRROptimizeEnable=0";
    t1.expectedString = "VRROptimizeEnable=0";
    t1.defaultString = "VRROptimizeEnable=1";
    t1.mergeStringValue = true;

    RegisterTweak(
        {"fps_vrr",
         "Disable Windows VRR Management",
         "Stops Windows from overriding your monitor's variable refresh rate "
         "settings",
         RiskLevel::Moderate,
         TweakStatus::Unknown,
         false,
         {t1}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_CURRENT_USER;
    t1.subKey = "Control Panel\\Desktop";
    t1.valueName = "MenuShowDelay";
    t1.valueType = RegValueType::String;
    t1.applyString = "0";
    t1.expectedString = "0";
    t1.defaultString = "400";

    RegisterTweak(
        {"fps_menu_delay",
         "Reduce Menu Show Delay",
         "Sets context menu delay to 0ms for instant right-click menus",
         RiskLevel::Safe,
         TweakStatus::Unknown,
         false,
         {t1}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_LOCAL_MACHINE;
    t1.subKey = "SYSTEM\\CurrentControlSet\\Services\\mouclass\\Parameters";
    t1.valueName = "MouseDataQueueSize";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 16;
    t1.expectedDword = 16;
    t1.defaultDword = 100;

    RegistryTarget t2;
    t2.root = HKEY_LOCAL_MACHINE;
    t2.subKey = "SYSTEM\\CurrentControlSet\\Services\\kbdclass\\Parameters";
    t2.valueName = "KeyboardDataQueueSize";
    t2.valueType = RegValueType::Dword;
    t2.applyDword = 16;
    t2.expectedDword = 16;
    t2.defaultDword = 100;

    RegisterTweak(
        {"fps_input_queue",
         "Optimize Input Queue Size",
         "Reduces mouse/keyboard buffer size from 100 to 16 for lower input "
         "latency",
         RiskLevel::Safe,
         TweakStatus::Unknown,
         false,
         {t1, t2}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_LOCAL_MACHINE;
    t1.subKey = "SYSTEM\\CurrentControlSet\\Control\\Power\\PowerThrottling";
    t1.valueName = "PowerThrottlingOff";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 1;
    t1.expectedDword = 1;
    t1.deleteOnRevert = true;

    RegisterTweak({"fps_power_throttle",
                   "Disable Power Throttling",
                   "Prevents Windows from throttling foreground app CPU "
                   "frequency for power savings",
                   RiskLevel::Moderate,
                   TweakStatus::Unknown,
                   false,
                   {t1}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_CURRENT_USER;
    t1.subKey = "Software\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccess"
                "Applications";
    t1.valueName = "GlobalUserDisabled";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 1;
    t1.expectedDword = 1;
    t1.defaultDword = 0;

    RegistryTarget t2;
    t2.root = HKEY_CURRENT_USER;
    t2.subKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Search";
    t2.valueName = "BackgroundAppGlobalToggle";
    t2.valueType = RegValueType::Dword;
    t2.applyDword = 0;
    t2.expectedDword = 0;
    t2.defaultDword = 1;

    RegisterTweak(
        {"fps_background_apps",
         "Disable Background Apps",
         "Prevents UWP apps from running in the background and consuming "
         "resources",
         RiskLevel::Safe,
         TweakStatus::Unknown,
         false,
         {t1, t2}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_CURRENT_USER;
    t1.subKey =
        "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Serialize";
    t1.valueName = "StartupDelayInMSec";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 0;
    t1.expectedDword = 0;
    t1.deleteOnRevert = true;

    RegisterTweak(
        {"fps_startup_delay",
         "Disable Startup Delay",
         "Removes the artificial delay Windows adds before launching startup "
         "programs",
         RiskLevel::Safe,
         TweakStatus::Unknown,
         false,
         {t1}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_LOCAL_MACHINE;
    t1.subKey = "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory "
                "Management";
    t1.valueName = "DisablePagingExecutive";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 1;
    t1.expectedDword = 1;
    t1.defaultDword = 0;

    RegistryTarget t2;
    t2.root = HKEY_LOCAL_MACHINE;
    t2.subKey = "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory "
                "Management";
    t2.valueName = "LargeSystemCache";
    t2.valueType = RegValueType::Dword;
    t2.applyDword = 1;
    t2.expectedDword = 1;
    t2.defaultDword = 0;

    RegisterTweak(
        {"fps_large_system_cache",
         "Keep Kernel in RAM",
         "Keeps kernel and drivers in physical RAM and optimizes system cache",
         RiskLevel::Moderate,
         TweakStatus::Unknown,
         true,
         {t1, t2}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_CURRENT_USER;
    t1.subKey = "Control Panel\\Desktop";
    t1.valueName = "AutoEndTasks";
    t1.valueType = RegValueType::String;
    t1.applyString = "1";
    t1.expectedString = "1";
    t1.defaultString = "0";

    RegisterTweak({"fps_auto_end_tasks",
                   "Auto-End Hung Tasks",
                   "Automatically closes unresponsive applications during "
                   "shutdown or logoff",
                   RiskLevel::Safe,
                   TweakStatus::Unknown,
                   false,
                   {t1}});
  }

  {
    RegisterTweak(
        {"fps_aspm", "Disable PCIe ASPM",
         "Disables Active State Power Management for PCIe devices to eliminate "
         "latency spikes",
         RiskLevel::Moderate, TweakStatus::Unknown, false});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_LOCAL_MACHINE;
    t1.subKey = "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power";
    t1.valueName = "HiberbootEnabled";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 0;
    t1.expectedDword = 0;
    t1.defaultDword = 1;

    RegisterTweak({"fps_fast_startup",
                   "Disable Fast Startup",
                   "Disables hybrid shutdown for a clean boot \xe2\x80\x94 "
                   "fixes driver issues and timer drift",
                   RiskLevel::Safe,
                   TweakStatus::Unknown,
                   false,
                   {t1}});
  }

  {
    RegistryTarget t1;
    t1.root = HKEY_LOCAL_MACHINE;
    t1.subKey = "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel";
    t1.valueName = "DistributeTimers";
    t1.valueType = RegValueType::Dword;
    t1.applyDword = 1;
    t1.expectedDword = 1;
    t1.deleteOnRevert = true;

    RegisterTweak(
        {"fps_distribute_timers",
         "Distribute Timers Across Cores",
         "Spreads timer interrupts across all CPU cores to reduce contention",
         RiskLevel::Moderate,
         TweakStatus::Unknown,
         true,
         {t1}});
  }

  {
    RegisterTweak({"fps_disable_procs", "Disable Non-Microsoft Services",
                   "Dynamically detects and disables all third-party services "
                   "(like msconfig)",
                   RiskLevel::Moderate, TweakStatus::Unknown, true});
  }

  RegisterGroup(
      {"fps_display",
       "Display & Compositor",
       "\xf0\x9f\x96\xa5\xef\xb8\x8f",
       "Fullscreen Opt, MPO, Transparency, DWM Batch, Visual Effects, "
       "Animations, DWM Queue, Ghosting",
       {"fps_fullscreen_opt", "fps_mpo", "fps_transparency", "fps_dwm_batch",
        "fps_visual_effects", "fps_animations", "fps_dwm_maxqueued",
        "fps_disable_ghosting"}});

  RegisterGroup(
      {"fps_overlays",
       "Game Bar & Overlays",
       "\xf0\x9f\x8e\xae",
       "Game Bar/DVR, Snap Layouts, Aero Shake, Game Mode, Edge Swipe",
       {"fps_game_bar", "fps_snap_layouts", "fps_aero_shake", "fps_game_mode",
        "fps_edge_swipe"}});

  RegisterGroup({"fps_gpu",
                 "GPU & Scheduling",
                 "\xe2\x9a\x99\xef\xb8\x8f",
                 "GPU Scheduling, DPI Awareness",
                 {"fps_gpu_scheduling", "fps_dpi_aware"}});

  RegisterGroup({"fps_input",
                 "Input & Mouse",
                 "\xf0\x9f\x96\xb1\xef\xb8\x8f",
                 "Mouse Acceleration, Frame Latency, VRR Management, Menu "
                 "Delay, Input Queue Size",
                 {"fps_mouse_accel", "fps_frame_latency", "fps_vrr",
                  "fps_menu_delay", "fps_input_queue"}});

  RegisterGroup(
      {"fps_cpu",
       "CPU & Power",
       "\xf0\x9f\x92\xa1",
       "Power Throttling, Background Apps, Startup Delay, "
       "Keep Kernel in RAM, Auto-End Tasks, PCIe ASPM, "
       "Fast Startup, Timers",
       {"fps_power_throttle", "fps_background_apps", "fps_startup_delay",
        "fps_large_system_cache", "fps_auto_end_tasks", "fps_aspm",
        "fps_fast_startup", "fps_distribute_timers"}});

  RegisterGroup({"fps_services",
                 "Services & Maintenance",
                 "\xf0\x9f\x94\xa7",
                 "Disable non-Microsoft services for maximum performance",
                 {"fps_disable_procs"}});
}

bool FpsModule::ApplyTweak(const std::string &tweakId) {
  m_lastFailReason.clear();

  if (tweakId == "fps_aspm")
    return ApplyAspm();
  if (tweakId == "fps_disable_procs")
    return ApplyDisableProcesses();
  return BaseModule::ApplyTweak(tweakId);
}

bool FpsModule::RevertTweak(const std::string &tweakId) {
  m_lastFailReason.clear();

  if (tweakId == "fps_aspm")
    return RevertAspm();
  if (tweakId == "fps_disable_procs")
    return RevertDisableProcesses();
  return BaseModule::RevertTweak(tweakId);
}

void FpsModule::RefreshStatus() {
  BaseModule::RefreshStatus();

  TweakInfo *aspm = FindTweak("fps_aspm");
  if (aspm)
    aspm->status =
        IsAspmDisabled() ? TweakStatus::Applied : TweakStatus::NotApplied;

  TweakInfo *procs = FindTweak("fps_disable_procs");
  if (procs)
    procs->status = IsDisableProcessesApplied() ? TweakStatus::Applied
                                                : TweakStatus::NotApplied;
}

static std::string GetActiveSchemeGuid() {
  auto val = Vax::System::Registry::ReadString(
      HKEY_LOCAL_MACHINE,
      "SYSTEM\\CurrentControlSet\\Control\\Power\\User\\PowerSchemes",
      "ActivePowerScheme");
  if (val.has_value() && !val.value().empty()) {
    return val.value();
  }

  std::string output =
      Vax::System::RunCommandCapture("powercfg /getactivescheme");
  size_t guidStart = output.find(": ");
  if (guidStart != std::string::npos) {
    guidStart += 2;
    size_t guidEnd = output.find(' ', guidStart);
    if (guidEnd == std::string::npos)
      guidEnd = output.length();
    std::string guid = output.substr(guidStart, guidEnd - guidStart);
    while (!guid.empty() &&
           (guid.back() == ' ' || guid.back() == '\r' || guid.back() == '\n'))
      guid.pop_back();
    if (!guid.empty())
      return guid;
  }
  return "";
}

bool FpsModule::IsAspmDisabled() {
  std::string guid = GetActiveSchemeGuid();
  if (guid.empty())
    return false;

  std::string subKey =
      "SYSTEM\\CurrentControlSet\\Control\\Power\\User\\PowerSchemes\\" + guid +
      "\\" + kPciExpressSubgroup + "\\" + kAspmSetting;

  auto val =
      System::Registry::ReadDword(HKEY_LOCAL_MACHINE, subKey, "ACSettingIndex");
  return val.has_value() && val.value() == 0;
}

bool FpsModule::ApplyAspm() {
  using namespace System;
  if (!Admin::IsElevated()) {
    m_lastFailReason = "Requires Administrator privileges. Restart as Admin to "
                       "apply this tweak.";
    Logger::Warning("ApplyAspm: skipped (not elevated)");
    return false;
  }
  std::string guid = GetActiveSchemeGuid();
  if (guid.empty()) {
    m_lastFailReason = "Could not determine active power scheme.";
    return false;
  }
  bool ok = RunSilentCommand("powercfg -setacvalueindex " + guid + " " +
                             kPciExpressSubgroup + " " + kAspmSetting + " 0");
  ok &= RunSilentCommand("powercfg -setdcvalueindex " + guid + " " +
                         kPciExpressSubgroup + " " + kAspmSetting + " 0");
  ok &= RunSilentCommand("powercfg -setactive " + guid);
  if (ok)
    Logger::Success("Applied: PCIe ASPM disabled");
  else
    Logger::Error("Failed to disable PCIe ASPM \xe2\x80\x94 requires admin");
  return ok;
}

bool FpsModule::RevertAspm() {
  using namespace System;
  if (!Admin::IsElevated()) {
    m_lastFailReason = "Requires Administrator privileges. Restart as Admin to "
                       "revert this tweak.";
    Logger::Warning("RevertAspm: skipped (not elevated)");
    return false;
  }
  std::string guid = GetActiveSchemeGuid();
  if (guid.empty())
    return false;
  bool ok = RunSilentCommand("powercfg -setacvalueindex " + guid + " " +
                             kPciExpressSubgroup + " " + kAspmSetting + " 1");
  ok &= RunSilentCommand("powercfg -setdcvalueindex " + guid + " " +
                         kPciExpressSubgroup + " " + kAspmSetting + " 2");
  ok &= RunSilentCommand("powercfg -setactive " + guid);
  if (ok)
    Logger::Success("Reverted: PCIe ASPM restored to defaults");
  else
    Logger::Error("Failed to revert PCIe ASPM");
  return ok;
}

bool FpsModule::IsDisableProcessesApplied() {
  auto saved = LoadDisabledServicesList();
  return !saved.empty();
}

bool FpsModule::ApplyDisableProcesses() {
  if (!Admin::IsElevated()) {
    m_lastFailReason = "Requires Administrator privileges. Restart as Admin to "
                       "apply this tweak.";
    System::Logger::Warning("ApplyDisableProcesses: skipped (not elevated)");
    return false;
  }

  auto services = EnumerateThirdPartyServices();

  if (services.empty()) {
    m_lastFailReason = "No third-party services were detected to disable.";
    System::Logger::Info("No third-party services found to disable");
    return false;
  }

  int succeeded = 0, failed = 0;
  std::vector<std::string> disabledList;

  for (const auto &svcName : services) {
    if (System::RunSilentCommand("sc config \"" + svcName +
                                 "\" start=disabled")) {
      ++succeeded;
      disabledList.push_back(svcName);
    } else {
      ++failed;
    }
  }

  if (!disabledList.empty() && !SaveDisabledServicesList(disabledList)) {
    m_lastFailReason =
        "Services were disabled but the recovery list could not be saved.";
    System::Logger::Error("Failed to save disabled services list to disk");
  }

  System::Logger::Success("Disable Services: " + std::to_string(succeeded) +
                          " disabled, " + std::to_string(failed) +
                          " failed (of " + std::to_string(services.size()) +
                          " third-party services)");
  return (succeeded > 0);
}

bool FpsModule::RevertDisableProcesses() {
  if (!Admin::IsElevated()) {
    m_lastFailReason = "Requires Administrator privileges. Restart as Admin to "
                       "revert this tweak.";
    System::Logger::Warning("RevertDisableProcesses: skipped (not elevated)");
    return false;
  }

  auto saved = LoadDisabledServicesList();
  if (saved.empty()) {
    m_lastFailReason = "No saved service list found to revert.";
    System::Logger::Error("No saved service list found to revert");
    return false;
  }

  int restored = 0;

  for (const auto &svcName : saved) {
    std::string subKey = "SYSTEM\\CurrentControlSet\\Services\\" + svcName;
    const auto &backups = System::Registry::GetBackupEntries();
    bool found = false;
    for (const auto &entry : backups) {
      if (entry.subKey == subKey && entry.valueName == "Start") {
        if (System::Registry::RestoreEntry(entry))
          ++restored;
        found = true;
        break;
      }
    }
    if (!found) {
      if (System::RunSilentCommand("sc config \"" + svcName +
                                   "\" start=demand"))
        ++restored;
    }
  }

  DeleteDisabledServicesList();

  System::Logger::Success("Reverted: " + std::to_string(restored) +
                          " services restored");
  return (restored > 0);
}

}
