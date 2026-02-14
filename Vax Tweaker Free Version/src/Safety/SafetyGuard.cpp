
#include "SafetyGuard.h"
#include "../UI/Theme.h"
#include "../UI/Console.h"
#include "../UI/Renderer.h"
#include <iostream>

namespace Vax::Safety {

    bool SafetyGuard::ConfirmTweak(const TweakInfo& tweak) {
        using namespace Vax::UI;

        std::cout << "\n";
        std::cout << std::string(Layout::Indent, ' ');
        std::cout << Color::Dim << "┌─────────────────────────────────────────────────────────────────┐\n";

        std::cout << std::string(Layout::Indent, ' ');
        std::cout << "│" << Color::Reset;
        std::string confirmHeader = std::string(Color::Yellow) + "  " + std::string(Icon::Warning) + "  CONFIRM OPERATION" + std::string(Color::Reset);
        std::cout << UI::Renderer::PadRight(confirmHeader, Layout::BoxWidth);
        std::cout << Color::Dim << "│\n";

        std::cout << std::string(Layout::Indent, ' ');
        std::cout << "├─────────────────────────────────────────────────────────────────┤\n" << Color::Reset;

        std::cout << std::string(Layout::Indent, ' ');
        std::cout << Color::Dim << "│" << Color::Reset;
        std::string nameStr = std::string(Color::White) + "  Tweak: " + std::string(Color::Bold) + tweak.name + std::string(Color::Reset);
        std::cout << UI::Renderer::PadRight(nameStr, Layout::BoxWidth);
        std::cout << Color::Dim << "│\n" << Color::Reset;

        std::cout << std::string(Layout::Indent, ' ');
        std::cout << Color::Dim << "│" << Color::Reset;
        std::string descStr = std::string(Color::Gray) + "  " + tweak.description + std::string(Color::Reset);
        std::cout << UI::Renderer::PadRight(descStr, Layout::BoxWidth);
        std::cout << Color::Dim << "│\n" << Color::Reset;

        std::cout << std::string(Layout::Indent, ' ');
        std::cout << Color::Dim << "│" << std::string(Layout::BoxWidth, ' ') << "│\n" << Color::Reset;

        std::string riskStr = GetRiskDescription(tweak.risk);
        std::string riskColor = GetRiskColor(tweak.risk);
        std::cout << std::string(Layout::Indent, ' ');
        std::cout << Color::Dim << "│" << Color::Reset;
        std::string riskLine = "  Risk: " + riskColor + riskStr + std::string(Color::Reset);
        std::cout << UI::Renderer::PadRight(riskLine, Layout::BoxWidth);
        std::cout << Color::Dim << "│\n" << Color::Reset;

        if (tweak.requiresReboot) {
            std::cout << std::string(Layout::Indent, ' ');
            std::cout << Color::Dim << "│" << Color::Reset;
            std::string rebootLine = std::string(Color::Yellow) + "  " + std::string(Icon::Warning) + " Requires system reboot to take effect" + std::string(Color::Reset);
            std::cout << UI::Renderer::PadRight(rebootLine, Layout::BoxWidth);
            std::cout << Color::Dim << "│\n" << Color::Reset;
        }

        std::cout << std::string(Layout::Indent, ' ');
        std::cout << Color::Dim << "└─────────────────────────────────────────────────────────────────┘\n" << Color::Reset;

        std::cout << "\n";
        std::cout << std::string(Layout::Indent, ' ');
        std::cout << Color::Accent << Icon::Arrow << Color::Reset;
        std::cout << " Apply this tweak? ";
        std::cout << Color::Dim << "[" << Color::Reset;
        std::cout << Color::Green << "Y" << Color::Reset;
        std::cout << Color::Dim << "/" << Color::Reset;
        std::cout << Color::Red << "N" << Color::Reset;
        std::cout << Color::Dim << "]" << Color::Reset << ": " << Color::Bold;

        char choice;
        while (true) {
            choice = Console::ReadChar();
            if (choice == 'Y' || choice == 'y' || choice == 'N' || choice == 'n') break;
        }
        std::cout << Color::Reset << "\n";
        return (choice == 'Y' || choice == 'y');
    }

    bool SafetyGuard::ConfirmRevert(const TweakInfo& tweak) {
        using namespace Vax::UI;

        std::cout << "\n";
        std::cout << std::string(Layout::Indent, ' ');
        std::cout << Color::Accent << Icon::Arrow << Color::Reset;
        std::cout << " Revert \"" << Color::White << tweak.name << Color::Reset << "\"? ";
        std::cout << Color::Dim << "[" << Color::Reset;
        std::cout << Color::Green << "Y" << Color::Reset;
        std::cout << Color::Dim << "/" << Color::Reset;
        std::cout << Color::Red << "N" << Color::Reset;
        std::cout << Color::Dim << "]" << Color::Reset << ": " << Color::Bold;

        char choice;
        while (true) {
            choice = Console::ReadChar();
            if (choice == 'Y' || choice == 'y' || choice == 'N' || choice == 'n') break;
        }
        std::cout << Color::Reset << "\n";
        return (choice == 'Y' || choice == 'y');
    }

    bool SafetyGuard::ConfirmApplyAll(const std::string& moduleName, int tweakCount) {
        using namespace Vax::UI;

        std::cout << "\n";
        std::cout << std::string(Layout::Indent, ' ');
        std::cout << Color::Yellow << Icon::Warning << Color::Reset;
        std::cout << " Apply all " << Color::Bold << tweakCount << Color::Reset;
        std::cout << " tweaks in " << Color::White << moduleName << Color::Reset << "? ";
        std::cout << Color::Dim << "[" << Color::Reset;
        std::cout << Color::Green << "Y" << Color::Reset;
        std::cout << Color::Dim << "/" << Color::Reset;
        std::cout << Color::Red << "N" << Color::Reset;
        std::cout << Color::Dim << "]" << Color::Reset << ": " << Color::Bold;

        char choice;
        while (true) {
            choice = Console::ReadChar();
            if (choice == 'Y' || choice == 'y' || choice == 'N' || choice == 'n') break;
        }
        std::cout << Color::Reset << "\n";
        return (choice == 'Y' || choice == 'y');
    }

    void SafetyGuard::ShowRebootNotice() {
        using namespace Vax::UI;

        std::cout << "\n";
        std::cout << std::string(Layout::Indent, ' ');
        std::cout << Color::Yellow << Icon::Warning << Color::Reset;
        std::cout << Color::White << " Some changes require a " << Color::Bold << "system reboot"
                  << Color::Reset << Color::White << " to take effect." << Color::Reset << "\n";
    }

    std::string SafetyGuard::GetRiskDescription(RiskLevel level) {
        switch (level) {
            case RiskLevel::Safe:     return "SAFE - Easily reversible, no system impact";
            case RiskLevel::Moderate: return "MODERATE - Minor system changes, reversible";
            case RiskLevel::Advanced: return "ADVANCED - Significant changes, use with care";
            case RiskLevel::Risky:    return "RISKY - Expert only, potential system issues";
            default:                  return "UNKNOWN";
        }
    }

    std::string SafetyGuard::GetRiskColor(RiskLevel level) {
        using namespace Vax::UI;
        switch (level) {
            case RiskLevel::Safe:     return Color::Green;
            case RiskLevel::Moderate: return Color::Yellow;
            case RiskLevel::Advanced: return Color::Magenta;
            case RiskLevel::Risky:    return Color::Red;
            default:                  return Color::Gray;
        }
    }

}
