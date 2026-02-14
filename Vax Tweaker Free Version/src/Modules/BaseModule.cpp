
#include "BaseModule.h"
#include "../Core/Admin.h"
#include "../Core/Compatibility.h"
#include "../Safety/SafetyGuard.h"
#include "../System/Logger.h"
#include "../System/Registry.h"
#include "../UI/Console.h"
#include "../UI/Renderer.h"
#include "../UI/Theme.h"
#include <algorithm>
#include <iostream>
#include <sstream>

namespace Vax::Modules {

BaseModule::BaseModule(int id, const std::string &name,
                       const std::string &description, const std::string &icon,
                       ModuleCategory category) {
  m_info.id = id;
  m_info.name = name;
  m_info.description = description;
  m_info.icon = icon;
  m_info.category = category;
}

ModuleInfo BaseModule::GetInfo() const { return m_info; }

std::vector<TweakInfo> BaseModule::GetTweaks() const { return m_tweaks; }

std::vector<TweakGroup> BaseModule::GetGroups() const { return m_groups; }

void BaseModule::Show() {
  bool showStatus = m_showTweakStatus;

  if (m_groups.empty()) {
    ShowFlatList();
    return;
  }

  int groupPage = 0;
  bool inModule = true;
  while (inModule) {
    if (m_statusDirty) {
      RefreshStatus();
      m_statusDirty = false;
    }
    int totalGroups = static_cast<int>(m_groups.size());
    int groupPages =
        (totalGroups + UI::Layout::PageSize - 1) / UI::Layout::PageSize;

    UI::Console::Clear();
    UI::Renderer::DrawGroupList(m_info, m_groups, m_tweaks, groupPage,
                                groupPages, showStatus);

    std::string input = UI::Console::ReadLine();
    std::cout << Vax::UI::Color::Reset;

    if (input == "0") {
      inModule = false;
    } else if (input == "A" || input == "a") {
      if (Safety::SafetyGuard::ConfirmApplyAll(
              m_info.name, static_cast<int>(m_tweaks.size()))) {
        UI::Console::Clear();
        UI::Renderer::DrawProgressHeader(m_info.name, "APPLYING ALL TWEAKS");
        std::string batchWarn = Compatibility::GetBatchWarning(m_tweaks);
        if (!batchWarn.empty()) {
          UI::Renderer::PrintIndent();
          std::cout << UI::Color::Yellow << UI::Icon::Warning << " "
                    << batchWarn << UI::Color::Reset << "\n\n";
        }
        int succeeded = 0, failed = 0;
        for (const auto &tweak : m_tweaks) {
          bool result = ApplyTweak(tweak.id);
          UI::Renderer::DrawTweakResult(tweak.name, result, false,
                                        m_lastFailReason);
          if (result)
            ++succeeded;
          else
            ++failed;
        }
        bool needsReboot = false;
        for (const auto &tweak : m_tweaks) {
          if (tweak.requiresReboot) {
            needsReboot = true;
            break;
          }
        }
        if (needsReboot)
          Safety::SafetyGuard::ShowRebootNotice();
        UI::Renderer::DrawProgressFooter(succeeded, failed);
        UI::Console::WaitForKey();
        m_statusDirty = true;
      }
    } else if (showStatus && (input == "R" || input == "r")) {
      if (Safety::SafetyGuard::ConfirmApplyAll(
              m_info.name + " (Revert)", static_cast<int>(m_tweaks.size()))) {
        UI::Console::Clear();
        UI::Renderer::DrawProgressHeader(m_info.name, "REVERTING ALL TWEAKS");
        int succeeded = 0, failed = 0;
        for (const auto &tweak : m_tweaks) {
          bool result = RevertTweak(tweak.id);
          UI::Renderer::DrawTweakResult(tweak.name, result, true,
                                        m_lastFailReason);
          if (result)
            ++succeeded;
          else
            ++failed;
        }
        UI::Renderer::DrawProgressFooter(succeeded, failed);
        UI::Console::WaitForKey();
        m_statusDirty = true;
      }
    } else if ((input == "Z" || input == "z") && groupPage > 0) {
      --groupPage;
    } else if ((input == "X" || input == "x") && groupPage < groupPages - 1) {
      ++groupPage;
    } else {
      int choice = -1;
      try {
        choice = std::stoi(input);
      } catch (...) {
      }
      if (choice >= 1 && choice <= totalGroups) {
        ShowGroupTweaks(choice - 1);
      }
    }
  }
}

void BaseModule::ShowFlatList() {
  bool allowRevertAll = m_showTweakStatus;
  bool inModule = true;
  while (inModule) {
    if (m_statusDirty) {
      RefreshStatus();
      m_statusDirty = false;
    }
    UI::Console::Clear();
    UI::Renderer::DrawTweakList(m_info, m_tweaks, m_showTweakStatus);

    std::string input = UI::Console::ReadLine();
    std::cout << Vax::UI::Color::Reset;

    if (input == "0") {
      inModule = false;
    } else if (input == "A" || input == "a") {
      if (Safety::SafetyGuard::ConfirmApplyAll(
              m_info.name, static_cast<int>(m_tweaks.size()))) {
        UI::Console::Clear();
        UI::Renderer::DrawProgressHeader(m_info.name, "APPLYING TWEAKS");
        std::string batchWarn = Compatibility::GetBatchWarning(m_tweaks);
        if (!batchWarn.empty()) {
          UI::Renderer::PrintIndent();
          std::cout << UI::Color::Yellow << UI::Icon::Warning << " "
                    << batchWarn << UI::Color::Reset << "\n\n";
        }
        int succeeded = 0, failed = 0;
        for (const auto &tweak : m_tweaks) {
          bool result = ApplyTweak(tweak.id);
          UI::Renderer::DrawTweakResult(tweak.name, result, false,
                                        m_lastFailReason);
          if (result)
            ++succeeded;
          else
            ++failed;
        }
        bool needsReboot = false;
        for (const auto &tweak : m_tweaks) {
          if (tweak.requiresReboot) {
            needsReboot = true;
            break;
          }
        }
        if (needsReboot)
          Safety::SafetyGuard::ShowRebootNotice();
        UI::Renderer::DrawProgressFooter(succeeded, failed);
        UI::Console::WaitForKey();
        m_statusDirty = true;
      }
    } else if (allowRevertAll && (input == "R" || input == "r")) {
      if (Safety::SafetyGuard::ConfirmApplyAll(
              m_info.name + " (Revert)", static_cast<int>(m_tweaks.size()))) {
        UI::Console::Clear();
        UI::Renderer::DrawProgressHeader(m_info.name, "REVERTING TWEAKS");
        int succeeded = 0, failed = 0;
        for (const auto &tweak : m_tweaks) {
          bool result = RevertTweak(tweak.id);
          UI::Renderer::DrawTweakResult(tweak.name, result, true,
                                        m_lastFailReason);
          if (result)
            ++succeeded;
          else
            ++failed;
        }
        UI::Renderer::DrawProgressFooter(succeeded, failed);
        UI::Console::WaitForKey();
        m_statusDirty = true;
      }
    } else {
      int choice = -1;
      try {
        choice = std::stoi(input);
      } catch (...) {
      }
      if (choice >= 1 && choice <= static_cast<int>(m_tweaks.size())) {
        auto &tweak = m_tweaks[choice - 1];
        if (tweak.registryKeys.empty() &&
            tweak.status == TweakStatus::Unknown) {
          ApplyTweak(tweak.id);
          m_statusDirty = true;
        } else if (tweak.status == TweakStatus::Applied) {
          if (Safety::SafetyGuard::ConfirmRevert(tweak)) {
            UI::Console::Clear();
            UI::Renderer::DrawProgressHeader(m_info.name, "REVERTING TWEAK");
            bool result = RevertTweak(tweak.id);
            UI::Renderer::DrawTweakResult(tweak.name, result, true,
                                          m_lastFailReason);
            UI::Renderer::DrawProgressFooter(result ? 1 : 0, result ? 0 : 1);
            UI::Console::WaitForKey();
            m_statusDirty = true;
          }
        } else {
          if (Safety::SafetyGuard::ConfirmTweak(tweak)) {
            UI::Console::Clear();
            UI::Renderer::DrawProgressHeader(m_info.name, "APPLYING TWEAK");
            std::string warn = Compatibility::GetWarning(tweak.id);
            if (!warn.empty()) {
              UI::Renderer::PrintIndent();
              std::cout << UI::Color::Yellow << UI::Icon::Warning << " " << warn
                        << UI::Color::Reset << "\n\n";
            }
            bool result = ApplyTweak(tweak.id);
            UI::Renderer::DrawTweakResult(tweak.name, result, false,
                                          m_lastFailReason);
            if (tweak.requiresReboot && result)
              Safety::SafetyGuard::ShowRebootNotice();
            UI::Renderer::DrawProgressFooter(result ? 1 : 0, result ? 0 : 1);
            UI::Console::WaitForKey();
            m_statusDirty = true;
          }
        }
      }
    }
  }
}

void BaseModule::ShowGroupTweaks(int groupIndex) {
  bool showStatus = m_showTweakStatus;
  int totalGroups = static_cast<int>(m_groups.size());
  int tweakPage = 0;
  bool inGroup = true;

  while (inGroup) {
    if (m_statusDirty) {
      RefreshStatus();
      m_statusDirty = false;
    }
    const auto &group = m_groups[groupIndex];

    std::vector<TweakInfo> groupTweaks;
    for (const auto &tid : group.tweakIds) {
      TweakInfo *t = FindTweak(tid);
      if (t)
        groupTweaks.push_back(*t);
    }

    int tweakPages = (std::max)(1, (static_cast<int>(groupTweaks.size()) +
                                    UI::Layout::PageSize - 1) /
                                       UI::Layout::PageSize);

    UI::Console::Clear();
    UI::Renderer::DrawGroupTweakList(m_info, group, groupTweaks, groupIndex,
                                     totalGroups, tweakPage, tweakPages,
                                     showStatus);

    std::string input = UI::Console::ReadLine();
    std::cout << Vax::UI::Color::Reset;

    if (input == "0") {
      inGroup = false;
    } else if (input == "N" || input == "n") {
      if (groupIndex < totalGroups - 1) {
        ++groupIndex;
        tweakPage = 0;
      }
    } else if (input == "P" || input == "p") {
      if (groupIndex > 0) {
        --groupIndex;
        tweakPage = 0;
      }
    } else if ((input == "Z" || input == "z") && tweakPage > 0) {
      --tweakPage;
    } else if ((input == "X" || input == "x") && tweakPage < tweakPages - 1) {
      ++tweakPage;
    } else if (input == "A" || input == "a") {
      if (Safety::SafetyGuard::ConfirmApplyAll(
              group.name, static_cast<int>(groupTweaks.size()))) {
        UI::Console::Clear();
        UI::Renderer::DrawProgressHeader(m_info.name, "APPLYING TWEAKS");
        std::string batchWarn = Compatibility::GetBatchWarning(groupTweaks);
        if (!batchWarn.empty()) {
          UI::Renderer::PrintIndent();
          std::cout << UI::Color::Yellow << UI::Icon::Warning << " "
                    << batchWarn << UI::Color::Reset << "\n\n";
        }
        int succeeded = 0, failed = 0;
        for (const auto &tweak : groupTweaks) {
          bool result = ApplyTweak(tweak.id);
          UI::Renderer::DrawTweakResult(tweak.name, result, false,
                                        m_lastFailReason);
          if (result)
            ++succeeded;
          else
            ++failed;
        }
        bool needsReboot = false;
        for (const auto &tweak : groupTweaks) {
          if (tweak.requiresReboot) {
            needsReboot = true;
            break;
          }
        }
        if (needsReboot)
          Safety::SafetyGuard::ShowRebootNotice();
        UI::Renderer::DrawProgressFooter(succeeded, failed);
        UI::Console::WaitForKey();
        m_statusDirty = true;
      }
    } else if (showStatus && (input == "R" || input == "r")) {
      if (Safety::SafetyGuard::ConfirmApplyAll(
              group.name + " (Revert)", static_cast<int>(groupTweaks.size()))) {
        UI::Console::Clear();
        UI::Renderer::DrawProgressHeader(m_info.name, "REVERTING TWEAKS");
        int succeeded = 0, failed = 0;
        for (const auto &tweak : groupTweaks) {
          bool result = RevertTweak(tweak.id);
          UI::Renderer::DrawTweakResult(tweak.name, result, true,
                                        m_lastFailReason);
          if (result)
            ++succeeded;
          else
            ++failed;
        }
        UI::Renderer::DrawProgressFooter(succeeded, failed);
        UI::Console::WaitForKey();
        m_statusDirty = true;
      }
    } else {
      int choice = -1;
      try {
        choice = std::stoi(input);
      } catch (...) {
      }
      if (choice >= 1 && choice <= static_cast<int>(groupTweaks.size())) {
        const std::string &tid = groupTweaks[choice - 1].id;
        TweakInfo *tweak = FindTweak(tid);
        if (tweak) {
          if (tweak->registryKeys.empty() &&
              tweak->status == TweakStatus::Unknown) {
            if (Safety::SafetyGuard::ConfirmTweak(*tweak)) {
              UI::Console::Clear();
              UI::Renderer::DrawProgressHeader(m_info.name, "APPLYING TWEAK");
              bool result = ApplyTweak(tweak->id);
              UI::Renderer::DrawTweakResult(tweak->name, result, false,
                                            m_lastFailReason);
              if (tweak->requiresReboot && result)
                Safety::SafetyGuard::ShowRebootNotice();
              UI::Renderer::DrawProgressFooter(result ? 1 : 0, result ? 0 : 1);
              UI::Console::WaitForKey();
            }
            m_statusDirty = true;
          } else if (tweak->status == TweakStatus::Applied) {
            if (Safety::SafetyGuard::ConfirmRevert(*tweak)) {
              UI::Console::Clear();
              UI::Renderer::DrawProgressHeader(m_info.name, "REVERTING TWEAK");
              bool result = RevertTweak(tweak->id);
              UI::Renderer::DrawTweakResult(tweak->name, result, true,
                                            m_lastFailReason);
              UI::Renderer::DrawProgressFooter(result ? 1 : 0, result ? 0 : 1);
              UI::Console::WaitForKey();
              m_statusDirty = true;
            }
          } else {
            if (Safety::SafetyGuard::ConfirmTweak(*tweak)) {
              UI::Console::Clear();
              UI::Renderer::DrawProgressHeader(m_info.name, "APPLYING TWEAK");
              std::string warn = Compatibility::GetWarning(tweak->id);
              if (!warn.empty()) {
                UI::Renderer::PrintIndent();
                std::cout << UI::Color::Yellow << UI::Icon::Warning << " "
                          << warn << UI::Color::Reset << "\n\n";
              }
              bool result = ApplyTweak(tweak->id);
              UI::Renderer::DrawTweakResult(tweak->name, result, false,
                                            m_lastFailReason);
              if (tweak->requiresReboot && result)
                Safety::SafetyGuard::ShowRebootNotice();
              UI::Renderer::DrawProgressFooter(result ? 1 : 0, result ? 0 : 1);
              UI::Console::WaitForKey();
              m_statusDirty = true;
            }
          }
        }
      }
    }
  }
}

void BaseModule::Hide() {
}

bool BaseModule::IsTargetApplied(const RegistryTarget &target) const {
  using namespace System;

  switch (target.valueType) {
  case RegValueType::Dword: {
    auto val =
        Registry::ReadDword(target.root, target.subKey, target.valueName);
    return val.has_value() && val.value() == target.expectedDword;
  }
  case RegValueType::String: {
    auto val =
        Registry::ReadString(target.root, target.subKey, target.valueName);
    if (!val.has_value())
      return false;
    if (!target.expectedString.empty()) {
      if (target.expectedString.find('=') != std::string::npos) {
        return val.value().find(target.expectedString) != std::string::npos;
      }
      return val.value() == target.expectedString;
    }
    return val.value() == target.applyString;
  }
  case RegValueType::Binary: {
    auto val =
        Registry::ReadBinary(target.root, target.subKey, target.valueName);
    return val.has_value() && val.value() == target.applyBinary;
  }
  }
  return false;
}

void BaseModule::RefreshStatus() {
  for (auto &tweak : m_tweaks) {
    if (tweak.registryKeys.empty()) {
      tweak.status = TweakStatus::Unknown;
      continue;
    }

    int applied = 0;
    int total = static_cast<int>(tweak.registryKeys.size());

    for (const auto &target : tweak.registryKeys) {
      if (IsTargetApplied(target)) {
        ++applied;
      }
    }

    if (applied == total) {
      tweak.status = TweakStatus::Applied;
    } else if (applied > 0) {
      tweak.status = TweakStatus::Partial;
    } else {
      tweak.status = TweakStatus::NotApplied;
    }
  }
}

bool BaseModule::ApplyTweak(const std::string &tweakId) {
  m_lastFailReason.clear();

  TweakInfo *tweak = FindTweak(tweakId);
  if (!tweak || tweak->registryKeys.empty()) {
    System::Logger::Error("ApplyTweak: no registry keys found for tweak " +
                          tweakId);
    return false;
  }

  if (!Admin::IsElevated()) {
    for (const auto &target : tweak->registryKeys) {
      if (target.root == HKEY_LOCAL_MACHINE) {
        m_lastFailReason = "Requires Administrator privileges. Restart as "
                           "Admin to apply this tweak.";
        System::Logger::Warning("ApplyTweak: skipped " + tweakId +
                                " (not elevated, HKLM target)");
        return false;
      }
    }
  }

  bool allSuccess = true;
  bool hklmFailed = false;
  for (const auto &target : tweak->registryKeys) {
    bool ok = false;
    switch (target.valueType) {
    case RegValueType::Dword:
      ok = System::Registry::WriteDword(target.root, target.subKey,
                                        target.valueName, target.applyDword);
      break;
    case RegValueType::String:
      if (target.mergeStringValue) {
        auto existing = System::Registry::ReadString(target.root, target.subKey,
                                                     target.valueName);
        std::string newValue;
        if (existing.has_value() && !existing.value().empty()) {
          std::string result;
          std::istringstream stream(existing.value());
          std::string token;
          bool found = false;
          std::string applyKey =
              target.applyString.substr(0, target.applyString.find('='));
          while (std::getline(stream, token, ';')) {
            if (token.empty())
              continue;
            std::string tokenKey = token.substr(0, token.find('='));
            if (tokenKey == applyKey) {
              if (!result.empty())
                result += ";";
              result += target.applyString;
              found = true;
            } else {
              if (!result.empty())
                result += ";";
              result += token;
            }
          }
          if (!found) {
            if (!result.empty())
              result += ";";
            result += target.applyString;
          }
          newValue = result;
        } else {
          newValue = target.applyString;
        }
        ok = System::Registry::WriteString(target.root, target.subKey,
                                           target.valueName, newValue);
      } else {
        ok = System::Registry::WriteString(
            target.root, target.subKey, target.valueName, target.applyString);
      }
      break;
    case RegValueType::Binary:
      ok = System::Registry::WriteBinary(target.root, target.subKey,
                                         target.valueName, target.applyBinary);
      break;
    }
    if (!ok) {
      allSuccess = false;
      if (target.root == HKEY_LOCAL_MACHINE && !Admin::IsElevated()) {
        hklmFailed = true;
        System::Logger::Warning("Write to HKLM failed \xe2\x80\x94 likely "
                                "requires Administrator privileges");
      }
    }
  }

  System::Registry::PersistToDisk();

  if (allSuccess) {
    System::Logger::Success("Applied: " + tweak->name);
  } else {
    if (hklmFailed) {
      m_lastFailReason = "Requires Administrator privileges. Restart as Admin "
                         "to apply this tweak.";
    } else if (m_lastFailReason.empty()) {
      m_lastFailReason = "Registry write failed. The key may be protected by "
                         "Windows or Group Policy.";
    }
    System::Logger::Error("Failed: " + tweak->name);
  }

  return allSuccess;
}

bool BaseModule::RevertTweak(const std::string &tweakId) {
  m_lastFailReason.clear();

  TweakInfo *tweak = FindTweak(tweakId);
  if (!tweak || tweak->registryKeys.empty()) {
    System::Logger::Error("RevertTweak: no registry keys found for tweak " +
                          tweakId);
    return false;
  }

  if (!Admin::IsElevated()) {
    for (const auto &target : tweak->registryKeys) {
      if (target.root == HKEY_LOCAL_MACHINE) {
        m_lastFailReason = "Requires Administrator privileges. Restart as "
                           "Admin to revert this tweak.";
        System::Logger::Warning("RevertTweak: skipped " + tweakId +
                                " (not elevated, HKLM target)");
        return false;
      }
    }
  }

  const auto &backups = System::Registry::GetBackupEntries();
  bool allSuccess = true;

  for (const auto &target : tweak->registryKeys) {
    if (target.mergeStringValue && target.valueType == RegValueType::String) {
      auto existing = System::Registry::ReadString(target.root, target.subKey,
                                                   target.valueName);
      if (existing.has_value() && !existing.value().empty()) {
        std::string result;
        std::istringstream stream(existing.value());
        std::string token;
        std::string applyKey =
            target.applyString.substr(0, target.applyString.find('='));
        while (std::getline(stream, token, ';')) {
          if (token.empty())
            continue;
          std::string tokenKey = token.substr(0, token.find('='));
          if (tokenKey == applyKey)
            continue;
          if (!result.empty())
            result += ";";
          result += token;
        }
        bool ok = false;
        if (result.empty()) {
          ok = System::Registry::DeleteValueNoBackup(target.root, target.subKey,
                                                     target.valueName);
        } else {
          ok = System::Registry::WriteStringNoBackup(target.root, target.subKey,
                                                     target.valueName, result);
        }
        if (!ok)
          allSuccess = false;
      }
      continue;
    }

    bool foundBackup = false;

    for (auto it = backups.rbegin(); it != backups.rend(); ++it) {
      if (it->rootKey == target.root && it->subKey == target.subKey &&
          it->valueName == target.valueName) {
        foundBackup = true;
        if (!System::Registry::RestoreEntry(*it)) {
          allSuccess = false;
        }
        break;
      }
    }

    if (!foundBackup) {
      bool ok = false;
      if (target.deleteOnRevert) {
        ok = System::Registry::DeleteValueNoBackup(target.root, target.subKey,
                                                   target.valueName);
      } else {
        switch (target.valueType) {
        case RegValueType::Dword:
          ok = System::Registry::WriteDwordNoBackup(target.root, target.subKey,
                                                    target.valueName,
                                                    target.defaultDword);
          break;
        case RegValueType::String:
          ok = System::Registry::WriteStringNoBackup(target.root, target.subKey,
                                                     target.valueName,
                                                     target.defaultString);
          break;
        case RegValueType::Binary:
          ok = System::Registry::WriteBinaryNoBackup(target.root, target.subKey,
                                                     target.valueName,
                                                     target.defaultBinary);
          break;
        }
      }
      if (!ok)
        allSuccess = false;
      System::Logger::Info("RevertTweak: used default fallback for " +
                           target.subKey + "\\" + target.valueName);
    }
  }

  System::Registry::PersistToDisk();

  if (allSuccess) {
    System::Logger::Success("Reverted tweak: " + tweakId);
  } else {
    if (m_requiresAdmin && !Admin::IsElevated()) {
      m_lastFailReason = "Requires Administrator privileges. Restart as Admin "
                         "to revert this tweak.";
    }
    System::Logger::Error("Partial revert failure for tweak: " + tweakId);
  }
  return allSuccess;
}

bool BaseModule::RequiresAdmin() const { return m_requiresAdmin; }

bool BaseModule::IsImplemented() const { return m_isImplemented; }

void BaseModule::RegisterTweak(const TweakInfo &tweak) {
  m_tweakIndex[tweak.id] = m_tweaks.size();
  m_tweaks.push_back(tweak);
}

void BaseModule::RegisterGroup(const TweakGroup &group) {
  m_groups.push_back(group);
}

TweakInfo *BaseModule::FindTweak(const std::string &tweakId) {
  auto it = m_tweakIndex.find(tweakId);
  if (it != m_tweakIndex.end() && it->second < m_tweaks.size()) {
    return &m_tweaks[it->second];
  }
  return nullptr;
}

}
