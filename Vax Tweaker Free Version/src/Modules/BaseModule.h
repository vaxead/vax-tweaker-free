
#pragma once

#include "IModule.h"
#include <unordered_map>

namespace Vax::Modules {

    class BaseModule : public IModule {
    public:
        BaseModule(int id, const std::string& name, const std::string& description,
                   const std::string& icon, ModuleCategory category);

        virtual ~BaseModule() = default;

        ModuleInfo GetInfo() const override;
        std::vector<TweakInfo> GetTweaks() const override;
        std::vector<TweakGroup> GetGroups() const override;

        void Show() override;
        void Hide() override;

        void RefreshStatus() override;
        bool ApplyTweak(const std::string& tweakId) override;
        bool RevertTweak(const std::string& tweakId) override;

        bool RequiresAdmin() const override;
        bool IsImplemented() const override;

        const std::string& GetLastFailReason() const { return m_lastFailReason; }

    protected:
        ModuleInfo m_info;

        std::vector<TweakInfo> m_tweaks;

        std::vector<TweakGroup> m_groups;

        bool m_isImplemented = false;

        bool m_requiresAdmin = true;

        std::string m_lastFailReason;

        bool m_showTweakStatus = true;

        bool m_statusDirty = true;

        void RegisterTweak(const TweakInfo& tweak);

        void RegisterGroup(const TweakGroup& group);

        TweakInfo* FindTweak(const std::string& tweakId);

    private:
        std::unordered_map<std::string, size_t> m_tweakIndex;
        bool IsTargetApplied(const RegistryTarget& target) const;

        void ShowFlatList();

        void ShowGroupTweaks(int groupIndex);
    };

}
