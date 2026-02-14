
#pragma once

#include "BaseModule.h"

namespace Vax::Modules {

class NetworkModule : public BaseModule {
public:
  NetworkModule();
  ~NetworkModule() override = default;

  bool ApplyTweak(const std::string &tweakId) override;
  bool RevertTweak(const std::string &tweakId) override;
  void RefreshStatus() override;

private:
  void InitializeTweaks();

  bool ApplyNicProperty(const std::string &property, const std::string &value);
  bool RevertNicProperty(const std::string &property);
  bool IsNicPropertySet(const std::string &property,
                        const std::string &expected);
  std::string FindActiveAdapterRegistryKey();

  bool ApplyDnsServer(const std::string &primary, const std::string &secondary);
  bool RevertDnsServer();
  bool IsDnsServerSet(const std::string &primary);

  bool ApplyNetBios();
  bool RevertNetBios();
  bool IsNetBiosDisabled();

  bool ApplyNagle();
  bool RevertNagle();
  bool IsNagleDisabled();

  bool ApplyTcpSystemSettings();
  bool RevertTcpSystemSettings();
  bool IsTcpSystemOptimized();

  bool ResetWinsock();
  bool ResetTcpIp();
  bool FlushDns();
};

}
