#ifndef LINTEL_SHAREDMEMORYSEGMENTSTATS_H
#define LINTEL_SHAREDMEMORYSEGMENTSTATS_H

#include <cstddef>
#include <string>

namespace lintel {

/** Runtime statistics for a shared memory segment. */
struct SharedMemorySegmentStats
{
  std::string name;
  std::size_t allocatedBytes = 0;
  std::size_t capacityBytes = 0;
  std::size_t freeBytes = 0;
  std::size_t growCount = 0;
  std::size_t shrinkCount = 0;
  std::size_t recoveryCount = 0;
  std::size_t generation = 0;
};

}// namespace lintel
#endif// LINTEL_SHAREDMEMORYSEGMENTSTATS_H
