#ifndef CPP_BASE_LIBRARY_BOOTSTRAPPLUGIN_H
#define CPP_BASE_LIBRARY_BOOTSTRAPPLUGIN_H

#include "base_library/core/models/BootstrapSequence.h"

/** Base class for bootstrap plugins that are executed during application startup. */
class BootstrapPlugin {
public:
    BootstrapPlugin() = default;

    virtual ~BootstrapPlugin() = default;

    /** Called when the bootstrap plugin should execute its initialization logic. */
    virtual void onStart() = 0;

    /** Returns the execution priority of this bootstrap plugin.
     * @return the priority determining the order of plugin execution */
    virtual BootstrapSequence getPriority() = 0;
};

#endif  // CPP_BASE_LIBRARY_BOOTSTRAPPLUGIN_H
