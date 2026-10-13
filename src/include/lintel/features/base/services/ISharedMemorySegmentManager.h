#ifndef LINTEL_ISHAREDMEMORYSEGMENTMANAGER_H
#define LINTEL_ISHAREDMEMORYSEGMENTMANAGER_H

#include <memory>
#include <string>
#include <vector>

namespace lintel {
class SharedMemorySegment;

class ISharedMemorySegmentManager
{
public:
  virtual ~ISharedMemorySegmentManager() = default;

  virtual std::shared_ptr<SharedMemorySegment> of(const std::string &sharedMemorySegmentName) = 0;

  virtual std::vector<std::shared_ptr<SharedMemorySegment>> allOf(const std::string &segmentName = ".*") = 0;
};

}// namespace lintel
#endif
