
#pragma once

namespace Vax::UI {

    namespace Color {
        constexpr const char* Reset     = "\033[0m";
        constexpr const char* Bold      = "\033[1m";
        constexpr const char* Dim       = "\033[2m";
        constexpr const char* Italic    = "\033[3m";
        constexpr const char* Underline = "\033[4m";

        constexpr const char* Red       = "\033[91m";
        constexpr const char* Green     = "\033[92m";
        constexpr const char* Yellow    = "\033[93m";
        constexpr const char* Blue      = "\033[94m";
        constexpr const char* Magenta   = "\033[95m";
        constexpr const char* Cyan      = "\033[96m";
        constexpr const char* White     = "\033[97m";

        constexpr const char* Accent    = "\033[38;5;45m";
        constexpr const char* Accent2   = "\033[38;5;213m";
        constexpr const char* Gold      = "\033[38;5;220m";
        constexpr const char* Gray      = "\033[38;5;245m";
        constexpr const char* DarkGray  = "\033[38;5;240m";

        constexpr const char* BgDark    = "\033[48;5;236m";
    }

    namespace Icon {
        constexpr const char* Success   = "✓";
        constexpr const char* Error     = "✕";
        constexpr const char* Warning   = "⚠";
        constexpr const char* Info      = "ℹ";
        constexpr const char* Arrow     = "»";
        constexpr const char* Dot       = "●";
        constexpr const char* Circle    = "○";
        constexpr const char* Star      = "★";

        constexpr const char* Fps       = "⚡";
        constexpr const char* Privacy   = "🔒";
        constexpr const char* Gaming    = "🎮";
        constexpr const char* Network   = "🌐";
        constexpr const char* Analysis  = "📊";
        constexpr const char* Cleaner   = "🧹";
        constexpr const char* Startup   = "🚀";
        constexpr const char* Debloater = "🗑";
        constexpr const char* Security  = "🛡";
    }

    namespace Box {
        constexpr const char* TopLeft       = "┌";
        constexpr const char* TopRight      = "┐";
        constexpr const char* BottomLeft    = "└";
        constexpr const char* BottomRight   = "┘";
        constexpr const char* Horizontal    = "─";
        constexpr const char* Vertical      = "│";
        constexpr const char* LeftTee       = "├";
        constexpr const char* RightTee      = "┤";
    }

    namespace Layout {
        constexpr int BoxWidth      = 65;
        constexpr int Indent        = 4;
        constexpr int MenuPadding   = 8;
        constexpr int PageSize      = 10;
    }

}
