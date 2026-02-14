
#pragma once

#include <windows.h>
#include <string>

namespace Vax::System {

    constexpr DWORD kDefaultProcessTimeout = 15000;

    bool RunSilentCommand(const std::string& command,
                          DWORD timeoutMs = kDefaultProcessTimeout,
                          DWORD* outExitCode = nullptr);

    std::string RunCommandCapture(const std::string& command,
                                  DWORD timeoutMs = kDefaultProcessTimeout);

}
