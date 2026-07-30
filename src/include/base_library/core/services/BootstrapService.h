#ifndef CPP_BASE_LIBRARY_BOOTSTRAPSERVICE_H
#define CPP_BASE_LIBRARY_BOOTSTRAPSERVICE_H

#include <memory>
#include <vector>

#include "base_library/core/plugins/BootstrapPlugin.h"

/** Service that coordinates the execution of bootstrap plugins during startup. */
class BootstrapService {
private:
    std::vector<std::shared_ptr<BootstrapPlugin>> m_bootstrapPlugins;

public:
    /** Construct a BootstrapService with a list of bootstrap plugins.
     * @param bootstrapPlugins the plugins to execute */
    explicit BootstrapService(
            std::vector<std::shared_ptr<BootstrapPlugin>> bootstrapPlugins);

    /** Execute all bootstrap plugins in priority order. */
    void onStart();
};

#endif  // CPP_BASE_LIBRARY_BOOTSTRAPSERVICE_H
