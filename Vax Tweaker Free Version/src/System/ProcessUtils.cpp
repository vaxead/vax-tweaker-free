
#include "ProcessUtils.h"
#include "Logger.h"
#include <vector>

namespace Vax::System {

    static bool ContainsDangerousChars(const std::string& command) {
        for (char c : command) {
            if (c == '\n' || c == '\r' || c == '\0') {
                return true;
            }
        }
        return false;
    }

    bool RunSilentCommand(const std::string& command, DWORD timeoutMs, DWORD* outExitCode) {
        if (ContainsDangerousChars(command)) {
            Logger::Error("RunSilentCommand: rejected command with dangerous characters");
            if (outExitCode) *outExitCode = static_cast<DWORD>(-1);
            return false;
        }

        STARTUPINFOA si = {};
        PROCESS_INFORMATION pi = {};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        std::string cmd = "cmd.exe /c " + command;
        std::vector<char> cmdBuf(cmd.begin(), cmd.end());
        cmdBuf.push_back('\0');

        BOOL ok = CreateProcessA(nullptr, cmdBuf.data(), nullptr, nullptr, FALSE,
                                  CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
        if (!ok) {
            if (outExitCode) *outExitCode = static_cast<DWORD>(-1);
            return false;
        }

        DWORD waitResult = WaitForSingleObject(pi.hProcess, timeoutMs);
        DWORD exitCode = 1;
        if (waitResult == WAIT_OBJECT_0) {
            GetExitCodeProcess(pi.hProcess, &exitCode);
        } else {
            TerminateProcess(pi.hProcess, 1);
            exitCode = static_cast<DWORD>(-2);
        }
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (outExitCode) *outExitCode = exitCode;
        return (exitCode == 0);
    }

    std::string RunCommandCapture(const std::string& command, DWORD timeoutMs) {
        std::string result;

        if (ContainsDangerousChars(command)) {
            Logger::Error("RunCommandCapture: rejected command with dangerous characters");
            return result;
        }

        SECURITY_ATTRIBUTES sa = {};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;

        HANDLE hRead = nullptr, hWrite = nullptr;
        if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return result;
        SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA si = {};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        si.hStdOutput = hWrite;
        si.hStdError = hWrite;
        si.wShowWindow = SW_HIDE;

        PROCESS_INFORMATION pi = {};
        std::string cmd = "cmd.exe /c " + command;
        std::vector<char> cmdBuf(cmd.begin(), cmd.end());
        cmdBuf.push_back('\0');

        BOOL ok = CreateProcessA(nullptr, cmdBuf.data(), nullptr, nullptr,
                                  TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
        CloseHandle(hWrite);

        if (!ok) {
            CloseHandle(hRead);
            return result;
        }

        constexpr size_t kMaxOutputBytes = 1024 * 1024;
        char buffer[4096];
        DWORD bytesRead = 0;
        while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            result += buffer;
            if (result.size() >= kMaxOutputBytes) {
                Logger::Warning("RunCommandCapture: output truncated at 1 MB");
                break;
            }
        }

        WaitForSingleObject(pi.hProcess, timeoutMs);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hRead);
        return result;
    }

}
