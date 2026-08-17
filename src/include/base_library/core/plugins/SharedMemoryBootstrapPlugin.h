#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYBOOTSTRAPPLUGIN_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYBOOTSTRAPPLUGIN_H

#include <memory>

#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"
#include "base_library/core/plugins/BootstrapPlugin.h"
#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"
#include "base_library/features/base/configuration/EventBusEntry.h"
#include "base_library/features/base/events/EventBus.h"
#include "base_library/features/base/repositories/SharedMemoryRepository.h"
#include "base_library/features/base/services/SharedMemorySegmentManager.h"

/** Bootstrap plugin that validates shared memory segments and initializes them
 * from the database. The structural validation (sanity and file size) and the
 * event bus configuration check run in every process regardless of a database
 * connection; the version bookmark check only runs when a default database
 * connection is configured. */
class SharedMemoryBootstrapPlugin : public BootstrapPlugin {
private:
    std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;
    std::vector<std::shared_ptr<SharedMemoryRepository>>
            m_sharedMemoryRepositories;
    std::shared_ptr<SharedMemorySegmentManager> m_sharedMemorySegmentManager;
    std::shared_ptr<Configuration> m_configuration;

    /** Validate the structure of every configured shared memory segment file.
     * Missing files are valid (they will be created). Throws on truncated or
     * corrupted segment files. */
    void validateSegments();

    /** Validate that every configured event bus stored in a shared memory
     * segment matches its configuration. Missing buses are valid (they will be
     * created). Throws if an existing bus has a different configuration. */
    void validateEventBuses();

    /** Convert an entry to a runtime bus configuration.
     * @param entry the configuration entry
     * @return the validated bus configuration
     * @throws std::runtime_error if the configuration is invalid */
    static EventBusConfig configOf(const std::shared_ptr<EventBusEntry> &entry);

public:
    /** Construct a SharedMemoryBootstrapPlugin.
     * @param connectionConfigurations database connection configurations
     * @param sharedMemoryRepositories list of shared memory repositories to initialize
     * @param sharedMemorySegmentManager configured shared memory segments
     * @param configuration application configuration containing event buses */
    SharedMemoryBootstrapPlugin(
            const std::shared_ptr<DatabaseConnectionConfigurations>
            &connectionConfigurations,
            std::vector<std::shared_ptr<SharedMemoryRepository>>
            sharedMemoryRepositories,
            std::shared_ptr<SharedMemorySegmentManager>
            sharedMemorySegmentManager,
            std::shared_ptr<Configuration> configuration);

    /** Load shared memory data from the database into shared memory segments. */
    void onStart() override;

    /** Returns the priority for ordering this plugin in the bootstrap sequence. */
    BootstrapSequence getPriority() override;
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYBOOTSTRAPPLUGIN_H
