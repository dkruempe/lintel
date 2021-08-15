#ifndef CPP_BASE_LIBRARY_BOOTSTRAPSERVICE_H
#define CPP_BASE_LIBRARY_BOOTSTRAPSERVICE_H

#include <memory>
#include <vector>

#include "base_library/core/plugins/BootstrapPlugin.h"

class BootstrapService {
 private:
  std::vector<std::shared_ptr<BootstrapPlugin>> m_bootstrapPlugins;

 public:
  explicit BootstrapService(std::vector<std::shared_ptr<BootstrapPlugin>> bootstrapPlugins);

  void onStart();
};

#endif  // CPP_BASE_LIBRARY_BOOTSTRAPSERVICE_H
