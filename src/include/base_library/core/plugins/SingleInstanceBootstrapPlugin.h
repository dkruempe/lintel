#ifndef CPP_BASE_LIBRARY_SINGLEINSTANCEBOOTSTRAPPLUGIN_H
#define CPP_BASE_LIBRARY_SINGLEINSTANCEBOOTSTRAPPLUGIN_H

#include <memory>

#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"
#include "base_library/core/plugins/BootstrapPlugin.h"
#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"
#include "base_library/features/base/models/ProcessName.h"

/**
 * Bootstrap plugin that guarantees that a process with a database connection
 * (the server) only runs once.
 *
 * The plugin acquires an exclusive operating system file lock on
 * &lt;shared memory path&gt;/&lt;process name&gt;.lock and holds it for the lifetime of
 * the process. A second instance of the same process detects the conflict on
 * startup and aborts fatally. The lock is released automatically by the
 * operating system when the process exits or crashes, so no stale locks
 * accumulate.
 *
 * Processes without a default database connection (e.g. worker processes)
 * are intentionally not restricted and skip the lock entirely.
 */
class SingleInstanceBootstrapPlugin : public BootstrapPlugin {
private:
    /** Boost typed operating system file lock. Defined in the implementation
     *  file to keep boost out of this header; the plugin holds it for its
     *  whole lifetime so a second instance detects the conflict. */
    class Lock;

    std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;
    std::shared_ptr<Configuration> m_configuration;
    std::shared_ptr<ProcessName> m_processName;
    std::unique_ptr<Lock> m_fileLock;

    /** Resolve the lock file path and acquire the exclusive lock.
     * @throws std::runtime_error if another instance is already running */
    void acquireLock();

public:
    /** Construct a SingleInstanceBootstrapPlugin.
     * @param connectionConfigurations database connection configurations
     * @param configuration application configuration containing the shared
     *        memory path used for the lock file location
     * @param processName the process name of the running process */
    SingleInstanceBootstrapPlugin(
            const std::shared_ptr<DatabaseConnectionConfigurations>
            &connectionConfigurations,
            std::shared_ptr<Configuration> configuration,
            std::shared_ptr<ProcessName> processName);

    ~SingleInstanceBootstrapPlugin() override;

    /** Acquire the single instance lock unless no database connection exists. */
    void onStart() override;

    /** Returns the priority for ordering this plugin in the bootstrap sequence. */
    BootstrapSequence getPriority() override;
};

#endif  // CPP_BASE_LIBRARY_SINGLEINSTANCEBOOTSTRAPPLUGIN_H
