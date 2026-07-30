#ifndef CPP_BASE_LIBRARY_MOCKSHAREDMEMORYSEGMENTMANAGER_H
#define CPP_BASE_LIBRARY_MOCKSHAREDMEMORYSEGMENTMANAGER_H

#include <catch2/trompeloeil.hpp>

#include "base_library/features/base/services/ISharedMemorySegmentManager.h"

class MockSharedMemorySegmentManager : public ISharedMemorySegmentManager {
public:
    MAKE_MOCK1(of, std::shared_ptr<SharedMemorySegment>(const std::string &), override);
    MAKE_MOCK1(allOf, std::vector<std::shared_ptr<SharedMemorySegment>>(const std::string &), override);
};

#endif
