
#include "Admin.h"
#include "../System/Logger.h"
#include <string>
#include <vector>

namespace Vax {

    bool Admin::IsUserInAdminGroup() {
        BOOL isMember = FALSE;
        PSID adminGroup = nullptr;
        SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

        if (AllocateAndInitializeSid(
                &ntAuthority, 
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &adminGroup)) {

            CheckTokenMembership(nullptr, adminGroup, &isMember);
            FreeSid(adminGroup);
        }

        return isMember == TRUE;
    }

    bool Admin::IsProcessElevated() {
        HANDLE hToken = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            System::Logger::Warning("Could not open process token to check elevation");
            return false;
        }

        TOKEN_ELEVATION elevation = {};
        DWORD size = sizeof(elevation);
        BOOL ok = GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size);
        CloseHandle(hToken);

        if (!ok) {
            System::Logger::Warning("GetTokenInformation(TokenElevation) failed");
            return false;
        }

        return elevation.TokenIsElevated != 0;
    }

    bool Admin::IsElevated() {
        return IsProcessElevated();
    }

    bool Admin::RequestElevation() {
        DWORD bufSize = MAX_PATH;
        std::wstring wpath(bufSize, L'\0');

        while (true) {
            DWORD len = GetModuleFileNameW(nullptr, &wpath[0], bufSize);
            if (len == 0) {
                System::Logger::Error("GetModuleFileNameW failed (error: " + std::to_string(GetLastError()) + ")");
                return false;
            }
            if (len < bufSize) {
                wpath.resize(len);
                break;
            }
            bufSize *= 2;
            wpath.resize(bufSize, L'\0');
        }

        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpVerb = L"runas";
        sei.lpFile = wpath.c_str();
        sei.hwnd = nullptr;
        sei.nShow = SW_NORMAL;

        if (ShellExecuteExW(&sei)) {
            return true;
        }

        DWORD err = GetLastError();
        if (err == ERROR_CANCELLED) {
            System::Logger::Info("User cancelled elevation prompt");
        } else {
            System::Logger::Error("ShellExecuteEx failed (error: " + std::to_string(err) + ")");
        }
        return false;
    }

    std::string Admin::GetExecutablePath() {
        DWORD bufSize = MAX_PATH;
        std::wstring wpath(bufSize, L'\0');

        while (true) {
            DWORD len = GetModuleFileNameW(nullptr, &wpath[0], bufSize);
            if (len == 0) {
                System::Logger::Error("GetModuleFileNameW failed (error: " + std::to_string(GetLastError()) + ")");
                return {};
            }
            if (len < bufSize) {
                wpath.resize(len);
                break;
            }
            bufSize *= 2;
            wpath.resize(bufSize, L'\0');
        }

        int needed = WideCharToMultiByte(CP_UTF8, 0, wpath.data(),
                                         static_cast<int>(wpath.size()),
                                         nullptr, 0, nullptr, nullptr);
        if (needed <= 0) {
            System::Logger::Error("WideCharToMultiByte failed for executable path");
            return {};
        }

        std::string utf8(static_cast<size_t>(needed), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wpath.data(),
                            static_cast<int>(wpath.size()),
                            &utf8[0], needed, nullptr, nullptr);
        return utf8;
    }

}
