
#include "Renderer.h"
#include "../System/Registry.h"
#include "Console.h"
#include "Theme.h"
#include <iomanip>
#include <iostream>
#include <unordered_map>

namespace Vax::UI {

void Renderer::DrawLogo() {
  std::cout << "\n";
  std::cout << Color::Accent << Color::Bold;
  std::cout << "    ██╗   ██╗ █████╗ ██╗  ██╗\n";
  std::cout << "    ██║   ██║██╔══██╗╚██╗██╔╝\n";
  std::cout << "    ██║   ██║███████║ ╚███╔╝ \n";
  std::cout << "    ╚██╗ ██╔╝██╔══██║ ██╔██╗ \n";
  std::cout << "     ╚████╔╝ ██║  ██║██╔╝ ██╗\n";
  std::cout << "      ╚═══╝  ╚═╝  ╚═╝╚═╝  ╚═╝\n";
  std::cout << Color::Reset;
  std::cout << Color::Gray << "         T W E A K E R" << Color::Reset << "  ";
  std::cout << Color::Dim << "v" << APP_VERSION << Color::Reset << "\n";
}

void Renderer::DrawStatusBar(bool isAdmin) {
  std::cout << "\n";
  std::cout << Color::Dim;
  PrintIndent();
  std::cout << "┌──────────────────────────────────────────────────────────────"
               "───┐\n";

  PrintIndent();
  std::cout << "│" << Color::Reset;

  std::string content;
  if (isAdmin) {
    content +=
        std::string(Color::Green) + " " + Icon::Dot + " ADMIN" + Color::Reset;
  } else {
    content += std::string(Color::Yellow) + " " + Icon::Circle + " USER " +
               Color::Reset;
  }

  content += std::string(Color::Dim) + "  │  " + Color::Reset;
  content += std::string(Color::Cyan) + Console::GetWindowsVersionString() +
             Color::Reset + std::string(Color::Dim) + " (" +
             std::to_string(Console::GetWindowsBuildNumber()) + ")" +
             Color::Reset;
  content += std::string(Color::Dim) + "  │  " + Color::Reset;
  content += std::string(Color::Green) + "READY" + Color::Reset;
  content += std::string(Color::Dim) + "  │  " + Color::Reset;
  content += std::string(Color::Cyan) + Icon::Star + " FREE" + Color::Reset;

  std::cout << PadRight(content, Layout::BoxWidth);
  std::cout << Color::Dim << "│\n";

  PrintIndent();
  std::cout
      << "└─────────────────────────────────────────────────────────────────┘";
  std::cout << Color::Reset << "\n";
}

void Renderer::DrawMainMenu(const std::vector<ModuleInfo> &modules) {
  std::cout << "\n";

  for (const auto &module : modules) {
    PrintIndent();
    std::cout << Color::Accent << "[" << module.id << "]" << Color::Reset
              << " ";
    std::cout << Color::Bold << Color::White << std::left << std::setw(22)
              << module.name << Color::Reset;
    std::cout << "  " << Color::Gray << module.description << Color::Reset
              << "\n";
  }

  std::cout << "\n";
  std::cout << Color::Dim;
  PrintIndent();
  std::cout << "───────────────────────────────────────────────────────────────"
               "────\n";
  std::cout << Color::Reset;

  size_t backupCount = System::Registry::GetBackupEntries().size();
  if (backupCount > 0) {
    PrintIndent();
    std::cout << Color::Accent << "[R]" << Color::Reset << " ";
    std::cout << "  Restore All Changes\n";
  }

  PrintIndent();
  std::cout << Color::Dim << "[0]" << Color::Reset << " ";
  std::cout << Color::Red << Icon::Error << Color::Reset << "  Exit\n";
}

void Renderer::DrawFooter() {
  std::cout << "\n";
  std::cout << Color::Dim;
  PrintIndent();
  std::cout << "┌──────────────────────────────────────────────────────────────"
               "───┐\n";
  PrintIndent();
  std::cout << "│" << Color::Reset;
  std::string footerContent = std::string(Color::Gray) +
                              "  discord.gg/myYrWkn7" + Color::Reset +
                              "  " + std::string(Color::Dim) + "│" +
                              Color::Reset + std::string(Color::Gray) +
                              " github.com/vaxead/vax-tweaker-free" + Color::Reset;
  std::cout << PadRight(footerContent, Layout::BoxWidth);
  std::cout << Color::Dim << "│\n";
  PrintIndent();
  std::cout
      << "└─────────────────────────────────────────────────────────────────┘";
  std::cout << Color::Reset << "\n";
}

void Renderer::DrawPrompt() {
  std::cout << "\n";
  PrintIndent();
  std::cout << Color::Accent << Icon::Arrow << Color::Reset;
  std::cout << " Select option: " << Color::Bold;
}

static std::string GetCategoryName(ModuleCategory cat) {
  switch (cat) {
  case ModuleCategory::Performance:
    return "PERFORMANCE";
  case ModuleCategory::Network:
    return "NETWORK & PRIVACY";
  case ModuleCategory::Maintenance:
    return "SYSTEM MAINTENANCE";
  case ModuleCategory::Tools:
    return "TOOLS & PROFILES";
  case ModuleCategory::System:
    return "SYSTEM";
  case ModuleCategory::Privacy:
    return "PRIVACY";
  default:
    return "MODULE";
  }
}

void Renderer::DrawBreadcrumbs(const ModuleInfo &module,
                               const std::string &subSection) {
  std::cout << "\n";
  std::cout << Color::Dim;
  Renderer::PrintIndent();
  std::cout << "HOME \xc2\xbb " << GetCategoryName(module.category)
            << " \xc2\xbb " << Color::Reset;
  std::cout << Color::Accent << Color::Bold << module.name << Color::Reset;

  if (!subSection.empty()) {
    std::cout << Color::Dim << " \xc2\xbb " << Color::Reset;
    std::cout << Color::White << Color::Bold << subSection << Color::Reset;
  }
  std::cout << "\n";
}

void Renderer::DrawModuleView(const ModuleInfo &module) {
  DrawLogo();
  DrawBreadcrumbs(module);
  DrawTitleBox(module.icon, module.name, Color::Accent);
}

void Renderer::DrawTweakList(const ModuleInfo &module,
                             const std::vector<TweakInfo> &tweaks,
                             bool showStatus) {
  DrawLogo();
  DrawBreadcrumbs(module);
  DrawTitleBox(module.icon, module.name, Color::Accent);

  std::cout << "\n";
  PrintIndent();
  std::cout << Color::Dim << "--- " << Color::Reset;
  std::cout << Color::Bold << Color::White << "AVAILABLE TWEAKS"
            << Color::Reset;
  std::cout << Color::Dim << " ---\n" << Color::Reset;
  std::cout << "\n";

  for (size_t i = 0; i < tweaks.size(); ++i) {
    const auto &tweak = tweaks[i];
    std::string statusIcon, statusColor;
    bool hasStatusIcon = false;
    if (showStatus && tweak.status != TweakStatus::Unknown) {
      hasStatusIcon = true;
      switch (tweak.status) {
      case TweakStatus::Applied:
        statusIcon = Icon::Success;
        statusColor = Color::Green;
        break;
      case TweakStatus::NotApplied:
        statusIcon = Icon::Circle;
        statusColor = Color::Gray;
        break;
      case TweakStatus::Partial:
        statusIcon = Icon::Dot;
        statusColor = Color::Yellow;
        break;
      case TweakStatus::Error:
        statusIcon = Icon::Error;
        statusColor = Color::Red;
        break;
      default:
        hasStatusIcon = false;
        break;
      }
    }

    PrintIndent();
    std::cout << Color::Accent << "[" << (i + 1) << "]" << Color::Reset << " ";
    if (hasStatusIcon)
      std::cout << statusColor << statusIcon << Color::Reset << "  ";
    std::cout << Color::Bold << Color::White << tweak.name << Color::Reset
              << "\n";

    PrintIndent();
    std::cout << "    " << Color::Gray << tweak.description << Color::Reset;

    std::string riskLabel, riskColor;
    switch (tweak.risk) {
    case RiskLevel::Safe:
      riskLabel = "SAFE";
      riskColor = Color::Green;
      break;
    case RiskLevel::Moderate:
      riskLabel = "MOD";
      riskColor = Color::Yellow;
      break;
    case RiskLevel::Advanced:
      riskLabel = "ADV";
      riskColor = Color::Magenta;
      break;
    case RiskLevel::Risky:
      riskLabel = "RISK";
      riskColor = Color::Red;
      break;
    }
    std::cout << "  " << Color::Dim << "[" << Color::Reset << riskColor
              << riskLabel << Color::Reset << Color::Dim << "]" << Color::Reset;
    if (tweak.requiresReboot)
      std::cout << " " << Color::Yellow << Icon::Warning << Color::Reset;
    std::cout << "\n\n";
  }

  std::cout << Color::Dim;
  PrintIndent();
  std::cout
      << "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
         "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
         "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
         "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
         "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
         "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
         "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
         "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
         "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
         "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
         "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
         "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
         "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\n";
  std::cout << Color::Reset;

  std::cout << "\n";
  PrintIndent();
  std::cout << Color::Accent << "[A]" << Color::Reset << "  Apply All\n";
  if (showStatus) {
    PrintIndent();
    std::cout << Color::Accent << "[R]" << Color::Reset << "  Revert All\n";
  }
  PrintIndent();
  std::cout << Color::Dim << "[0]" << Color::Reset << "  Back to Main Menu\n";

  std::cout << "\n";
  PrintIndent();
  std::cout << Color::Accent;
  std::cout << " Select tweak or action: " << Color::Bold;
}

void Renderer::DrawGroupList(const ModuleInfo &module,
                             const std::vector<TweakGroup> &groups,
                             const std::vector<TweakInfo> &allTweaks, int page,
                             int totalPages, bool showStatus) {
  DrawLogo();
  DrawBreadcrumbs(module);
  DrawTitleBox(module.icon, module.name, Color::Accent);

  std::cout << "\n";
  PrintIndent();
  std::cout << Color::Dim << "--- " << Color::Reset;
  std::cout << Color::Bold << Color::White << "SELECT CATEGORY" << Color::Reset;
  std::cout << Color::Dim << " ---\n" << Color::Reset;
  std::cout << "\n";

  int startIdx = page * Layout::PageSize;
  int endIdx = startIdx + Layout::PageSize;
  if (endIdx > static_cast<int>(groups.size()))
    endIdx = static_cast<int>(groups.size());

  for (int i = startIdx; i < endIdx; ++i) {
    const auto &group = groups[i];

    int groupTotal = 0, groupApplied = 0;
    for (const auto &tweakId : group.tweakIds) {
      for (const auto &tweak : allTweaks) {
        if (tweak.id == tweakId) {
          ++groupTotal;
          if (tweak.status == TweakStatus::Applied)
            ++groupApplied;
          break;
        }
      }
    }

    std::string numStr = std::string(Color::Accent) + "[" +
                         std::to_string(i + 1) + "]" +
                         std::string(Color::Reset);
    std::string leftContent =
        numStr + " " + group.icon + "  " + std::string(Color::Bold) +
        std::string(Color::White) + group.name + std::string(Color::Reset);

    std::string rightContent;
    if (showStatus) {
      rightContent = std::string(Color::Green) + std::to_string(groupApplied) +
                     std::string(Color::Reset) + std::string(Color::Dim) + "/" +
                     std::string(Color::Reset) + std::string(Color::White) +
                     std::to_string(groupTotal) + std::string(Color::Reset) +
                     " " + std::string(Color::Gray) + "applied" +
                     std::string(Color::Reset);
    } else {
      rightContent = std::string(Color::Gray) + std::to_string(groupTotal) +
                     " tweaks" + std::string(Color::Reset);
    }

    int leftLen = VisibleLength(leftContent);
    int rightLen = VisibleLength(rightContent);
    int gap = Layout::BoxWidth - leftLen - rightLen;
    if (gap < 2)
      gap = 2;

    PrintIndent();
    std::cout << leftContent << std::string(gap, ' ') << rightContent << "\n";

    PrintIndent();
    std::cout << "    " << Color::Gray << group.description << Color::Reset
              << "\n";
    std::cout << "\n";
  }

  if (totalPages > 1) {
    PrintIndent();
    std::cout << Color::Dim << "Page " << (page + 1) << " of " << totalPages;
    if (page > 0)
      std::cout << "  [Z] Prev";
    if (page < totalPages - 1)
      std::cout << "  [X] Next";
    std::cout << Color::Reset << "\n";
  }

  std::cout << Color::Dim;
  PrintIndent();
  for (int i = 0; i < 67; ++i)
    std::cout << "\xe2\x94\x80";
  std::cout << "\n";
  std::cout << Color::Reset;

  int totalTweakCount = static_cast<int>(allTweaks.size());
  std::cout << "\n";
  PrintIndent();
  std::cout << Color::Accent << "[A]" << Color::Reset << "  Apply All ("
            << totalTweakCount << " tweaks)\n";
  if (showStatus) {
    PrintIndent();
    std::cout << Color::Accent << "[R]" << Color::Reset << "  Revert All\n";
  }
  PrintIndent();
  std::cout << Color::Dim << "[0]" << Color::Reset << "  Back to Main Menu\n";

  std::cout << "\n";
  PrintIndent();
  std::cout << Color::Accent;
  std::cout << " Select category or action: " << Color::Bold;
}

void Renderer::DrawGroupTweakList(const ModuleInfo &module,
                                  const TweakGroup &group,
                                  const std::vector<TweakInfo> &tweaks,
                                  int groupIndex, int totalGroups, int page,
                                  int totalPages, bool showStatus) {
  DrawLogo();
  DrawBreadcrumbs(module, group.name);
  DrawTitleBox(group.icon, group.name, Color::Accent);

  if (showStatus) {
    int appliedCount = 0;
    int totalCount = static_cast<int>(tweaks.size());
    for (const auto &t : tweaks) {
      if (t.status == TweakStatus::Applied)
        appliedCount++;
    }
    std::cout << "\n";
    PrintIndent();
    std::cout << Color::Dim << "Status: " << Color::Reset;
    std::cout << Color::Green << appliedCount << Color::Reset << "/"
              << totalCount << " tweaks applied";
    if (totalPages > 1) {
      std::cout << Color::Dim << "  (Page " << (page + 1) << "/" << totalPages
                << ")" << Color::Reset;
    }
    std::cout << "\n";
  }
  std::cout << "\n";

  /*
    std::cout << "\n";
    std::cout << Color::Dim;
    PrintIndent();
    std::cout << "\xe2\x94\x8c\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
                 "\xe2\x94"
                 "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
                 "\x80\xe2"
                 "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
                 "\x94\x80"
                 "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
                 "\xe2\x94"
                 "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
                 "\x80\xe2"
                 "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
                 "\x94\x80"
                 "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
                 "\xe2\x94"
                 "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
                 "\x80\xe2"
                 "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
                 "\x94\x80"
                 "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
                 "\xe2\x94"
                 "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
                 "\x80\xe2"
                 "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
                 "\x94\x80"
                 "\xe2\x94\x80\xe2\x94\x90\n";
    PrintIndent();
    std::cout << "\xe2\x94\x82" << Color::Reset;
    std::string breadcrumb =
        "  " + std::string(module.icon) + "  " + std::string(Color::Bold) +
        std::string(Color::Accent) + module.name + std::string(Color::Reset) +
        "  " + std::string(Color::Dim) + ">>" + std::string(Color::Reset) +
        "  " + std::string(Color::Bold) + std::string(Color::White) +
        group.name + std::string(Color::Reset);
    std::cout << PadRight(breadcrumb, Layout::BoxWidth);
    std::cout << Color::Dim << "\xe2\x94\x82\n";

    if (showStatus) {
      int appliedCount = 0;
      int totalCount = static_cast<int>(tweaks.size());
      for (const auto &t : tweaks) {
        if (t.status == TweakStatus::Applied)
          ++appliedCount;
      }
      PrintIndent();
      std::cout
          << "\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
             "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
             "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
             "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
             "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
             "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
             "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
             "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
             "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
             "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
             "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
             "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
             "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
             "\x94\xa4\n";
      PrintIndent();
      std::cout << "\xe2\x94\x82" << Color::Reset;
      std::string leftStat =
          "  " + std::string(Color::Green) + std::to_string(appliedCount) +
          std::string(Color::Reset) + std::string(Color::White) + " of " +
          std::to_string(totalCount) + " applied" + std::string(Color::Reset);
      std::string rightStat;
      if (totalPages > 1) {
        rightStat = std::string(Color::Gray) + "Page " +
                    std::to_string(page + 1) + " of " +
                    std::to_string(totalPages) + std::string(Color::Reset);
      }
      if (!rightStat.empty()) {
        int lLen = VisibleLength(leftStat);
        int rLen = VisibleLength(rightStat);
        int g = Layout::BoxWidth - lLen - rLen;
        if (g < 2)
          g = 2;
        std::cout << leftStat << std::string(g, ' ') << rightStat;
      } else {
        std::cout << PadRight(leftStat, Layout::BoxWidth);
      }
      std::cout << Color::Dim << "\xe2\x94\x82\n";
    }

    PrintIndent();
    std::cout << "\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
                 "\xe2\x94"
                 "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
                 "\x80\xe2"
                 "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
                 "\x94\x80"
                 "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
                 "\xe2\x94"
                 "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
                 "\x80\xe2"
                 "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
                 "\x94\x80"
                 "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
                 "\xe2\x94"
                 "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
                 "\x80\xe2"
                 "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
                 "\x94\x80"
                 "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
                 "\xe2\x94"
                 "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
                 "\x80\xe2"
                 "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
                 "\x94\x80"
                 "\xe2\x94\x80\xe2\x94\x98";
    std::cout << Color::Reset << "\n";

    std::string upperName = group.name;
    for (auto &c : upperName) {
      if (c >= 'a' && c <= 'z')
        c = c - 'a' + 'A';
    }
    std::cout << "\n";
    PrintIndent();
    std::cout << Color::Dim << "--- " << Color::Reset;
    std::cout << Color::Bold << Color::White << upperName << Color::Reset;
    std::cout << Color::Dim << " ---\n" << Color::Reset;
    std::cout << "\n";
  */

  int startIdx = page * Layout::PageSize;
  int endIdx = startIdx + Layout::PageSize;
  if (endIdx > static_cast<int>(tweaks.size()))
    endIdx = static_cast<int>(tweaks.size());

  for (int i = startIdx; i < endIdx; ++i) {
    const auto &tweak = tweaks[i];
    std::string statusIcon, statusColor;
    bool hasStatusIcon = false;
    if (showStatus && tweak.status != TweakStatus::Unknown) {
      hasStatusIcon = true;
      switch (tweak.status) {
      case TweakStatus::Applied:
        statusIcon = Icon::Success;
        statusColor = Color::Green;
        break;
      case TweakStatus::NotApplied:
        statusIcon = Icon::Circle;
        statusColor = Color::Gray;
        break;
      case TweakStatus::Partial:
        statusIcon = Icon::Dot;
        statusColor = Color::Yellow;
        break;
      case TweakStatus::Error:
        statusIcon = Icon::Error;
        statusColor = Color::Red;
        break;
      default:
        hasStatusIcon = false;
        break;
      }
    }

    PrintIndent();
    std::cout << Color::Accent << "[" << (i + 1) << "]" << Color::Reset << " ";
    if (hasStatusIcon)
      std::cout << statusColor << statusIcon << Color::Reset << "  ";
    std::cout << Color::Bold << Color::White << tweak.name << Color::Reset
              << "\n";

    PrintIndent();
    std::cout << "    " << Color::Gray << tweak.description << Color::Reset;

    std::string riskLabel, riskColor;
    switch (tweak.risk) {
    case RiskLevel::Safe:
      riskLabel = "SAFE";
      riskColor = Color::Green;
      break;
    case RiskLevel::Moderate:
      riskLabel = "MOD";
      riskColor = Color::Yellow;
      break;
    case RiskLevel::Advanced:
      riskLabel = "ADV";
      riskColor = Color::Magenta;
      break;
    case RiskLevel::Risky:
      riskLabel = "RISK";
      riskColor = Color::Red;
      break;
    }
    std::cout << "  " << Color::Dim << "[" << Color::Reset << riskColor
              << riskLabel << Color::Reset << Color::Dim << "]" << Color::Reset;
    if (tweak.requiresReboot)
      std::cout << " " << Color::Yellow << Icon::Warning << Color::Reset;
    std::cout << "\n\n";
  }

  if (totalPages > 1) {
    PrintIndent();
    std::cout << Color::Dim << "Page " << (page + 1) << " of " << totalPages;
    if (page > 0)
      std::cout << "  [Z] Prev Page";
    if (page < totalPages - 1)
      std::cout << "  [X] Next Page";
    std::cout << Color::Reset << "\n";
  }

  std::cout << Color::Dim;
  PrintIndent();
  std::cout << "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
               "\xe2\x94"
               "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
               "\x80\xe2"
               "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
               "\x94\x80"
               "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
               "\xe2\x94"
               "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
               "\x80\xe2"
               "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
               "\x94\x80"
               "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
               "\xe2\x94"
               "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
               "\x80\xe2"
               "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
               "\x94\x80"
               "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
               "\xe2\x94"
               "\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94"
               "\x80\xe2"
               "\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2"
               "\x94\x80"
               "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\n";
  std::cout << Color::Reset;

  int tweakCount = static_cast<int>(tweaks.size());
  std::cout << "\n";

  {
    std::string leftStr = std::string(Color::Accent) + "[A]" +
                          std::string(Color::Reset) + "  Apply All (" +
                          std::to_string(tweakCount) + " tweaks)";
    std::string rightStr;
    if (groupIndex < totalGroups - 1) {
      rightStr = std::string(Color::Accent) + "[N]" +
                 std::string(Color::Reset) + " Next Category";
    }
    PrintIndent();
    if (!rightStr.empty()) {
      int lLen = VisibleLength(leftStr);
      int rLen = VisibleLength(rightStr);
      int pad = Layout::BoxWidth - lLen - rLen;
      if (pad < 2)
        pad = 2;
      std::cout << leftStr << std::string(pad, ' ') << rightStr << "\n";
    } else {
      std::cout << leftStr << "\n";
    }
  }

  {
    std::string leftStr;
    std::string rightStr;
    if (showStatus) {
      leftStr = std::string(Color::Accent) + "[R]" + std::string(Color::Reset) +
                "  Revert All";
    }
    if (groupIndex > 0) {
      rightStr = std::string(Color::Accent) + "[P]" +
                 std::string(Color::Reset) + " Prev Category";
    }
    if (!leftStr.empty() || !rightStr.empty()) {
      PrintIndent();
      if (!leftStr.empty() && !rightStr.empty()) {
        int lLen = VisibleLength(leftStr);
        int rLen = VisibleLength(rightStr);
        int pad = Layout::BoxWidth - lLen - rLen;
        if (pad < 2)
          pad = 2;
        std::cout << leftStr << std::string(pad, ' ') << rightStr << "\n";
      } else if (!leftStr.empty()) {
        std::cout << leftStr << "\n";
      } else {
        int rLen = VisibleLength(rightStr);
        int pad = Layout::BoxWidth - rLen;
        if (pad < 0)
          pad = 0;
        std::cout << std::string(pad, ' ') << rightStr << "\n";
      }
    }
  }

  PrintIndent();
  std::cout << Color::Dim << "[0]" << Color::Reset << "  Back to Categories\n";

  std::cout << "\n";
  PrintIndent();
  std::cout << Color::Accent;
  std::cout << " Select tweak or action: " << Color::Bold;
}

void Renderer::DrawTweakResult(const std::string &tweakName, bool success,
                               bool isRevert, const std::string &failReason) {
  PrintIndent();
  if (success) {
    std::cout << Color::Green << Icon::Success << Color::Reset;
    std::cout << "  " << Color::White << tweakName << Color::Reset;
    if (isRevert)
      std::cout << Color::Green << " — Reverted successfully" << Color::Reset
                << "\n";
    else
      std::cout << Color::Green << " — Applied successfully" << Color::Reset
                << "\n";
  } else {
    std::cout << Color::Red << Icon::Error << Color::Reset;
    std::cout << "  " << Color::White << tweakName << Color::Reset;
    if (isRevert)
      std::cout << Color::Red << " — Failed to revert" << Color::Reset << "\n";
    else
      std::cout << Color::Red << " — Failed to apply" << Color::Reset << "\n";
    if (!failReason.empty()) {
      PrintIndent();
      std::cout << "     " << Color::Yellow << Icon::Warning << " "
                << failReason << Color::Reset << "\n";
    }
  }
}

void Renderer::DrawProgressHeader(const std::string &moduleName,
                                  const std::string &operation) {
  std::cout << "\n";
  PrintIndent();
  std::cout << Color::Accent << Color::Bold << operation << Color::Reset;
  std::cout << Color::Dim << " — " << Color::Reset;
  std::cout << Color::White << moduleName << Color::Reset << "\n";
  PrintIndent();
  std::cout << Color::Dim
            << "────────────────────────────────────────────────────\n"
            << Color::Reset;
  std::cout << "\n";
}

void Renderer::DrawProgressFooter(int succeeded, int failed) {
  std::cout << "\n";
  PrintIndent();
  std::cout << Color::Dim
            << "────────────────────────────────────────────────────\n"
            << Color::Reset;
  PrintIndent();
  std::cout << Color::Green << Icon::Success << " " << succeeded << " succeeded"
            << Color::Reset;
  if (failed > 0) {
    std::cout << "  " << Color::Red << Icon::Error << " " << failed << " failed"
              << Color::Reset;
  }
  std::cout << "\n\n";
  PrintIndent();
  std::cout << Color::Dim << "Press any key to return..." << Color::Reset;
}

void Renderer::DrawRestorePointPrompt() {
  DrawLogo();
  std::cout << "\n";

  std::cout << Color::Dim;
  PrintIndent();
  std::cout << "┌──────────────────────────────────────────────────────────────"
               "───┐\n";

  PrintIndent();
  std::cout << "│" << Color::Reset;
  std::string headerContent =
      std::string(Color::Cyan) + "  " + std::string(Icon::Info) +
      "  SYSTEM RESTORE POINT" + std::string(Color::Reset);
  std::cout << PadRight(headerContent, Layout::BoxWidth);
  std::cout << Color::Dim << "│\n";

  PrintIndent();
  std::cout << "├──────────────────────────────────────────────────────────────"
               "───┤\n";

  DrawBoxEmptyLine();
  DrawBoxTextLine("   It is recommended to create a System Restore Point",
                  Color::White);
  DrawBoxTextLine("   before applying any system modifications.", Color::White);
  DrawBoxEmptyLine();
  DrawBoxTextLine("   This allows you to roll back all Windows changes",
                  Color::Gray);
  DrawBoxTextLine("   if anything goes wrong.", Color::Gray);
  DrawBoxEmptyLine();
  DrawBoxTextLine("   Restore point name: \"VaxTweaker Restore Point\"",
                  Color::Cyan);
  DrawBoxEmptyLine();

  PrintIndent();
  std::cout << "└────────────────────────────────────────────────────────────"
               "─────┘";
  std::cout << Color::Reset << "\n";

  std::cout << "\n";
  PrintIndent();
  std::cout << Color::Accent << Icon::Arrow << Color::Reset;
  std::cout << " Create Restore Point? ";
  std::cout << Color::Dim << "[" << Color::Reset;
  std::cout << Color::Green << "Y" << Color::Reset;
  std::cout << Color::Dim << "/" << Color::Reset;
  std::cout << Color::Red << "N" << Color::Reset;
  std::cout << Color::Dim << "]" << Color::Reset << ": " << Color::Bold;
}

void Renderer::DrawRestoreAllPrompt(int backupCount) {
  DrawLogo();
  std::cout << "\n";

  std::cout << Color::Dim;
  PrintIndent();
  std::cout << "┌──────────────────────────────────────────────────────────────"
               "───┐\n";

  PrintIndent();
  std::cout << "│" << Color::Reset;
  std::string headerContent = std::string(Color::Yellow) + "  " +
                              Icon::Warning + "  RESTORE ALL CHANGES" +
                              std::string(Color::Reset);
  std::cout << PadRight(headerContent, Layout::BoxWidth);
  std::cout << Color::Dim << "│\n";

  PrintIndent();
  std::cout << "├──────────────────────────────────────────────────────────────"
               "───┤\n";

  DrawBoxEmptyLine();
  DrawBoxTextLine("   This will restore all registry modifications made by",
                  Color::White);
  DrawBoxTextLine("   VAX TWEAKER (" + std::to_string(backupCount) +
                      " backup entries).",
                  Color::White);
  DrawBoxEmptyLine();
  DrawBoxTextLine("   File cleaning operations (System Cleaner) cannot",
                  Color::Gray);
  DrawBoxTextLine("   be undone. Only registry changes will be restored.",
                  Color::Gray);
  DrawBoxEmptyLine();

  PrintIndent();
  std::cout << "└────────────────────────────────────────────────────────────"
               "─────┘";
  std::cout << Color::Reset << "\n";

  std::cout << "\n";
  PrintIndent();
  std::cout << Color::Accent << Icon::Arrow << Color::Reset;
  std::cout << " Restore all changes? ";
  std::cout << Color::Dim << "[" << Color::Reset;
  std::cout << Color::Green << "Y" << Color::Reset;
  std::cout << Color::Dim << "/" << Color::Reset;
  std::cout << Color::Red << "N" << Color::Reset;
  std::cout << Color::Dim << "]" << Color::Reset << ": " << Color::Bold;
}

void Renderer::DrawDevelopmentNotice(const ModuleInfo &module) {
  DrawLogo();
  std::cout << "\n";

  std::cout << Color::Dim;
  PrintIndent();
  std::cout << "┌──────────────────────────────────────────────────────────────"
               "───┐\n";

  PrintIndent();
  std::cout << "│" << Color::Reset;
  std::string devContent =
      "  " + std::string(module.icon) + "  " + std::string(Color::Bold) +
      std::string(Color::Accent) + module.name + std::string(Color::Reset);
  std::cout << PadRight(devContent, Layout::BoxWidth);
  std::cout << Color::Dim << "│\n";

  PrintIndent();
  std::cout << "├──────────────────────────────────────────────────────────────"
               "───┤\n";

  DrawBoxEmptyLine();
  DrawBoxTextLine("   " + std::string(Icon::Warning) +
                      "  DEVELOPMENT IN PROGRESS",
                  Color::Yellow);
  DrawBoxEmptyLine();
  DrawBoxTextLine("   This module is currently being developed.", Color::Gray);
  DrawBoxTextLine("   Follow our GitHub for updates and release dates.",
                  Color::Gray);
  DrawBoxEmptyLine();

  PrintIndent();
  std::cout << "└────────────────────────────────────────────────────────────"
               "─────┘";
  std::cout << Color::Reset << "\n";

  std::cout << "\n";
  PrintIndent();
  std::cout << Color::Dim << "Press any key to return..." << Color::Reset;
}

void Renderer::DrawAdminPrompt() {
  DrawLogo();
  std::cout << "\n";

  std::cout << Color::Dim;
  PrintIndent();
  std::cout << "┌──────────────────────────────────────────────────────────────"
               "───┐\n";

  PrintIndent();
  std::cout << "│" << Color::Reset;
  std::string adminContent = std::string(Color::Yellow) + "  " + Icon::Warning +
                             "  ADMINISTRATOR PRIVILEGES REQUIRED" +
                             std::string(Color::Reset);
  std::cout << PadRight(adminContent, Layout::BoxWidth);
  std::cout << Color::Dim << "│\n";

  PrintIndent();
  std::cout << "├──────────────────────────────────────────────────────────────"
               "───┤\n";

  DrawBoxEmptyLine();
  DrawBoxTextLine("   VAX TWEAKER requires elevated privileges to:",
                  Color::White);
  DrawBoxEmptyLine();
  DrawBoxTextLine("   • Modify system registry for performance tweaks",
                  Color::Gray);
  DrawBoxTextLine("   • Configure Windows services and startup items",
                  Color::Gray);
  DrawBoxTextLine("   • Apply network and power optimizations", Color::Gray);
  DrawBoxEmptyLine();

  PrintIndent();
  std::cout << "└────────────────────────────────────────────────────────────"
               "─────┘";
  std::cout << Color::Reset << "\n";

  std::cout << "\n";
  PrintIndent();
  std::cout << Color::Accent << Icon::Arrow << Color::Reset;
  std::cout << " Restart as Administrator? ";
  std::cout << Color::Dim << "[" << Color::Reset;
  std::cout << Color::Green << "Y" << Color::Reset;
  std::cout << Color::Dim << "/" << Color::Reset;
  std::cout << Color::Red << "N" << Color::Reset;
  std::cout << Color::Dim << "]" << Color::Reset << ": " << Color::Bold;
}

void Renderer::DrawExitScreen() {
  DrawLogo();
  std::cout << "\n";

  std::cout << Color::Dim;
  PrintIndent();
  std::cout << "┌──────────────────────────────────────────────────────────────"
               "───┐\n";

  PrintIndent();
  std::cout << "│" << Color::Reset;
  std::string exitContent = std::string(Color::Accent) + "  " + Icon::Success +
                            "  SESSION COMPLETE" + std::string(Color::Reset);
  std::cout << PadRight(exitContent, Layout::BoxWidth);
  std::cout << Color::Dim << "│\n";

  PrintIndent();
  std::cout << "├──────────────────────────────────────────────────────────────"
               "───┤\n";

  DrawBoxEmptyLine();
  DrawBoxTextLine("   Thank you for using VAX TWEAKER", Color::White);
  DrawBoxTextLine("   Your system. Your rules. Maximum performance.",
                  Color::Gray);
  DrawBoxEmptyLine();

  PrintIndent();
  std::cout << "└────────────────────────────────────────────────────────────"
               "─────┘";
  std::cout << Color::Reset << "\n\n";
}

void Renderer::DrawDisclaimer() {
  DrawLogo();
  std::cout << "\n";

  std::cout << Color::Dim;
  PrintIndent();
  std::cout << "┌──────────────────────────────────────────────────────────────"
               "───┐\n";

  PrintIndent();
  std::cout << "│" << Color::Reset;
  std::string header = std::string(Color::Yellow) + "  " + Icon::Warning +
                       "  DISCLAIMER — TERMS OF USE" +
                       std::string(Color::Reset);
  std::cout << PadRight(header, Layout::BoxWidth);
  std::cout << Color::Dim << "│\n";

  PrintIndent();
  std::cout << "├────────────────────────────────────────────────────────────"
               "─────┤\n"
            << Color::Reset;

  DrawBoxEmptyLine();
  DrawBoxTextLine("  This software modifies Windows Registry values,",
                  Color::White);
  DrawBoxTextLine("  system services, power plans, and file contents.",
                  Color::White);
  DrawBoxEmptyLine();
  DrawBoxTextLine("  By continuing, you acknowledge that:", Color::Gray);
  DrawBoxEmptyLine();
  DrawBoxTextLine("  1. You use this software ENTIRELY AT YOUR OWN RISK.",
                  Color::Yellow);
  DrawBoxTextLine("  2. The author is NOT responsible for any damage,",
                  Color::Gray);
  DrawBoxTextLine("     data loss, or system instability.", Color::Gray);
  DrawBoxTextLine("  3. You should create a Restore Point before", Color::Gray);
  DrawBoxTextLine("     applying any tweaks.", Color::Gray);
  DrawBoxTextLine("  4. All changes can be reverted via the built-in",
                  Color::Gray);
  DrawBoxTextLine("     backup system or Windows System Restore.", Color::Gray);
  DrawBoxEmptyLine();
  DrawBoxTextLine("  NO WARRANTY IS PROVIDED, EXPRESS OR IMPLIED.", Color::Red);
  DrawBoxEmptyLine();

  PrintIndent();
  std::cout << Color::Dim
            << "└────────────────────────────────────────────────────────────"
               "─────┘\n"
            << Color::Reset;

  std::cout << "\n";
  PrintIndent();
  std::cout << Color::Accent << Icon::Arrow << Color::Reset;
  std::cout << " Do you accept these terms? ";
  std::cout << Color::Dim << "[" << Color::Reset;
  std::cout << Color::Green << "Y" << Color::Reset;
  std::cout << Color::Dim << "/" << Color::Reset;
  std::cout << Color::Red << "N" << Color::Reset;
  std::cout << Color::Dim << "]" << Color::Reset << ": " << Color::Bold;
}

void Renderer::DrawDriftWarning(
    const std::vector<Vax::System::DriftEntry> &drifted) {
  DrawLogo();
  std::cout << "\n";

  std::cout << Color::Dim;
  PrintIndent();
  std::cout << "┌──────────────────────────────────────────────────────────────"
               "───┐\n";

  PrintIndent();
  std::cout << "│" << Color::Reset;
  std::string header = std::string(Color::Yellow) + "  " + Icon::Warning +
                       "  DRIFT DETECTED — TWEAKS REVERTED" +
                       std::string(Color::Reset);
  std::cout << PadRight(header, Layout::BoxWidth);
  std::cout << Color::Dim << "│\n";

  PrintIndent();
  std::cout << "├────────────────────────────────────────────────────────────"
               "─────┤\n"
            << Color::Reset;

  DrawBoxEmptyLine();
  DrawBoxTextLine("   The following tweaks have been reverted since your",
                  Color::White);
  DrawBoxTextLine("   last session (e.g., by Windows Update or GP refresh):",
                  Color::White);
  DrawBoxEmptyLine();

  std::string lastModule;
  for (const auto &d : drifted) {
    if (d.moduleName != lastModule) {
      DrawBoxTextLine("   " + std::string(Color::Accent) + d.moduleName +
                          std::string(Color::Reset),
                      "");
      lastModule = d.moduleName;
    }
    DrawBoxTextLine("     " + std::string(Color::Red) + Icon::Error + " " +
                        std::string(Color::Gray) + d.tweakName +
                        std::string(Color::Reset),
                    "");
  }

  DrawBoxEmptyLine();
  DrawBoxTextLine("   Re-apply these tweaks from their respective modules.",
                  Color::Dim);
  DrawBoxEmptyLine();

  PrintIndent();
  std::cout << Color::Dim
            << "└────────────────────────────────────────────────────────────"
               "─────┘\n"
            << Color::Reset;

  std::cout << "\n";
  PrintIndent();
  std::cout << Color::Dim << "Press any key to continue..." << Color::Reset;
}

void Renderer::DrawBoxEmptyLine() {
  PrintIndent();
  std::cout << Color::Dim << "│" << Color::Reset;
  std::cout << std::string(Layout::BoxWidth, ' ');
  std::cout << Color::Dim << "│\n" << Color::Reset;
}

void Renderer::DrawBoxTextLine(const std::string &text,
                               const std::string &color) {
  PrintIndent();
  std::cout << Color::Dim << "│" << Color::Reset;

  if (!color.empty()) {
    std::cout << color;
  }

  std::cout << text;

  int padding = Layout::BoxWidth - VisibleLength(text);
  if (padding > 0) {
    std::cout << std::string(padding, ' ');
  }

  std::cout << Color::Reset << Color::Dim << "│\n" << Color::Reset;
}

void Renderer::DrawTitleBox(const std::string &icon, const std::string &title,
                            const std::string &color) {
  std::cout << "\n";
  std::cout << Color::Dim;
  PrintIndent();
  std::cout << "┌──────────────────────────────────────────────────────────────"
               "───┐\n";

  PrintIndent();
  std::cout << "│" << Color::Reset;
  std::string content = "  " + std::string(icon) + "  " +
                        std::string(Color::Bold) + color + title +
                        std::string(Color::Reset);
  std::cout << PadRight(content, Layout::BoxWidth);
  std::cout << Color::Dim << "│\n";

  PrintIndent();
  std::cout << "└────────────────────────────────────────────────────────────"
               "─────┘";
  std::cout << Color::Reset << "\n";
}

void Renderer::PrintIndent() { std::cout << std::string(Layout::Indent, ' '); }

std::string Renderer::PadRight(const std::string &text, int totalWidth) {
  int visLen = VisibleLength(text);
  int padding = totalWidth - visLen;
  if (padding <= 0)
    return text;
  return text + std::string(padding, ' ');
}

int Renderer::VisibleLength(const std::string &text) {
  int len = 0;
  bool inEscape = false;
  for (size_t i = 0; i < text.size();) {
    unsigned char c = static_cast<unsigned char>(text[i]);
    if (c == '\033') {
      inEscape = true;
      ++i;
    } else if (inEscape) {
      if ((text[i] >= 'A' && text[i] <= 'Z') ||
          (text[i] >= 'a' && text[i] <= 'z')) {
        inEscape = false;
      }
      ++i;
    } else if (c >= 0xF0) {
      len += 2;
      i += 4;
    } else if (c >= 0xE0) {
      len += 1;
      i += 3;
    } else if (c >= 0xC0) {
      len += 1;
      i += 2;
    } else {
      len += 1;
      i += 1;
    }
  }
  return len;
}

}
