
#pragma once

#include "../Core/Types.h"
#include <vector>
#include <string>

namespace Vax::Modules {

    class IModule {
    public:
        virtual ~IModule() = default;

        
        virtual ModuleInfo GetInfo() const = 0;

        virtual std::vector<TweakInfo> GetTweaks() const = 0;

        virtual std::vector<TweakGroup> GetGroups() const = 0;

        virtual void Show() = 0;

        virtual void Hide() = 0;

        virtual void RefreshStatus() = 0;

        virtual bool ApplyTweak(const std::string& tweakId) = 0;

        virtual bool RevertTweak(const std::string& tweakId) = 0;

        virtual bool RequiresAdmin() const = 0;

        virtual bool IsImplemented() const = 0;
    };

}
