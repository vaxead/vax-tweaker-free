
#pragma once

#include "../System/DriftDetector.h"
#include "Types.h"
#include <atomic>
#include <mutex>
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
  void RunDriftCheck();

  void MainLoop();
  void HandleMenuChoice(int choice);
  void HandleRestoreAll();
  void ShowModule(int moduleId);

  void ShowMainScreen();
  bool ShowAdminPrompt();
  void ShowExitScreen();

  AppState m_state;

  std::atomic<bool> m_driftCheckDone{false};
  std::vector<System::DriftEntry> m_driftResults;
  std::mutex m_driftMutex;
};

}
