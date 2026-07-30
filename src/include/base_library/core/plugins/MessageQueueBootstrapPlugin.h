#ifndef CPP_BASE_LIBRARY_MESSAGEQUEUEBOOTSTRAPPLUGIN_H
#define CPP_BASE_LIBRARY_MESSAGEQUEUEBOOTSTRAPPLUGIN_H

#include <base_library/features/base/configuration/Configuration.h>
#include <base_library/features/base/repositories/IMessageQueueRepository.h>
#include <base_library/features/base/models/ProcessName.h>
#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"
#include "base_library/core/plugins/BootstrapPlugin.h"
#include "base_library/features/base/provider/GroupProvider.h"

class MessageQueueBootstrapPlugin : public BootstrapPlugin {
private:
    std::shared_ptr<IMessageQueueRepository> m_messageQueueRepository;
    std::shared_ptr<Configuration> m_configuration;
    std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;
    std::shared_ptr<ProcessName> m_processName;

public:
    explicit MessageQueueBootstrapPlugin(
            const std::shared_ptr<DatabaseConnectionConfigurations> &connectionConfigurations,
            std::shared_ptr<Configuration> configuration,
            std::shared_ptr<IMessageQueueRepository> messageQueueRepository,
            std::shared_ptr<ProcessName> processName);

    void onStart() override;

    BootstrapSequence getPriority() override;
};

#endif  // CPP_BASE_LIBRARY_MESSAGEQUEUEBOOTSTRAPPLUGIN_H
