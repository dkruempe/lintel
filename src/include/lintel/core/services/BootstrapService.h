#ifndef LINTEL_BOOTSTRAPSERVICE_H
#define LINTEL_BOOTSTRAPSERVICE_H

#include <memory>
#include <vector>

#include "lintel/core/plugins/BootstrapPlugin.h"

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

#endif  // LINTEL_BOOTSTRAPSERVICE_H
