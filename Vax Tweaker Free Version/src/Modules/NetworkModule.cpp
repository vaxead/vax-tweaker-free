
#include "NetworkModule.h"
#include "../Core/Admin.h"
#include "../System/Logger.h"
#include "../System/ProcessUtils.h"
#include "../System/Registry.h"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

namespace Vax::Modules {

static const char *kTcpParams =
    "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters";
static const char *kTcpParamsIfaces =
    "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces";
static const char *kNetBT =
    "SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters";

NetworkModule::NetworkModule()
    : BaseModule(2, "Network & Latency",
                 "Optimize network settings for gaming latency", "🌐",
                 Vax::ModuleCategory::Network) {
  InitializeTweaks();
}

void NetworkModule::InitializeTweaks() {
  m_isImplemented = true;

  RegisterGroup({"group_nic",
                 "NIC & Hardware",
                 "🔌",
                 "Low-level adapter settings for latency reduction",
                 {"net_nic_intmod", "net_nic_flow", "net_nic_eee",
                  "net_nic_rsc", "net_nic_lso"}});

  RegisterGroup({"group_tcp",
                 "TCP/IP Stack",
                 "📡",
                 "System-wide protocol optimizations",
                 {"net_nagle", "net_tcp_system"}});

  RegisterGroup({"group_dns",
                 "DNS & Protocols",
                 "🌍",
                 "Name resolution and legacy protocol settings",
                 {"net_netbios", "net_dns_cloudflare", "net_dns_google"}});

  RegisterGroup({"group_reset",
                 "Maintenance & Reset",
                 "🔧",
                 "Fix connectivity issues",
                 {"net_reset_winsock", "net_reset_tcpip", "net_flush_dns"}});

  RegisterTweak({"net_nic_intmod", "Disable Interrupt Moderation",
                 "Forces immediate packet processing by CPU. Reduces latency "
                 "significantly "
                 "at the cost of higher CPU usage.",
                 RiskLevel::Moderate, TweakStatus::Unknown, false});

  RegisterTweak(
      {"net_nic_flow", "Disable Flow Control",
       "Prevents the NIC from pausing traffic when buffers are full, forcing "
       "continuous data flow.",
       RiskLevel::Safe, TweakStatus::Unknown, false});

  RegisterTweak({"net_nic_eee", "Disable Energy Efficient Ethernet",
                 "Prevents NIC from entering low-power states which cause "
                 "latency spikes during wake-up.",
                 RiskLevel::Safe, TweakStatus::Unknown, false});

  RegisterTweak(
      {"net_nic_rsc", "Disable RSC (Receive Segment Coalescing)",
       "Prevents grouping of received packets. Essential for gaming latency as "
       "RSC adds delay to process logical units.",
       RiskLevel::Moderate, TweakStatus::Unknown, false});

  RegisterTweak({"net_nic_lso", "Disable Large Send Offload (LSO)",
                 "Forces CPU to handle segmentation instead of NIC. Fixes "
                 "packet loss/lag "
                 "on some adapters.",
                 RiskLevel::Moderate, TweakStatus::Unknown, false});

  RegisterTweak({"net_nagle", "Disable Nagle Algorithm",
                 "Disables Nagle's algorithm (TCPNoDelay) to send packets "
                 "immediately without waiting for buffer fill.",
                 RiskLevel::Safe, TweakStatus::Unknown, false});

  RegisterTweak({"net_tcp_system", "Optimize TCP System Settings",
                 "Optimizes MaxUserPort (65534), TcpTimedWaitDelay (30s), and "
                 "disables TCP Heuristics.",
                 RiskLevel::Safe, TweakStatus::Unknown, false});

  RegisterTweak({"net_netbios", "Disable NetBIOS",
                 "Reduces local broadcast traffic/noise on the network.",
                 RiskLevel::Moderate, TweakStatus::Unknown, false});

  RegisterTweak({"net_dns_cloudflare", "Set Cloudflare DNS",
                 "1.1.1.1 / 1.0.0.1 - Generally fastest public DNS.",
                 RiskLevel::Safe, TweakStatus::Unknown, false});

  RegisterTweak({"net_dns_google", "Set Google DNS",
                 "8.8.8.8 / 8.8.4.4 - Reliable fallback.", RiskLevel::Safe,
                 TweakStatus::Unknown, false});

  RegisterTweak(
      {"net_reset_winsock", "Reset Winsock Catalog",
       "Fixes connectivity issues caused by corrupted socket providers.",
       RiskLevel::Safe, TweakStatus::Unknown, true});

  RegisterTweak({"net_reset_tcpip", "Reset TCP/IP Stack",
                 "Resets all IP settings to Windows defaults.", RiskLevel::Safe,
                 TweakStatus::Unknown, true});

  RegisterTweak({"net_flush_dns", "Flush DNS Cache",
                 "Clears the local DNS resolver cache.", RiskLevel::Safe,
                 TweakStatus::Unknown, true});
}

bool NetworkModule::ApplyTweak(const std::string &tweakId) {
  m_lastFailReason.clear();

  if (!Vax::Admin::IsElevated()) {
    m_lastFailReason =
        "Requires Administrator privileges. Restart as Admin to apply this tweak.";
    System::Logger::Error("Network tweaks require Administrator privileges");
    return false;
  }

  if (tweakId == "net_nic_intmod")
    return ApplyNicProperty("*InterruptModeration", "0");
  if (tweakId == "net_nic_flow")
    return ApplyNicProperty("*FlowControl", "0");
  if (tweakId == "net_nic_eee")
    return ApplyNicProperty("*EEE", "0");
  if (tweakId == "net_nic_rsc")
    return ApplyNicProperty("*RscIPv4", "0") &&
           ApplyNicProperty("*RscIPv6", "0");
  if (tweakId == "net_nic_lso")
    return ApplyNicProperty("*LsoV2IPv4", "0") &&
           ApplyNicProperty("*LsoV2IPv6", "0");

  if (tweakId == "net_nagle")
    return ApplyNagle();
  if (tweakId == "net_tcp_system")
    return ApplyTcpSystemSettings();

  if (tweakId == "net_netbios")
    return ApplyNetBios();
  if (tweakId == "net_dns_cloudflare")
    return ApplyDnsServer("1.1.1.1", "1.0.0.1");
  if (tweakId == "net_dns_google")
    return ApplyDnsServer("8.8.8.8", "8.8.4.4");

  if (tweakId == "net_reset_winsock")
    return ResetWinsock();
  if (tweakId == "net_reset_tcpip")
    return ResetTcpIp();
  if (tweakId == "net_flush_dns")
    return FlushDns();

  return BaseModule::ApplyTweak(tweakId);
}

bool NetworkModule::RevertTweak(const std::string &tweakId) {
  m_lastFailReason.clear();

  if (!Vax::Admin::IsElevated()) {
    m_lastFailReason =
        "Requires Administrator privileges. Restart as Admin to revert this tweak.";
    return false;
  }

  if (tweakId == "net_nic_intmod")
    return ApplyNicProperty("*InterruptModeration", "1");
  if (tweakId == "net_nic_flow") {
    bool ok = ApplyNicProperty("*FlowControl", "3");
    if (!ok)
      ok = RevertNicProperty(
          "*FlowControl");
    return ok;
  }
  if (tweakId == "net_nic_eee")
    return ApplyNicProperty("*EEE", "1");
  if (tweakId == "net_nic_rsc")
    return ApplyNicProperty("*RscIPv4", "1") &&
           ApplyNicProperty("*RscIPv6", "1");
  if (tweakId == "net_nic_lso")
    return ApplyNicProperty("*LsoV2IPv4", "1") &&
           ApplyNicProperty("*LsoV2IPv6", "1");

  if (tweakId == "net_nagle")
    return RevertNagle();
  if (tweakId == "net_tcp_system")
    return RevertTcpSystemSettings();

  if (tweakId == "net_netbios")
    return RevertNetBios();
  if (tweakId == "net_dns_cloudflare" || tweakId == "net_dns_google")
    return RevertDnsServer();

  return BaseModule::RevertTweak(tweakId);
}

void NetworkModule::RefreshStatus() {
  BaseModule::RefreshStatus();

  auto Update = [&](const char *id, bool condition) {
    TweakInfo *t = FindTweak(id);
    if (t)
      t->status = condition ? TweakStatus::Applied : TweakStatus::NotApplied;
  };

  Update("net_nic_intmod", IsNicPropertySet("*InterruptModeration", "0"));
  Update("net_nic_flow", IsNicPropertySet("*FlowControl", "0"));
  Update("net_nic_eee", IsNicPropertySet("*EEE", "0"));
  Update("net_nic_rsc", IsNicPropertySet("*RscIPv4", "0"));
  Update("net_nic_lso", IsNicPropertySet("*LsoV2IPv4", "0"));

  Update("net_nagle", IsNagleDisabled());
  Update("net_tcp_system", IsTcpSystemOptimized());

  Update("net_netbios", IsNetBiosDisabled());
  Update("net_dns_cloudflare", IsDnsServerSet("1.1.1.1"));
  Update("net_dns_google", IsDnsServerSet("8.8.8.8"));
}

std::string NetworkModule::FindActiveAdapterRegistryKey() {
  using namespace Vax::System;

  static const char *kNetClass = "SYSTEM\\CurrentControlSet\\Control\\Class\\{"
                                 "4d36e972-e325-11ce-bfc1-08002be10318}";

  for (int i = 0; i < 20; ++i) {
    char sub[10];
    snprintf(sub, sizeof(sub), "%04d", i);
    std::string key = std::string(kNetClass) + "\\" + sub;

    if (Registry::ReadString(HKEY_LOCAL_MACHINE, key, "*InterruptModeration")
            .has_value()) {
      return key;
    }
  }
  return "";
}

bool NetworkModule::ApplyNicProperty(const std::string &property,
                                     const std::string &value) {
  std::string cmd =
      "powershell -NoProfile -Command \""
      "$ok=$false; Get-NetAdapter -Physical | ForEach-Object { "
      "try { Set-NetAdapterAdvancedProperty -Name $_.Name -RegistryKeyword '" +
      property + "' -RegistryValue '" + value +
      "' -ErrorAction Stop; $ok=$true } catch {} "
      "}; if(-not $ok){exit 1}\"";
  bool ok = Vax::System::RunSilentCommand(cmd);
  if (ok)
    Vax::System::Logger::Success("Applied NIC Property: " + property + " = " +
                                 value);
  else
    Vax::System::Logger::Warning("Failed to apply NIC Property: " + property);
  return ok;
}

bool NetworkModule::RevertNicProperty(const std::string &property) {
  std::string cmd = "powershell -NoProfile -Command \""
                    "$ok=$false; Get-NetAdapter -Physical | ForEach-Object { "
                    "try { Reset-NetAdapterAdvancedProperty -Name $_.Name "
                    "-RegistryKeyword '" +
                    property +
                    "' -ErrorAction Stop; $ok=$true } catch {} "
                    "}; if(-not $ok){exit 1}\"";
  bool ok = Vax::System::RunSilentCommand(cmd);
  if (ok)
    Vax::System::Logger::Success("Reverted NIC Property: " + property);
  return ok;
}

bool NetworkModule::IsNicPropertySet(const std::string &property,
                                     const std::string &expected) {
  std::string key = FindActiveAdapterRegistryKey();
  if (key.empty())
    return false;

  auto val =
      Vax::System::Registry::ReadString(HKEY_LOCAL_MACHINE, key, property);
  if (val.has_value()) {
    return val.value() == expected;
  }

  return false;
}

bool NetworkModule::ApplyTcpSystemSettings() {
  using namespace Vax::System;
  bool ok = true;
  ok &= Registry::WriteDword(HKEY_LOCAL_MACHINE, kTcpParams, "MaxUserPort",
                             65534);
  ok &= Registry::WriteDword(HKEY_LOCAL_MACHINE, kTcpParams,
                             "TcpTimedWaitDelay", 30);

  System::RunSilentCommand("netsh int tcp set heuristics disabled");
  System::RunSilentCommand(
      "netsh int tcp set global autotuninglevel=normal");

  if (ok)
    Logger::Success("Applied System TCP Optimizations");
  return ok;
}

bool NetworkModule::RevertTcpSystemSettings() {
  using namespace Vax::System;
  Registry::DeleteValueNoBackup(HKEY_LOCAL_MACHINE, kTcpParams, "MaxUserPort");
  Registry::DeleteValueNoBackup(HKEY_LOCAL_MACHINE, kTcpParams,
                                "TcpTimedWaitDelay");

  System::RunSilentCommand("netsh int tcp set heuristics default");
  System::RunSilentCommand("netsh int tcp set global autotuninglevel=normal");

  Logger::Success("Reverted System TCP Optimizations");
  return true;
}

bool NetworkModule::IsTcpSystemOptimized() {
  using namespace Vax::System;
  auto p = Registry::ReadDword(HKEY_LOCAL_MACHINE, kTcpParams, "MaxUserPort");
  if (!p.has_value() || p.value() != 65534)
    return false;

  std::string out = RunCommandCapture("netsh int tcp show heuristics");
  if (out.find("disabled") == std::string::npos &&
      out.find("Disabled") == std::string::npos)
    return false;

  return true;
}

bool NetworkModule::ApplyNagle() {
  using namespace Vax::System;
  HKEY hInterfaces;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, kTcpParamsIfaces, 0,
                    KEY_READ | KEY_WOW64_64KEY, &hInterfaces) != ERROR_SUCCESS)
    return false;

  char subKeyName[256];
  DWORD index = 0;
  DWORD nameLen = sizeof(subKeyName);
  int applied = 0;

  while (RegEnumKeyExA(hInterfaces, index++, subKeyName, &nameLen, nullptr,
                       nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
    std::string fullKey = std::string(kTcpParamsIfaces) + "\\" + subKeyName;
    Registry::WriteDword(HKEY_LOCAL_MACHINE, fullKey, "TcpAckFrequency", 1);
    Registry::WriteDword(HKEY_LOCAL_MACHINE, fullKey, "TCPNoDelay", 1);
    ++applied;
    nameLen = sizeof(subKeyName);
  }
  RegCloseKey(hInterfaces);
  if (applied > 0)
    Logger::Success("Nagle disabled on interfaces");
  return applied > 0;
}

