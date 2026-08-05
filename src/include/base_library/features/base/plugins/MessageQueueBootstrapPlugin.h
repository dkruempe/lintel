#ifndef CPP_BASE_LIBRARY_MESSAGEQUEUEBOOTSTRAPPLUGIN_H
#define CPP_BASE_LIBRARY_MESSAGEQUEUEBOOTSTRAPPLUGIN_H

#include <base_library/core/configuration/Configuration.h>
#include <base_library/features/base/repositories/IMessageQueueRepository.h>
#include <base_library/core/models/ProcessName.h>
#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"
#include "base_library/features/base/plugins/BootstrapPlugin.h"
#include "base_library/features/base/provider/GroupProvider.h"

/** Bootstrap plugin that initializes message queue tables and repositories on startup. */
class MessageQueueBootstrapPlugin : public BootstrapPlugin {
private:
    std::shared_ptr<IMessageQueueRepository> m_messageQueueRepository;
    std::shared_ptr<Configuration> m_configuration;
    std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;
    std::shared_ptr<ProcessName> m_processName;

public:
    /** Construct a MessageQueueBootstrapPlugin.
     * @param connectionConfigurations database connection configurations
     * @param configuration            application configuration
     * @param messageQueueRepository   repository for message queue operations
     * @param processName              the process name */
    explicit MessageQueueBootstrapPlugin(
            const std::shared_ptr<DatabaseConnectionConfigurations> &connectionConfigurations,
            std::shared_ptr<Configuration> configuration,
            std::shared_ptr<IMessageQueueRepository> messageQueueRepository,
            std::shared_ptr<ProcessName> processName);

    /** Initialize the message queue tables and repository. */
    void onStart() override;

    /** Returns the priority for ordering this plugin in the bootstrap sequence. */
    BootstrapSequence getPriority() override;
};

#endif  // CPP_BASE_LIBRARY_MESSAGEQUEUEBOOTSTRAPPLUGIN_H
