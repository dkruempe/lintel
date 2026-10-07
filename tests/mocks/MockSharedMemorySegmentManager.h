#ifndef LINTEL_MOCKSHAREDMEMORYSEGMENTMANAGER_H
#define LINTEL_MOCKSHAREDMEMORYSEGMENTMANAGER_H

#include <catch2/trompeloeil.hpp>

#include "lintel/features/base/services/ISharedMemorySegmentManager.h"

class MockSharedMemorySegmentManager : public ISharedMemorySegmentManager {
public:
    MAKE_MOCK1(of, std::shared_ptr<SharedMemorySegment>(const std::string &), override);
    MAKE_MOCK1(allOf, std::vector<std::shared_ptr<SharedMemorySegment>>(const std::string &), override);
};

#endif