bool NetworkModule::RevertNagle() {
  using namespace Vax::System;
  HKEY hInterfaces;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, kTcpParamsIfaces, 0,
                    KEY_READ | KEY_WOW64_64KEY, &hInterfaces) != ERROR_SUCCESS)
    return false;

  char subKeyName[256];
  DWORD index = 0;
  DWORD nameLen = sizeof(subKeyName);

  while (RegEnumKeyExA(hInterfaces, index++, subKeyName, &nameLen, nullptr,
                       nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
    std::string fullKey = std::string(kTcpParamsIfaces) + "\\" + subKeyName;
    Registry::DeleteValueNoBackup(HKEY_LOCAL_MACHINE, fullKey,
                                  "TcpAckFrequency");
    Registry::DeleteValueNoBackup(HKEY_LOCAL_MACHINE, fullKey, "TCPNoDelay");
    nameLen = sizeof(subKeyName);
  }
  RegCloseKey(hInterfaces);
  Logger::Success("Reverted Nagle");
  return true;
}

bool NetworkModule::IsNagleDisabled() {
  using namespace Vax::System;
  HKEY hInterfaces;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, kTcpParamsIfaces, 0,
                    KEY_READ | KEY_WOW64_64KEY, &hInterfaces) != ERROR_SUCCESS)
    return false;

  char subKeyName[256];
  DWORD index = 0;
  DWORD nameLen = sizeof(subKeyName);
  bool found = false;

  while (RegEnumKeyExA(hInterfaces, index++, subKeyName, &nameLen, nullptr,
                       nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
    std::string fullKey = std::string(kTcpParamsIfaces) + "\\" + subKeyName;
    auto val = Registry::ReadDword(HKEY_LOCAL_MACHINE, fullKey, "TCPNoDelay");
    if (val.has_value()) {
      found = true;
      if (val.value() != 1) {
        RegCloseKey(hInterfaces);
        return false;
      }
    }
    nameLen = sizeof(subKeyName);
  }
  RegCloseKey(hInterfaces);
  return found;
}

bool NetworkModule::ApplyNetBios() {
  using namespace Vax::System;
  HKEY hInterfaces;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, kTcpParamsIfaces, 0,
                    KEY_READ | KEY_WOW64_64KEY, &hInterfaces) != ERROR_SUCCESS)
    return false;

  char subKeyName[256];
  DWORD index = 0;
  DWORD nameLen = sizeof(subKeyName);
  bool anySet = false;

  while (RegEnumKeyExA(hInterfaces, index++, subKeyName, &nameLen, nullptr,
                       nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
    std::string fullKey = std::string(kTcpParamsIfaces) + "\\" + subKeyName;
    Registry::WriteDword(HKEY_LOCAL_MACHINE, fullKey, "NetbiosOptions", 2);
    anySet = true;
    nameLen = sizeof(subKeyName);
  }
  RegCloseKey(hInterfaces);
  if (anySet)
    Logger::Success("Disabled NetBIOS");
  return anySet;
}

bool NetworkModule::RevertNetBios() {
  using namespace Vax::System;
  HKEY hInterfaces;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, kTcpParamsIfaces, 0,
                    KEY_READ | KEY_WOW64_64KEY, &hInterfaces) != ERROR_SUCCESS)
    return false;

  char subKeyName[256];
  DWORD index = 0;
  DWORD nameLen = sizeof(subKeyName);

  while (RegEnumKeyExA(hInterfaces, index++, subKeyName, &nameLen, nullptr,
                       nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
    std::string fullKey = std::string(kTcpParamsIfaces) + "\\" + subKeyName;
    Registry::WriteDword(HKEY_LOCAL_MACHINE, fullKey, "NetbiosOptions", 0);
    nameLen = sizeof(subKeyName);
  }
  RegCloseKey(hInterfaces);
  Logger::Success("Reverted NetBIOS");
  return true;
}

