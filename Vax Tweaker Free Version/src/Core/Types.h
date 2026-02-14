
#pragma once

#include <string>
#include <vector>
#include <windows.h>

namespace Vax {

constexpr const char *APP_VERSION = "1.0.0-FREE";
constexpr const char *APP_NAME = "VAX TWEAKER";
constexpr const char *APP_TITLE = "VAX TWEAKER v1.0.0 - Free Edition";

enum class RiskLevel {
  Safe,
  Moderate,
  Advanced,
  Risky
};

enum class TweakStatus {
  Unknown,
  Applied,
  NotApplied,
  Partial,
  Error
};

enum class ModuleCategory {
  Performance,
  Privacy,
  Network,
  System,
  Maintenance,
  Tools,
  Experimental
};

enum class RegValueType { Dword, String, Binary };

struct RegistryTarget {
  HKEY root;
  std::string subKey;
  std::string valueName;

  RegValueType valueType = RegValueType::Dword;

  DWORD applyDword = 0;
  std::string applyString = "";
  std::vector<BYTE> applyBinary = {};

  DWORD expectedDword = 0;
  std::string expectedString = "";

  DWORD defaultDword = 0;
  std::string defaultString = "";
  std::vector<BYTE> defaultBinary = {};

  bool deleteOnRevert = false;

  bool mergeStringValue = false;
};

struct TweakInfo {
  std::string id;
  std::string name;
  std::string description;
  RiskLevel risk;
  TweakStatus status;
  bool requiresReboot;
  std::vector<RegistryTarget> registryKeys;
};

struct ModuleInfo {
  int id;
  std::string name;
  std::string description;
  std::string icon;
  ModuleCategory category;
};

struct TweakGroup {
  std::string id;
  std::string name;
  std::string icon;
  std::string description;
  std::vector<std::string> tweakIds;
};

struct AppState {
  bool isAdmin = false;
  bool isRunning = true;
  int currentMenu = 0;
};

}
