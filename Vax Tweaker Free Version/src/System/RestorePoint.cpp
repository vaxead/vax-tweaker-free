
#include "RestorePoint.h"
#include "Logger.h"
#include <objbase.h>
#include <srrestoreptapi.h>
#include <string>
#include <vector>
#include <windows.h>

#pragma comment(lib, "srclient.lib")
#pragma comment(lib, "ole32.lib")

namespace Vax::System {

bool RestorePoint::IsProtectionEnabled() {
  HKEY hKey;
  LONG result = RegOpenKeyExA(
      HKEY_LOCAL_MACHINE,
      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SystemRestore", 0,
      KEY_READ, &hKey);
  if (result != ERROR_SUCCESS)
    return false;

  DWORD value = 0;
  DWORD size = sizeof(DWORD);
  result = RegQueryValueExA(hKey, "DisableSR", nullptr, nullptr,
                            reinterpret_cast<LPBYTE>(&value), &size);
  RegCloseKey(hKey);

  if (result != ERROR_SUCCESS)
    return true;
  return (value == 0);
}

bool RestorePoint::EnableProtection() {
  char winDir[MAX_PATH] = {};
  GetWindowsDirectoryA(winDir, MAX_PATH);
  std::string sysDrive(1, winDir[0]);

  STARTUPINFOA si = {};
  PROCESS_INFORMATION pi = {};
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;

  std::string cmdStr =
      "powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \""
      "Enable-ComputerRestore -Drive '" +
      sysDrive + ":\\'\"";
  std::vector<char> cmd(cmdStr.begin(), cmdStr.end());
  cmd.push_back('\0');

  BOOL ok = CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, FALSE,
                           CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
  if (!ok) {
    Logger::Error("Failed to launch PowerShell to enable System Protection");
    return false;
  }

  DWORD waitResult = WaitForSingleObject(pi.hProcess, 30000);

  DWORD exitCode = 1;
  if (waitResult == WAIT_OBJECT_0) {
    GetExitCodeProcess(pi.hProcess, &exitCode);
  } else {
    Logger::Error("PowerShell Enable-ComputerRestore timed out");
    TerminateProcess(pi.hProcess, 1);
  }
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  if (exitCode != 0) {
    Logger::Error("PowerShell Enable-ComputerRestore failed (exit: " +
                  std::to_string(exitCode) + ")");
    return false;
  }

  Logger::Success("System Protection enabled on " + sysDrive + ":\\");
  return true;
}

void RestorePoint::DisableFrequencyLimit() {
  HKEY hKey;
  DWORD disposition;
  LONG result = RegCreateKeyExA(
      HKEY_LOCAL_MACHINE,
      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SystemRestore", 0,
      nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey,
      &disposition);
  if (result == ERROR_SUCCESS) {
    DWORD value = 0;
    RegSetValueExA(hKey, "SystemRestorePointCreationFrequency", 0, REG_DWORD,
                   reinterpret_cast<const BYTE *>(&value), sizeof(DWORD));
    RegCloseKey(hKey);
  }
}

void RestorePoint::RestoreFrequencyLimit() {
  HKEY hKey;
  LONG result = RegOpenKeyExA(
      HKEY_LOCAL_MACHINE,
      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SystemRestore", 0,
      KEY_SET_VALUE, &hKey);
  if (result == ERROR_SUCCESS) {
    RegDeleteValueA(hKey, "SystemRestorePointCreationFrequency");
    RegCloseKey(hKey);
  }
}

bool RestorePoint::Create(const std::string &description) {
  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  bool comInitialized = SUCCEEDED(hr) || hr == S_FALSE;
  if (hr == RPC_E_CHANGED_MODE) {
    comInitialized = false;
  }

  if (!IsProtectionEnabled()) {
    Logger::Warning("System Protection is disabled, attempting to enable...");
    if (!EnableProtection()) {
      Logger::Error("Cannot create restore point: System Protection could not "
                    "be enabled");
      if (comInitialized)
        CoUninitialize();
      return false;
    }
    bool ready = false;
    for (int i = 0; i < 10; ++i) {
      Sleep(300);
      if (IsProtectionEnabled()) {
        ready = true;
        break;
      }
    }
    if (!ready) {
      Logger::Warning("System Protection enable may not have taken effect yet, "
                      "attempting anyway...");
    }
  }

  DisableFrequencyLimit();

  RESTOREPOINTINFOA restorePoint = {};
  STATEMGRSTATUS status = {};

  restorePoint.dwEventType = BEGIN_SYSTEM_CHANGE;
  restorePoint.dwRestorePtType = MODIFY_SETTINGS;
  restorePoint.llSequenceNumber = 0;
  strncpy_s(restorePoint.szDescription, description.c_str(), MAX_DESC_W);

  bool success = false;
  const int kMaxRetries = 3;

  for (int attempt = 1; attempt <= kMaxRetries; ++attempt) {
    if (SRSetRestorePointA(&restorePoint, &status)) {
      success = true;
      break;
    }

    Logger::Warning("Restore point attempt " + std::to_string(attempt) +
                    " failed (error: " + std::to_string(status.nStatus) + ")");

    if (attempt < kMaxRetries) {
      Sleep(1000);

      if (status.nStatus == ERROR_SERVICE_DISABLED || !IsProtectionEnabled()) {
        Logger::Info("Re-enabling System Protection before retry...");
        EnableProtection();
        Sleep(1000);
      }
    }
  }

  if (!success) {
    Logger::Error(
        "Failed to create restore point after " + std::to_string(kMaxRetries) +
        " attempts (last error: " + std::to_string(status.nStatus) + ")");
    RestoreFrequencyLimit();
    if (comInitialized)
      CoUninitialize();
    return false;
  }

  restorePoint.dwEventType = END_SYSTEM_CHANGE;
  restorePoint.llSequenceNumber = status.llSequenceNumber;
  if (!SRSetRestorePointA(&restorePoint, &status)) {
    Logger::Warning("End restore point call failed (non-critical, error: " +
                    std::to_string(status.nStatus) + ")");
  }

  RestoreFrequencyLimit();
  if (comInitialized)
    CoUninitialize();

  Logger::Success("Created system restore point: " + description);
  return true;
}

}
