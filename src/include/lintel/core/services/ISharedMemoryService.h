#ifndef LINTEL_ISHAREDMEMORYSERVICE_H
#define LINTEL_ISHAREDMEMORYSERVICE_H

#include <cstddef>
#include <memory>

namespace lintel {
class SharedMemorySegment;
class SharedMemorySegmentInfo;
struct SharedMemorySegmentStats;

class ISharedMemoryService
{
public:
  virtual ~ISharedMemoryService() = default;

  virtual SharedMemorySegmentInfo showStateOf(const std::shared_ptr<SharedMemorySegment> &segment) const = 0;

  /**
   * Collect the runtime statistics of a segment.
   *
   * @param segment the segment to inspect
   * @return size, capacity, free bytes, lifecycle counters and generation
   * @throws ShmSegmentNotFound if the service does not map the segment
   */
  [[nodiscard]] virtual SharedMemorySegmentStats statsOf(const std::shared_ptr<SharedMemorySegment> &segment) const = 0;

  virtual void growOf(const std::shared_ptr<SharedMemorySegment> &segment, std::size_t grow) = 0;

  virtual void shrinkOf(const std::shared_ptr<SharedMemorySegment> &segment) = 0;

  /**
   * Reclaim the locks of segments whose holder process is gone.
   *
   * A process that crashes while holding a segment's remap semaphore leaves it
   * stuck, which would make every later acquirer wait for the bounded timeout
   * and then fail. Opening a segment runs the same check once; this method
   * repeats it for a segment that got stuck later on. It never touches a lock
   * whose holder is still alive.
   *
   * @return the number of locks that were reclaimed
   */
  virtual std::size_t recover() { return 0; }

  virtual void onInitialize() = 0;
};

}// namespace lintel
#endif
