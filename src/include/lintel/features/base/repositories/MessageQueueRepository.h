#ifndef LINTEL_MESSAGEQUEUEREPOSITORY_H
#define LINTEL_MESSAGEQUEUEREPOSITORY_H

#include <chrono>
#include <cstdint>
#include <lintel/core/configuration/DatabaseConnectionEntry.h>
#include <lintel/core/persistence/DatabaseConnectionConfigurations.h>
#include <lintel/core/persistence/Result.h>
#include <lintel/features/base/configuration/MessageQueueEntry.h>
#include <lintel/features/base/models/Page.h>
#include <lintel/features/base/repositories/IMessageQueueRepository.h>
#include <memory>
#include <optional>
#include <vector>

namespace lintel {
/**
 * Database-backed implementation of IMessageQueueRepository.
 */
class MessageQueueRepository : public IMessageQueueRepository
{
private:
  std::shared_ptr<DatabaseConnectionConfigurations> m_connectionConfigurations;
  std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;

  MessageQueueEntry entryFromResult(db::Result &result, std::size_t row) const;

public:
  /**
   * @param name message queue name
   * @return the matching MessageQueueEntry
   */
  MessageQueueEntry allMessageQueueNameOf(const std::string &name) override;

  /**
   * @param filter process/queue name patterns
   * @return matching entries
   */
  std::vector<MessageQueueEntry> allOf(const MessageQueueFilter &filter) override;

  /**
   * Keyset-paginated queue query ordered by queue name ascending.
   * @param filter process/queue name patterns
   * @param afterName exclusive lower bound on the queue name; pass the
   *                  previous page's nextAfter value (nullopt for first page)
   * @param limit maximum number of entries per page
   * @return one page of entries plus continuation info
   */
  Page<MessageQueueEntry>
    pageOf(const MessageQueueFilter &filter, const std::optional<std::string> &afterName, std::size_t limit) override;

  /**
   * @param processName filter by process name
   * @return all entries for the given process
   */
  std::vector<MessageQueueEntry> allProcessNameOf(const std::string &processName) override;

  /**
   * Insert entries.
   * @param entries entries to insert
   */
  void insertOf(const std::vector<MessageQueueEntry> &entries) override;

  /**
   * Delete entries.
   * @param entries entries to delete
   */
  void deleteOf(const std::vector<MessageQueueEntry> &entries) override;

  /**
   * Register the owner of a queue (or refresh its registration).
   * @param registration the owner record incl. its identity and timestamps
   */
  void registerOwner(const MessageQueueOwnerRegistration &registration) override;
  void updateHeartbeat(const std::string &queueName, const std::chrono::system_clock::time_point &heartbeatAt) override;
  void updateOwnerState(const MessageQueueOwnerStateUpdate &update) override;
  std::vector<MessageQueueEntry> findOrphans(const std::chrono::seconds &staleThreshold) override;
  void reclaimOrphans(const std::vector<MessageQueueEntry> &entries) override;
  void clearOwner(const std::string &queueName) override;

  /**
   * Constructor.
   * @param connectionConfigurations database connection configuration
   */
  MessageQueueRepository(const std::shared_ptr<DatabaseConnectionConfigurations> &connectionConfigurations);
};

}// namespace lintel
#endif// LINTEL_MESSAGEQUEUEREPOSITORY_H