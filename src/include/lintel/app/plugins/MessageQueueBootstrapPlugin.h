#ifndef LINTEL_MESSAGEQUEUEBOOTSTRAPPLUGIN_H
#define LINTEL_MESSAGEQUEUEBOOTSTRAPPLUGIN_H

#include "lintel/core/persistence/DatabaseConnectionConfigurations.h"
#include "lintel/core/plugins/BootstrapPlugin.h"
#include "lintel/features/base/provider/GroupProvider.h"
#include <lintel/core/configuration/Configuration.h>
#include <lintel/core/models/ProcessName.h>
#include <lintel/features/base/repositories/IMessageQueueRepository.h>

namespace lintel {
/** Bootstrap plugin that initializes message queue tables and repositories on startup. */
class MessageQueueBootstrapPlugin : public BootstrapPlugin
{
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

}// namespace lintel
#endif// LINTEL_MESSAGEQUEUEBOOTSTRAPPLUGIN_H
