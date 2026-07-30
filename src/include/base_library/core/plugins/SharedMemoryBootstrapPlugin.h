#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYBOOTSTRAPPLUGIN_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYBOOTSTRAPPLUGIN_H

#include <memory>

#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"
#include "base_library/core/plugins/BootstrapPlugin.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"
#include "base_library/features/base/repositories/SharedMemoryRepository.h"

/** Bootstrap plugin that initializes shared memory segments from the database. */
class SharedMemoryBootstrapPlugin : public BootstrapPlugin {
private:
    std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;
    std::vector<std::shared_ptr<SharedMemoryRepository>>
            m_sharedMemoryRepositories;

public:
    /** Construct a SharedMemoryBootstrapPlugin.
     * @param connectionConfigurations database connection configurations
     * @param sharedMemoryRepositories list of shared memory repositories to initialize */
    SharedMemoryBootstrapPlugin(
            const std::shared_ptr<DatabaseConnectionConfigurations>
            &connectionConfigurations,
            std::vector<std::shared_ptr<SharedMemoryRepository>>
            sharedMemoryRepositories);

    /** Load shared memory data from the database into shared memory segments. */
    void onStart() override;

    /** Returns the priority for ordering this plugin in the bootstrap sequence. */
    BootstrapSequence getPriority() override;
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYBOOTSTRAPPLUGIN_H
