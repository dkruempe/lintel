#ifndef LINTEL_ISHAREDMEMORYSERVICE_H
#define LINTEL_ISHAREDMEMORYSERVICE_H

#include <cstddef>
#include <memory>

namespace lintel {
class SharedMemorySegment;
class SharedMemorySegmentInfo;

class ISharedMemoryService
{
public:
  virtual ~ISharedMemoryService() = default;

  virtual SharedMemorySegmentInfo showStateOf(const std::shared_ptr<SharedMemorySegment> &segment) const = 0;

  virtual void growOf(const std::shared_ptr<SharedMemorySegment> &segment, std::size_t grow) = 0;

  virtual void shrinkOf(const std::shared_ptr<SharedMemorySegment> &segment) = 0;

  /**
   * Reclaim the locks of segments whose holder process is gone.
   *
   * A process that crashes while holding a segment's remap semaphore leaves it
   * stuck, which would make every later acquirer wait for the bounded timeout
   * and then fail. Called automatically when a segment is opened, so a segment
   * that gets stuck later on is recovered at its next open. It never touches a
   * lock whose holder is still alive.
   *
   * @return the number of locks that were reclaimed
   */
  virtual std::size_t recover() { return 0; }

  virtual void onInitialize() = 0;
};

}// namespace lintel
#endif
