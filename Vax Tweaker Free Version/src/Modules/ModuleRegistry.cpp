
#include "ModuleRegistry.h"
#include "CleanerModule.h"
#include "FpsModule.h"
#include "NetworkModule.h"

namespace Vax::Modules {

ModuleRegistry &ModuleRegistry::Instance() {
  static ModuleRegistry instance;
  return instance;
}

void ModuleRegistry::Register(std::unique_ptr<IModule> module) {
  m_modules.push_back(std::move(module));
}

const std::vector<std::unique_ptr<IModule>> &ModuleRegistry::GetAll() const {
  return m_modules;
}

IModule *ModuleRegistry::GetById(int id) const {
  for (const auto &module : m_modules) {
    if (module->GetInfo().id == id) {
      return module.get();
    }
  }
  return nullptr;
}

std::vector<ModuleInfo> ModuleRegistry::GetModuleInfoList() const {
  std::vector<ModuleInfo> result;
  result.reserve(m_modules.size());

  for (const auto &module : m_modules) {
    result.push_back(module->GetInfo());
  }

  return result;
}

size_t ModuleRegistry::Count() const { return m_modules.size(); }

void ModuleRegistry::InitializeDefaults() {
  Register(std::make_unique<FpsModule>());
  Register(std::make_unique<NetworkModule>());
  Register(std::make_unique<CleanerModule>());
}

}
