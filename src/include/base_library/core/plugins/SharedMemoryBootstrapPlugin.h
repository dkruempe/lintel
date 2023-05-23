#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYBOOTSTRAPPLUGIN_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYBOOTSTRAPPLUGIN_H

#include <memory>

#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"
#include "base_library/core/plugins/BootstrapPlugin.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"
#include "base_library/features/base/repositories/SharedMemoryRepository.h"

class SharedMemoryBootstrapPlugin : public BootstrapPlugin {
private:
    std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;
    std::vector<std::shared_ptr<SharedMemoryRepository>>
            m_sharedMemoryRepositories;

public:
    SharedMemoryBootstrapPlugin(
            const std::shared_ptr<DatabaseConnectionConfigurations>
            &connectionConfigurations,
            std::vector<std::shared_ptr<SharedMemoryRepository>>
            sharedMemoryRepositories);

    void onStart() override;

    BootstrapSequence getPriority() override;
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYBOOTSTRAPPLUGIN_H
