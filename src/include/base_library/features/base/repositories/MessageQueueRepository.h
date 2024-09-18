#ifndef CPP_BASE_LIBRARY_MESSAGEQUEUEREPOSITORY_H
#define CPP_BASE_LIBRARY_MESSAGEQUEUEREPOSITORY_H

#include <memory>
#include <vector>
#include <base_library/features/base/configuration/MessageQueueEntry.h>
#include <base_library/core/persistence/DatabaseConnectionConfigurations.h>
#include <base_library/features/base/configuration/DatabaseConnectionEntry.h>

class MessageQueueRepository
{
private:
  std::shared_ptr<DatabaseConnectionConfigurations> m_connectionConfigurations;
  std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;

public:
  MessageQueueEntry allMessageQueueNameOf(const std::string &name);

  std::vector<MessageQueueEntry> allOf(const std::string &processName, const std::string &messageQueueName);

  std::vector<MessageQueueEntry> allProcessNameOf(const std::string &processName);

  void insertOf(const std::vector<MessageQueueEntry> &entries);

  void deleteOf(const std::vector<MessageQueueEntry> &entries);

  MessageQueueRepository(const std::shared_ptr<DatabaseConnectionConfigurations> &connectionConfigurations);
};

#endif //CPP_BASE_LIBRARY_MESSAGEQUEUEREPOSITORY_H