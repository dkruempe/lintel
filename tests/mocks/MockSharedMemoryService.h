#ifndef LINTEL_MOCKSHAREDMEMORYSERVICE_H
#define LINTEL_MOCKSHAREDMEMORYSERVICE_H

#include <catch2/trompeloeil.hpp>

#include "lintel/core/models/SharedMemorySegmentInfo.h"
#include "lintel/core/services/ISharedMemoryService.h"

namespace lintel {
class MockSharedMemoryService : public ISharedMemoryService
{
public:
  MAKE_CONST_MOCK1(showStateOf, SharedMemorySegmentInfo(const std::shared_ptr<SharedMemorySegment> &), override);
  MAKE_MOCK2(growOf, void(const std::shared_ptr<SharedMemorySegment> &, std::size_t), override);
  MAKE_MOCK1(shrinkOf, void(const std::shared_ptr<SharedMemorySegment> &), override);
  MAKE_MOCK0(onInitialize, void(), override);
};

}// namespace lintel
#endif
