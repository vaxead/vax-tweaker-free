
#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>

namespace Vax::UI {

    class Console {
    public:
        static void Initialize();

        static void Cleanup();

        static void Clear();

        static void SetTitle(const std::string& title);

        static void WaitForKey();

        static char ReadChar();

        static std::string ReadLine();

        static int ReadInt();

        static std::string GetWindowsVersionString();

        static int GetWindowsBuildNumber();

    private:
        static void EnableVirtualTerminal();

        static void EnableUtf8();

        static void SetupFont();

        static void SetupWindowSize();

        static CONSOLE_FONT_INFOEX s_originalFont;
        static bool s_fontSaved;

        Console() = default;
    };

}
