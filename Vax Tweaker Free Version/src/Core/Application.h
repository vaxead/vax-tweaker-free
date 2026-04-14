
#pragma once

#include "Types.h"
#include <vector>

namespace Vax {

class Application {
public:
  Application();
  ~Application();

  int Run();

private:
  void Initialize();
  void InitializeModules();
  bool HandleAdminCheck();
  bool ShowDisclaimer();
  bool HandleRestorePoint();

  void MainLoop();
  void HandleMenuChoice(int choice);
  void HandleRestoreAll();
  void ShowModule(int moduleId);

  void ShowMainScreen();
  bool ShowAdminPrompt();
  void ShowExitScreen();

  AppState m_state;
};

}
