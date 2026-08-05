#include "base_library/features/base/services/BootstrapService.h"

#include <algorithm>

BootstrapService::BootstrapService(
        std::vector<std::shared_ptr<BootstrapPlugin>> bootstrapPlugins)
        : m_bootstrapPlugins(std::move(bootstrapPlugins)) {}

void BootstrapService::onStart() {
    std::sort(m_bootstrapPlugins.begin(), m_bootstrapPlugins.end(),
              [](const std::shared_ptr<BootstrapPlugin> &a,
                 const std::shared_ptr<BootstrapPlugin> &b) -> bool {
                  return a->getPriority() < b->getPriority();
              });
    std::for_each(m_bootstrapPlugins.begin(), m_bootstrapPlugins.end(),
                  [](const std::shared_ptr<BootstrapPlugin> &plugin) {
                      plugin->onStart();
                  });
}
