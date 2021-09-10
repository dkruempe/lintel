#ifndef CPP_BASE_LIBRARY_VIRTUALGROUPBOOTSTRAPPLUGIN_H
#define CPP_BASE_LIBRARY_VIRTUALGROUPBOOTSTRAPPLUGIN_H

#include "base_library/core/persistence/ConnectionConfigurations.h"
#include "base_library/core/plugins/BootstrapPlugin.h"
#include "base_library/features/base/provider/GroupProvider.h"

class VirtualGroupBootstrapPlugin : public BootstrapPlugin {
 private:
  std::vector<std::shared_ptr<GroupProvider>> m_groupProviders;
  std::shared_ptr<ConnectionEntry> m_connectionEntry;

 public:
  explicit VirtualGroupBootstrapPlugin(
      const std::shared_ptr<ConnectionConfigurations> &connectionConfigurations,
      std::vector<std::shared_ptr<GroupProvider>> groupProviders);
  void onStart() override;
  BootstrapSequence getPriority() override;
};

#endif  // CPP_BASE_LIBRARY_VIRTUALGROUPBOOTSTRAPPLUGIN_H
