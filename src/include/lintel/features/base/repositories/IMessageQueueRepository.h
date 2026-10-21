#ifndef LINTEL_IMESSAGEQUEUEREPOSITORY_H
#define LINTEL_IMESSAGEQUEUEREPOSITORY_H

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lintel/features/base/models/Page.h"

namespace lintel {
class MessageQueueEntry;

/**
 * Interface for message queue persistence operations.
 */
class IMessageQueueRepository
{
public:
  virtual ~IMessageQueueRepository() = default;

  /**
   * @param name message queue name
   * @return the matching MessageQueueEntry
   */
  virtual MessageQueueEntry allMessageQueueNameOf(const std::string &name) = 0;

  /**
   * @param processName process name filter
   * @param messageQueueName queue name filter
   * @return matching entries
   */
  virtual std::vector<MessageQueueEntry> allOf(const std::string &processName, const std::string &messageQueueName) = 0;

  /**
   * Keyset-paginated queue query ordered by queue name ascending.
   * @param processName process name filter
   * @param messageQueueName queue name filter
   * @param afterName exclusive lower bound on the queue name; pass the
   *                  previous page's nextAfter value (nullopt for first page)
   * @param limit maximum number of entries per page
   * @return one page of entries plus continuation info
   */
  virtual Page<MessageQueueEntry> pageOf(const std::string &processName,
    const std::string &messageQueueName,
    const std::optional<std::string> &afterName,
    std::size_t limit) = 0;

  /**
   * @param processName filter by process name
   * @return all entries for the given process
   */
  virtual std::vector<MessageQueueEntry> allProcessNameOf(const std::string &processName) = 0;

  /**
   * Insert entries.
   * @param entries entries to insert
   */
  virtual void insertOf(const std::vector<MessageQueueEntry> &entries) = 0;

  /**
   * Delete entries.
   * @param entries entries to delete
   */
  virtual void deleteOf(const std::vector<MessageQueueEntry> &entries) = 0;

  virtual void registerOwner(const std::string &queueName,
    const std::string &processName,
    const std::string &hostName,
    std::uint64_t pid,
    const std::chrono::system_clock::time_point &startedAt,
    const std::string &ownerState = "active") = 0;
  virtual void updateHeartbeat(const std::string &queueName,
    const std::chrono::system_clock::time_point &heartbeatAt) = 0;
  virtual void updateOwnerState(const std::string &queueName, const std::string &ownerState) = 0;
  virtual std::vector<MessageQueueEntry> findOrphans(const std::chrono::seconds &staleThreshold) = 0;
  virtual void reclaimOrphans(const std::vector<MessageQueueEntry> &entries) = 0;
  virtual void clearOwner(const std::string &queueName) = 0;
};

}// namespace lintel
#endif// LINTEL_IMESSAGEQUEUEREPOSITORY_H