bool NetworkModule::IsNetBiosDisabled() {
  using namespace Vax::System;
  HKEY hInterfaces;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, kTcpParamsIfaces, 0,
                    KEY_READ | KEY_WOW64_64KEY, &hInterfaces) != ERROR_SUCCESS)
    return false;

  char subKeyName[256];
  DWORD index = 0;
  DWORD nameLen = sizeof(subKeyName);
  bool allDisabled = true;
  bool found = false;

  while (RegEnumKeyExA(hInterfaces, index++, subKeyName, &nameLen, nullptr,
                       nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
    std::string fullKey = std::string(kTcpParamsIfaces) + "\\" + subKeyName;
    auto val =
        Registry::ReadDword(HKEY_LOCAL_MACHINE, fullKey, "NetbiosOptions");
    if (val.has_value()) {
      found = true;
      if (val.value() != 2) {
        allDisabled = false;
        break;
      }
    }
    nameLen = sizeof(subKeyName);
  }
  RegCloseKey(hInterfaces);
  return found && allDisabled;
}

bool NetworkModule::ApplyDnsServer(const std::string &primary,
                                   const std::string &secondary) {
  std::string cmd =
      "powershell -NoProfile -Command \""
      "$iface = (Get-NetAdapter -Physical | Where-Object Status -eq 'Up' | "
      "Select-Object -First 1).InterfaceAlias; "
      "if($iface){ Set-DnsClientServerAddress -InterfaceAlias $iface "
      "-ServerAddresses ('" +
      primary + "','" + secondary + "') }\"";
  bool ok = Vax::System::RunSilentCommand(cmd);
  if (ok)
    Vax::System::Logger::Success("Applied DNS: " + primary);
  return ok;
}

