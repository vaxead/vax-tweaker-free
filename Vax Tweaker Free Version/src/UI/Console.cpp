
#include "Console.h"
#include <conio.h>
#include <iostream>
#include <limits>
#include <string>

namespace Vax::UI {

CONSOLE_FONT_INFOEX Console::s_originalFont = {};
bool Console::s_fontSaved = false;

void Console::Initialize() {
  EnableUtf8();
  EnableVirtualTerminal();
  SetupFont();
  SetupWindowSize();
}

void Console::Cleanup() {
  if (s_fontSaved) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetCurrentConsoleFontEx(hConsole, FALSE, &s_originalFont);
    s_fontSaved = false;
  }
}

void Console::Clear() {
  std::cout << "\033[2J\033[3J\033[H";
  std::cout.flush();
}

void Console::SetTitle(const std::string &title) {
  SetConsoleTitleA(title.c_str());
}

void Console::WaitForKey() { _getch(); }

char Console::ReadChar() { return static_cast<char>(_getch()); }

std::string Console::ReadLine() {
  std::string line;
  std::getline(std::cin, line);
  return line;
}

int Console::ReadInt() {
  int value = -1;
  std::cin >> value;

  bool failed = std::cin.fail();
  if (failed) {
    std::cin.clear();
  }
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  return failed ? -1 : value;
}

std::string Console::GetWindowsVersionString() {
  HKEY hKey;
  LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                              0, KEY_READ, &hKey);
  if (result != ERROR_SUCCESS) {
    return "WINDOWS";
  }

  char buildStr[32] = {};
  DWORD size = sizeof(buildStr);
  result = RegQueryValueExA(hKey, "CurrentBuildNumber", nullptr, nullptr,
                            reinterpret_cast<LPBYTE>(buildStr), &size);
  RegCloseKey(hKey);

  if (result != ERROR_SUCCESS) {
    return "WINDOWS";
  }

  int build = 0;
  try {
    build = std::stoi(buildStr);
  } catch (...) {
    return "WINDOWS";
  }

  if (build >= 22000)
    return "WIN 11";
  if (build >= 10240)
    return "WIN 10";
  return "WINDOWS";
}

int Console::GetWindowsBuildNumber() {
  HKEY hKey;
  LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                              0, KEY_READ, &hKey);
  if (result != ERROR_SUCCESS)
    return 0;

  char buildStr[32] = {};
  DWORD size = sizeof(buildStr);
  result = RegQueryValueExA(hKey, "CurrentBuildNumber", nullptr, nullptr,
                            reinterpret_cast<LPBYTE>(buildStr), &size);
  RegCloseKey(hKey);

  if (result != ERROR_SUCCESS)
    return 0;
  try {
    return std::stoi(buildStr);
  } catch (...) {
    return 0;
  }
}

void Console::EnableVirtualTerminal() {
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD dwMode = 0;

  if (GetConsoleMode(hOut, &dwMode)) {
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
  }
}

void Console::EnableUtf8() {
  SetConsoleOutputCP(65001);
  SetConsoleCP(65001);
}

void Console::SetupFont() {
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

  s_originalFont = {};
  s_originalFont.cbSize = sizeof(s_originalFont);
  if (GetCurrentConsoleFontEx(hConsole, FALSE, &s_originalFont)) {
    s_fontSaved = true;
  }

  CONSOLE_FONT_INFOEX cfi = {};
  cfi.cbSize = sizeof(cfi);
  GetCurrentConsoleFontEx(hConsole, FALSE, &cfi);

  cfi.dwFontSize.Y = 18;
  wcscpy_s(cfi.FaceName, L"Consolas");

  SetCurrentConsoleFontEx(hConsole, FALSE, &cfi);
}

void Console::SetupWindowSize() {
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

  SMALL_RECT minWindow = {0, 0, 0, 0};
  SetConsoleWindowInfo(hConsole, TRUE, &minWindow);

  COORD bufferSize = {120, 500};
  SetConsoleScreenBufferSize(hConsole, bufferSize);

  SMALL_RECT windowSize = {0, 0, 119, 34};
  SetConsoleWindowInfo(hConsole, TRUE, &windowSize);
}

}
