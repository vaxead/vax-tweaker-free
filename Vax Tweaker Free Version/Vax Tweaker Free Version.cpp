
#include "Core/Application.h"
#include <iostream>
#include <windows.h>

static HANDLE g_mutex = nullptr;

static bool AcquireSingleInstance() {
  g_mutex = CreateMutexA(nullptr, TRUE, "VaxTweakerFree_SingleInstance");
  if (g_mutex == nullptr)
    return false;
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    CloseHandle(g_mutex);
    g_mutex = nullptr;
    return false;
  }
  return true;
}

static void ReleaseSingleInstance() {
  if (g_mutex) {
    ReleaseMutex(g_mutex);
    CloseHandle(g_mutex);
    g_mutex = nullptr;
  }
}

int main() {
  if (!AcquireSingleInstance()) {
    MessageBoxA(nullptr, "VAX TWEAKER Free is already running.",
                "VAX TWEAKER Free", MB_OK | MB_ICONINFORMATION);
    return 1;
  }

  int exitCode = 0;

  try {
    Vax::Application app;
    exitCode = app.Run();
  } catch (const std::exception &e) {
    std::string msg = "A fatal error occurred:\n";
    msg += e.what();
    MessageBoxA(nullptr, msg.c_str(), "VAX TWEAKER Free — Error",
                MB_OK | MB_ICONERROR);
    exitCode = 1;
  } catch (...) {
    MessageBoxA(nullptr, "An unknown fatal error occurred.",
                "VAX TWEAKER Free — Error", MB_OK | MB_ICONERROR);
    exitCode = 1;
  }

  ReleaseSingleInstance();
  return exitCode;
}
