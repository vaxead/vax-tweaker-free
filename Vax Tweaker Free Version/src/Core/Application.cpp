
#include "Application.h"
#include "../Modules/ModuleRegistry.h"
#include "../Safety/SafetyGuard.h"
#include "../System/DriftDetector.h"
#include "../System/Logger.h"
#include "../System/Registry.h"
#include "../System/RestorePoint.h"
#include "../UI/Console.h"
#include "../UI/Renderer.h"
#include "../UI/Theme.h"
#include "Admin.h"
#include "SystemProfile.h"
#include "Types.h"
#include <iostream>
#include <thread>

namespace Vax {

Application::Application() {
  m_state.isAdmin = false;
  m_state.isRunning = true;
  m_state.currentMenu = 0;
}

Application::~Application() { UI::Console::Cleanup(); }

int Application::Run() {
  Initialize();

  if (!ShowDisclaimer()) {
    return 0;
  }

  if (!HandleAdminCheck()) {
    return 0;
  }

  HandleRestorePoint();

  std::thread([this]() { RunDriftCheck(); }).detach();

  MainLoop();

  System::DriftDetector::SaveSnapshot(
      Modules::ModuleRegistry::Instance().GetAll());

  ShowExitScreen();
  return 0;
}

void Application::Initialize() {
  UI::Console::Initialize();
  UI::Console::SetTitle(APP_TITLE);

  m_state.isAdmin = Admin::IsProcessElevated();
  SystemProfile::GetCurrent().RunScan(m_state.isAdmin);
  System::Registry::LoadFromDisk();
  InitializeModules();
}

void Application::InitializeModules() {
  Modules::ModuleRegistry::Instance().InitializeDefaults();
}

void Application::RunDriftCheck() {
  auto &registry = Modules::ModuleRegistry::Instance();
  for (const auto &mod : registry.GetAll()) {
    if (mod && mod->IsImplemented()) {
      mod->RefreshStatus();
    }
  }

  auto drifted = System::DriftDetector::DetectDrift(registry.GetAll());

  {
    std::lock_guard<std::mutex> lock(m_driftMutex);
    m_driftResults = std::move(drifted);
  }
  m_driftCheckDone.store(true);
}

bool Application::ShowDisclaimer() {
  std::string flagPath = System::Registry::GetAppDataDir() + "\\accepted.dat";

  if (GetFileAttributesA(flagPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
    return true;
  }

  UI::Console::Clear();
  UI::Renderer::DrawDisclaimer();

  char choice;
  while (true) {
    choice = UI::Console::ReadChar();
    if (choice == 'Y' || choice == 'y' || choice == 'N' || choice == 'n')
      break;
  }
  std::cout << UI::Color::Reset << "\n";

  if (choice != 'Y' && choice != 'y') {
    System::Logger::Info("User declined disclaimer — exiting");
    return false;
  }

  HANDLE hFile = CreateFileA(flagPath.c_str(), GENERIC_WRITE, 0, nullptr,
                             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile != INVALID_HANDLE_VALUE) {
    const char stamp[] = "ACCEPTED\n";
    DWORD written = 0;
    WriteFile(hFile, stamp, sizeof(stamp) - 1, &written, nullptr);
    CloseHandle(hFile);
  }

  System::Logger::Info("User accepted disclaimer");
  return true;
}

bool Application::HandleAdminCheck() {
  if (m_state.isAdmin) {
    return true;
  }

  if (ShowAdminPrompt()) {
    if (Admin::RequestElevation()) {
      m_state.isRunning = false;
      return false;
    }
  }

  return true;
}

bool Application::HandleRestorePoint() {
  if (!m_state.isAdmin) {
    return false;
  }

  UI::Console::Clear();
  UI::Renderer::DrawRestorePointPrompt();

  char choice;
  while (true) {
    choice = UI::Console::ReadChar();
    if (choice == 'Y' || choice == 'y' || choice == 'N' || choice == 'n')
      break;
  }
  if (choice == 'Y' || choice == 'y') {
    UI::Console::Clear();
    UI::Renderer::DrawProgressHeader("System", "CREATING RESTORE POINT");

    bool result = System::RestorePoint::Create("VaxTweaker Restore Point");
    UI::Renderer::DrawTweakResult("System Restore Point", result);

    if (!result) {
      std::cout << "\n";
      UI::Renderer::PrintIndent();
      std::cout << UI::Color::Red << UI::Icon::Warning
                << "  Restore point was NOT created. Proceed with caution."
                << UI::Color::Reset << "\n";
    }

    UI::Renderer::DrawProgressFooter(result ? 1 : 0, result ? 0 : 1);
    UI::Console::WaitForKey();
    return result;
  }
  return false;
}

void Application::MainLoop() {
  while (m_state.isRunning) {
    if (m_driftCheckDone.exchange(false)) {
      std::lock_guard<std::mutex> lock(m_driftMutex);
      if (!m_driftResults.empty()) {
        UI::Console::Clear();
        UI::Renderer::DrawDriftWarning(m_driftResults);
        UI::Console::WaitForKey();
        m_driftResults.clear();
      }
    }

    ShowMainScreen();

    std::string input = UI::Console::ReadLine();
    std::cout << UI::Color::Reset;

    if (input == "R" || input == "r") {
      HandleRestoreAll();
    } else {
      int choice = -1;
      try {
        int parsed = std::stoi(input);
        if (parsed >= 0 && parsed <= 999)
          choice = parsed;
      } catch (...) {
      }
      HandleMenuChoice(choice);
    }
  }
}

void Application::HandleMenuChoice(int choice) {
  if (choice == 0) {
    m_state.isRunning = false;
    return;
  }

  auto &registry = Modules::ModuleRegistry::Instance();
  if (choice >= 1 && choice <= static_cast<int>(registry.Count())) {
    ShowModule(choice);
  }
}

void Application::HandleRestoreAll() {
  size_t backupCount = System::Registry::GetBackupEntries().size();
  if (backupCount == 0) {
    return;
  }

  UI::Console::Clear();
  UI::Renderer::DrawRestoreAllPrompt(static_cast<int>(backupCount));

  char choice;
  while (true) {
    choice = UI::Console::ReadChar();
    if (choice == 'Y' || choice == 'y' || choice == 'N' || choice == 'n')
      break;
  }
  if (choice != 'Y' && choice != 'y') {
    return;
  }

  UI::Console::Clear();
  UI::Renderer::DrawProgressHeader("System", "RESTORING ALL CHANGES");

  const auto backups = System::Registry::GetBackupEntries();
  int succeeded = 0, failed = 0;
  std::vector<System::RegistryBackupEntry> failedEntries;

  for (auto it = backups.rbegin(); it != backups.rend(); ++it) {
    std::string entryName = it->subKey + "\\" + it->valueName;
    bool result = System::Registry::RestoreEntry(*it);
    UI::Renderer::DrawTweakResult(entryName, result, true);
    if (result) {
      ++succeeded;
    } else {
      ++failed;
      failedEntries.push_back(*it);
    }
  }

  if (failed == 0) {
    System::Registry::ClearBackups();
    System::Logger::Success("All registry changes restored successfully");
  } else {
    System::Registry::ReplaceBackups(std::move(failedEntries));
    System::Logger::Error("Some entries failed to restore (" +
                          std::to_string(failed) + " failures)");
  }

  UI::Renderer::DrawProgressFooter(succeeded, failed);
  UI::Console::WaitForKey();
}

void Application::ShowModule(int moduleId) {
  auto *module = Modules::ModuleRegistry::Instance().GetById(moduleId);

  if (module == nullptr) {
    return;
  }

  if (!module->IsImplemented()) {
    UI::Console::Clear();
    UI::Renderer::DrawDevelopmentNotice(module->GetInfo());
    UI::Console::WaitForKey();
    return;
  }

  if (module->RequiresAdmin() && !m_state.isAdmin) {
    UI::Console::Clear();
    UI::Renderer::DrawLogo();
    std::cout << "\n";
    UI::Renderer::DrawTitleBox(UI::Icon::Warning,
                               "ADMIN REQUIRED \xe2\x80\x94 " +
                                   module->GetInfo().name,
                               UI::Color::Yellow);
    std::cout << "\n";
    UI::Renderer::PrintIndent();
    std::cout << UI::Color::Gray
              << "  Some tweaks in this module write to HKEY_LOCAL_MACHINE\n";
    UI::Renderer::PrintIndent();
    std::cout << "  and will fail without Administrator privileges.\n"
              << UI::Color::Reset;
    std::cout << "\n";
    UI::Renderer::PrintIndent();
    std::cout << UI::Color::Accent << UI::Icon::Arrow << UI::Color::Reset
              << " Continue anyway? " << UI::Color::Dim << "["
              << UI::Color::Reset << UI::Color::Green << "Y" << UI::Color::Reset
              << UI::Color::Dim << "/" << UI::Color::Reset << UI::Color::Red
              << "N" << UI::Color::Reset << UI::Color::Dim << "]"
              << UI::Color::Reset << ": " << UI::Color::Bold;
    char choice;
    while (true) {
      choice = UI::Console::ReadChar();
      if (choice == 'Y' || choice == 'y' || choice == 'N' || choice == 'n')
        break;
    }
    std::cout << UI::Color::Reset;
    if (choice != 'Y' && choice != 'y') {
      return;
    }
  }

  module->Show();
}

void Application::ShowMainScreen() {
  UI::Console::Clear();

  UI::Renderer::DrawLogo();
  UI::Renderer::DrawStatusBar(m_state.isAdmin);

  auto modules = Modules::ModuleRegistry::Instance().GetModuleInfoList();
  UI::Renderer::DrawMainMenu(modules);

  UI::Renderer::DrawFooter();
  UI::Renderer::DrawPrompt();
}

bool Application::ShowAdminPrompt() {
  UI::Console::Clear();
  UI::Renderer::DrawAdminPrompt();

  char choice;
  while (true) {
    choice = UI::Console::ReadChar();
    if (choice == 'Y' || choice == 'y' || choice == 'N' || choice == 'n')
      break;
  }
  return (choice == 'Y' || choice == 'y');
}

void Application::ShowExitScreen() {
  std::string logPath = System::Registry::GetBackupFilePath();
  size_t pos = logPath.find_last_of("\\/");
  if (pos != std::string::npos) {
    logPath = logPath.substr(0, pos + 1) + "vax_session.log";
  } else {
    logPath = "vax_session.log";
  }
  System::Logger::ExportToFile(logPath);

  UI::Console::Clear();
  UI::Renderer::DrawExitScreen();
}

}
