#ifndef CPP_BASE_LIBRARY_MOCKMESSAGEQUEUEREPOSITORY_H
#define CPP_BASE_LIBRARY_MOCKMESSAGEQUEUEREPOSITORY_H

#include <catch2/trompeloeil.hpp>

#include "base_library/features/base/configuration/MessageQueueEntry.h"
#include "base_library/features/base/repositories/IMessageQueueRepository.h"

class MockMessageQueueRepository : public IMessageQueueRepository {
public:
    MAKE_MOCK1(allMessageQueueNameOf, MessageQueueEntry(const std::string &), override);
    MAKE_MOCK2(allOf, std::vector<MessageQueueEntry>(const std::string &, const std::string &), override);
    MAKE_MOCK1(allProcessNameOf, std::vector<MessageQueueEntry>(const std::string &), override);
    MAKE_MOCK1(insertOf, void(const std::vector<MessageQueueEntry> &), override);
    MAKE_MOCK1(deleteOf, void(const std::vector<MessageQueueEntry> &), override);
};

#endif  // CPP_BASE_LIBRARY_MOCKMESSAGEQUEUEREPOSITORY_H
