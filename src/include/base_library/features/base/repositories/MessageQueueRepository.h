#ifndef CPP_BASE_LIBRARY_MESSAGEQUEUEREPOSITORY_H
#define CPP_BASE_LIBRARY_MESSAGEQUEUEREPOSITORY_H

#include <memory>
#include <optional>
#include <vector>
#include <base_library/features/base/configuration/MessageQueueEntry.h>
#include <base_library/core/persistence/DatabaseConnectionConfigurations.h>
#include <base_library/features/base/configuration/DatabaseConnectionEntry.h>
#include <base_library/features/base/models/Page.h>
#include <base_library/features/base/repositories/IMessageQueueRepository.h>

/**
 * Database-backed implementation of IMessageQueueRepository.
 */
class MessageQueueRepository : public IMessageQueueRepository
{
private:
  std::shared_ptr<DatabaseConnectionConfigurations> m_connectionConfigurations;
  std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;

public:
  /**
   * @param name message queue name
   * @return the matching MessageQueueEntry
   */
  MessageQueueEntry allMessageQueueNameOf(const std::string &name);

  /**
   * @param processName process name filter
   * @param messageQueueName queue name filter
   * @return matching entries
   */
  std::vector<MessageQueueEntry> allOf(const std::string &processName, const std::string &messageQueueName);

  /**
   * Keyset-paginated queue query ordered by queue name ascending.
   * @param processName process name filter
   * @param messageQueueName queue name filter
   * @param afterName exclusive lower bound on the queue name; pass the
   *                  previous page's nextAfter value (nullopt for first page)
   * @param limit maximum number of entries per page
   * @return one page of entries plus continuation info
   */
  Page<MessageQueueEntry> pageOf(const std::string &processName,
                                 const std::string &messageQueueName,
                                 const std::optional<std::string> &afterName,
                                 std::size_t limit) override;

  /**
   * @param processName filter by process name
   * @return all entries for the given process
   */
  std::vector<MessageQueueEntry> allProcessNameOf(const std::string &processName);

  /**
   * Insert entries.
   * @param entries entries to insert
   */
  void insertOf(const std::vector<MessageQueueEntry> &entries);

  /**
   * Delete entries.
   * @param entries entries to delete
   */
  void deleteOf(const std::vector<MessageQueueEntry> &entries);

  /**
   * Constructor.
   * @param connectionConfigurations database connection configuration
   */
  MessageQueueRepository(const std::shared_ptr<DatabaseConnectionConfigurations> &connectionConfigurations);
};

#endif //CPP_BASE_LIBRARY_MESSAGEQUEUEREPOSITORY_H