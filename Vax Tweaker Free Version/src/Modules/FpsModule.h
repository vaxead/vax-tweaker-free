
#pragma once

#include "BaseModule.h"

namespace Vax::Modules {

class FpsModule : public BaseModule {
public:
  FpsModule();
  ~FpsModule() override = default;

  bool ApplyTweak(const std::string &tweakId) override;
  bool RevertTweak(const std::string &tweakId) override;
  void RefreshStatus() override;

private:
  void InitializeTweaks();

  bool ApplyAspm();
  bool RevertAspm();
  bool IsAspmDisabled();

  bool ApplyDisableProcesses();
  bool RevertDisableProcesses();
  bool IsDisableProcessesApplied();
};

}
