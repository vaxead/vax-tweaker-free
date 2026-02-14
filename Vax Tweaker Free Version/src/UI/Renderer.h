
#pragma once

#include "../Core/Types.h"
#include "../System/DriftDetector.h"
#include <string>
#include <vector>

namespace Vax::UI {

class Renderer {
public:

  static void DrawLogo();

  static void DrawStatusBar(bool isAdmin);

  static void DrawMainMenu(const std::vector<ModuleInfo> &modules);

  static void DrawFooter();

  static void DrawPrompt();

  static void DrawModuleView(const ModuleInfo &module);

  static void DrawBreadcrumbs(const ModuleInfo &module,
                              const std::string &subSection = "");

  static void DrawTweakList(const ModuleInfo &module,
                            const std::vector<TweakInfo> &tweaks,
                            bool showStatus = true);

  static void DrawGroupList(const ModuleInfo &module,
                            const std::vector<TweakGroup> &groups,
                            const std::vector<TweakInfo> &allTweaks, int page,
                            int totalPages, bool showStatus);

  static void DrawGroupTweakList(const ModuleInfo &module,
                                 const TweakGroup &group,
                                 const std::vector<TweakInfo> &tweaks,
                                 int groupIndex, int totalGroups, int page,
                                 int totalPages, bool showStatus);

  static void DrawTweakResult(const std::string &tweakName, bool success,
                              bool isRevert = false,
                              const std::string &failReason = "");

  static void DrawDevelopmentNotice(const ModuleInfo &module);

  static void DrawAdminPrompt();

  static void DrawProgressHeader(const std::string &moduleName,
                                 const std::string &operation);

  static void DrawProgressFooter(int succeeded, int failed);

  static void DrawRestorePointPrompt();

  static void DrawRestoreAllPrompt(int backupCount);

  static void DrawExitScreen();

  static void DrawDisclaimer();

  static void
  DrawDriftWarning(const std::vector<Vax::System::DriftEntry> &drifted);

  static void DrawTitleBox(const std::string &icon, const std::string &title,
                           const std::string &color);

  static void DrawBoxEmptyLine();

  static void DrawBoxTextLine(const std::string &text,
                              const std::string &color = "");

  static void PrintIndent();

  static std::string PadRight(const std::string &text, int totalWidth);

  static int VisibleLength(const std::string &text);

private:
  Renderer() = default;
};

}
