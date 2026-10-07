#ifndef LINTEL_MOCKMESSAGEQUEUEREPOSITORY_H
#define LINTEL_MOCKMESSAGEQUEUEREPOSITORY_H

#include <catch2/trompeloeil.hpp>

#include <optional>

#include "lintel/features/base/configuration/MessageQueueEntry.h"
#include "lintel/features/base/repositories/IMessageQueueRepository.h"

class MockMessageQueueRepository : public IMessageQueueRepository {
public:
    MAKE_MOCK1(allMessageQueueNameOf, MessageQueueEntry(const std::string &), override);
    MAKE_MOCK2(allOf, std::vector<MessageQueueEntry>(const std::string &, const std::string &), override);
    MAKE_MOCK4(pageOf, Page<MessageQueueEntry>(const std::string &, const std::string &, const std::optional<std::string> &, std::size_t), override);
    MAKE_MOCK1(allProcessNameOf, std::vector<MessageQueueEntry>(const std::string &), override);
    MAKE_MOCK1(insertOf, void(const std::vector<MessageQueueEntry> &), override);
    MAKE_MOCK1(deleteOf, void(const std::vector<MessageQueueEntry> &), override);
};

#endif  // LINTEL_MOCKMESSAGEQUEUEREPOSITORY_H