bool NetworkModule::RevertDnsServer() {
  bool ok = Vax::System::RunSilentCommand(
      "powershell -NoProfile -Command \""
      "Get-NetAdapter -Physical | ForEach-Object { "
      "Set-DnsClientServerAddress -InterfaceAlias $_.InterfaceAlias "
      "-ResetServerAddresses }\"");
  if (ok)
    Vax::System::Logger::Success("Reverted DNS to DHCP");
  return ok;
}

bool NetworkModule::IsDnsServerSet(const std::string &primary) {
  std::string cmd =
      "powershell -NoProfile -Command \""
      "(Get-NetAdapter -Physical | Where-Object Status -eq 'Up' "
      "| Select-Object -First 1 | "
      "Get-DnsClientServerAddress -AddressFamily IPv4).ServerAddresses\"";
  std::string out = Vax::System::RunCommandCapture(cmd);
  return out.find(primary) != std::string::npos;
}

bool NetworkModule::ResetWinsock() {
  bool ok = Vax::System::RunSilentCommand("netsh winsock reset");
  if (ok)
    Vax::System::Logger::Success("Reset Winsock");
  return ok;
}
bool NetworkModule::ResetTcpIp() {
  bool ok = Vax::System::RunSilentCommand("netsh int ip reset");
  if (ok)
    Vax::System::Logger::Success("Reset TCP/IP");
  return ok;
}
bool NetworkModule::FlushDns() {
  bool ok = Vax::System::RunSilentCommand("ipconfig /flushdns");
  if (ok)
    Vax::System::Logger::Success("Flushed DNS Cache");
  return ok;
}

}
