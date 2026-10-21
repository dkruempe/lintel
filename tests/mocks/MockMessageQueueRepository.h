#ifndef LINTEL_MOCKMESSAGEQUEUEREPOSITORY_H
#define LINTEL_MOCKMESSAGEQUEUEREPOSITORY_H

#include <catch2/trompeloeil.hpp>

#include <chrono>
#include <cstdint>
#include <optional>

#include "lintel/features/base/configuration/MessageQueueEntry.h"
#include "lintel/features/base/repositories/IMessageQueueRepository.h"

namespace lintel {
class MockMessageQueueRepository : public IMessageQueueRepository
{
public:
  MAKE_MOCK1(allMessageQueueNameOf, MessageQueueEntry(const std::string &), override);
  MAKE_MOCK2(allOf, std::vector<MessageQueueEntry>(const std::string &, const std::string &), override);
  MAKE_MOCK4(pageOf,
    Page<MessageQueueEntry>(const std::string &, const std::string &, const std::optional<std::string> &, std::size_t),
    override);
  MAKE_MOCK1(allProcessNameOf, std::vector<MessageQueueEntry>(const std::string &), override);
  MAKE_MOCK1(insertOf, void(const std::vector<MessageQueueEntry> &), override);
  MAKE_MOCK1(deleteOf, void(const std::vector<MessageQueueEntry> &), override);

  // Owner/liveness calls are recorded instead of mocked: they are fired from
  // MessageQueueService::of()/heartbeat timers where trompeloeil expectations
  // of a helper scope would already be dead, and tests assert on the records.
  std::vector<std::string> registeredOwners;
  std::vector<std::string> heartbeats;
  std::vector<std::string> reclaimedQueues;
  std::vector<MessageQueueEntry> orphansToFind;

  void registerOwner(const std::string &queueName,
    const std::string &processName,
    const std::string &hostName,
    std::uint64_t,
    const std::chrono::system_clock::time_point &,
    const std::string &) override
  {
    registeredOwners.push_back(queueName + "/" + processName + "/" + hostName);
  }

  void updateHeartbeat(const std::string &queueName, const std::chrono::system_clock::time_point &) override
  {
    heartbeats.push_back(queueName);
  }

  void updateOwnerState(const std::string &, const std::string &) override {}

  std::vector<MessageQueueEntry> findOrphans(const std::chrono::seconds &) override { return orphansToFind; }

  void reclaimOrphans(const std::vector<MessageQueueEntry> &entries) override
  {
    for (const auto &entry : entries) { reclaimedQueues.push_back(entry.get_message_queue_name()); }
  }

  void clearOwner(const std::string &) override {}
};

}// namespace lintel
#endif// LINTEL_MOCKMESSAGEQUEUEREPOSITORY_H
