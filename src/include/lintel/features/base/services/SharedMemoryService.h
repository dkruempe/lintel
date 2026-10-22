#ifndef LINTEL_SHAREDMEMORYSERVICE_H
#define LINTEL_SHAREDMEMORYSERVICE_H

#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <set>
#include <utility>
#include <vector>

#include "lintel/core/exceptions/ShmSegmentNotFound.h"
#include "lintel/core/models/SharedMemorySegment.h"
#include "lintel/core/models/SharedMemorySegmentInfo.h"
#include "lintel/core/services/ISharedMemoryService.h"
#include "lintel/core/services/LoggerService.h"
#include "lintel/core/services/PropertyRegistration.h"
#include "lintel/core/shm/ShmSegmentAccessor.h"
#include "lintel/features/base/models/SharedMemorySegmentStats.h"
#include "lintel/features/base/services/SchedulerService.h"
#include "lintel/features/base/services/SharedMemorySegmentManager.h"

namespace lintel {
/** Allocated state of a mapped shared memory segment. Defined in
 *  ShmConstructors.h, because it exposes the boost interprocess types. */
class SharedMemorySegmentHandle;

/** Lifecycle events for shared memory segments. */
enum class SegmentLifecycle : std::uint8_t { BeforeGrow, AfterGrow, BeforeShrink, AfterShrink, OnRemap };

/** Callback invoked on segment lifecycle events. Callbacks must be short and non-blocking. */
using SegmentLifecycleHook = std::function<void(const std::string &segmentName, SegmentLifecycle action)>;

/** Service for managing shared memory segments and constructing data structures
 *  within them.
 *
 *  The service itself is boost free: everything that needs the boost
 *  interprocess types lives in ShmConstructors.h (free functions taking the
 *  service as first argument) and in ShmSegmentAccessor.h. */
class SharedMemoryService
  : public PropertyRegistration<SharedMemoryService>
  , public ISharedMemoryService
  , public std::enable_shared_from_this<SharedMemoryService>
{
private:
  // variables
  std::map<std::string, std::shared_ptr<SharedMemorySegmentHandle>> m_segments;
  mutable std::mutex m_segmentsMutex;
  std::size_t m_generation = 0;
  std::shared_ptr<SchedulerService> m_schedulerService;
  /** grow/shrink/reclaim counters per segment name (for statsOf) */
  std::map<std::string, std::size_t> m_growCount;
  std::map<std::string, std::size_t> m_shrinkCount;
  std::map<std::string, std::size_t> m_recoveryCount;
  /** registered lifecycle hooks and their mutex */
  mutable std::mutex m_hooksMutex;
  std::vector<SegmentLifecycleHook> m_lifecycleHooks;
  // properties
  std::shared_ptr<Property<std::chrono::seconds>> m_scheduleRate;
  std::shared_ptr<Property<bool>> m_autoExtend;
  std::shared_ptr<Property<std::size_t>> m_autoExtendEpsilon;

  // initializer function
  static std::map<std::string, std::shared_ptr<SharedMemorySegmentHandle>> create(
    const std::vector<std::shared_ptr<SharedMemorySegment>> &set);

  void notifyHooks(const std::string &segmentName, SegmentLifecycle action);

  void onCheck();

public:
  /** Construct a SharedMemoryService.
   * @param sharedMemorySegmentManager the segment manager providing segment definitions
   * @param schedulerService           the scheduler for periodic maintenance tasks
   * @param processName                the process name */
  SharedMemoryService(const std::shared_ptr<SharedMemorySegmentManager> &sharedMemorySegmentManager,
    std::shared_ptr<SchedulerService> schedulerService,
    const std::shared_ptr<ProcessName> &processName);

  ~SharedMemoryService() override = default;

  /** Initialize all shared memory segments by opening or creating them. */
  void onInitialize() override;

  /** Generation counter incremented on every grow/shrink. Repositories use it
   * to detect that a segment was remapped and re-fetch their references.
   * @return the current generation */
  [[nodiscard]] std::size_t getGeneration() const;

  /** grows the size of the mentioned shared memory block */
  void growOf(const std::shared_ptr<SharedMemorySegment> &segment, std::size_t grow) override;

  /** shrinks the shared memory size to the minimum */
  void shrinkOf(const std::shared_ptr<SharedMemorySegment> &segment) override;

  /** Reclaim the remap locks of segments whose holder process is gone.
   *
   * Iterates over the segments this process has mapped and hands each one to
   * shm::recoverOrphanedSegmentLock(), which reclaims the remap semaphore only
   * when its recorded holder process no longer exists; a lock whose holder is
   * still alive is never touched. A reclaimed semaphore is reopened so this
   * process keeps using the object every other process sees, and the segment's
   * recovery counter (reported by statsOf() as RECOVERY_COUNT) is bumped.
   *
   * Opening a segment already runs the same check once (see create()), so this
   * is the repeat an operator needs for a segment that got stuck later on.
   * @return the number of locks that were reclaimed */
  std::size_t recover() override;

  /**
   * shows state of the mentioned shared memory segment
   * @param segment
   * @return string with the printed information
   */
  [[nodiscard]] SharedMemorySegmentInfo showStateOf(const std::shared_ptr<SharedMemorySegment> &segment) const override;

  /** @return the raw segment map. Internal: used by the shm construct
   *  helpers of ShmConstructors.h. */
  std::map<std::string, std::shared_ptr<SharedMemorySegmentHandle>> &segmentsOf() { return m_segments; }

  /** @return the mutex guarding the segment map. Internal: used by the shm
   *  construct helpers of ShmConstructors.h, which take the lock before
   *  calling segmentOf. */
  std::mutex &segmentsMutexOf() { return m_segmentsMutex; }

  /** Register a lifecycle hook. Hook must be short and non-blocking. */
  void registerLifecycleHook(SegmentLifecycleHook hook);

  /** Get statistics for a segment.
   * @param segment the segment to inspect
   * @return size, capacity, free bytes, lifecycle counters and generation
   * @throws ShmSegmentNotFound if the segment is not mapped by this service */
  [[nodiscard]] SharedMemorySegmentStats statsOf(const std::shared_ptr<SharedMemorySegment> &segment) const override;

  /** Get the allocated state of the named shared memory segment.
   *  Internal: the caller has to hold segmentsMutexOf().
   * @param name the name of the segment
   * @return the segment handle
   * @throws ShmSegmentNotFound if the segment does not exist */
  SharedMemorySegmentHandle &segmentOf(const std::string &name);

  /** Get an accessor to allocate from the given shared memory segment. The
   * returned accessor is a value type and stays valid as long as the segment
   * is mapped by this service.
   *
   * The accessor carries the segment name and this process' allocator is bound
   * to that name, so a copy stored inside the segment allocates from the
   * process that uses it, not from the process that created it.
   *
   * @param segment the shared memory segment
   * @return accessor of the segment
   * @throws ShmSegmentNotFound if the segment does not exist
   * @throws std::invalid_argument if the segment name is longer than
   *         ShmSegmentAccessor::NAME_SIZE - 1 characters */
  ShmSegmentAccessor getSegmentAccessor(const std::shared_ptr<SharedMemorySegment> &segment);
};

}// namespace lintel
#endif// LINTEL_SHAREDMEMORYSERVICE_H
