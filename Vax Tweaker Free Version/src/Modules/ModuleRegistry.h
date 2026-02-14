
#pragma once

#include "IModule.h"
#include <memory>
#include <vector>

namespace Vax::Modules {

    class ModuleRegistry {
    public:
        static ModuleRegistry& Instance();

        void Register(std::unique_ptr<IModule> module);

        const std::vector<std::unique_ptr<IModule>>& GetAll() const;

        IModule* GetById(int id) const;

        std::vector<ModuleInfo> GetModuleInfoList() const;

        size_t Count() const;

        void InitializeDefaults();

    private:
        ModuleRegistry() = default;
        ~ModuleRegistry() = default;

        ModuleRegistry(const ModuleRegistry&) = delete;
        ModuleRegistry& operator=(const ModuleRegistry&) = delete;

        std::vector<std::unique_ptr<IModule>> m_modules;
    };

}
