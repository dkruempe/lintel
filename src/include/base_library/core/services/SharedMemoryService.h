#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYSERVICE_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYSERVICE_H

#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <set>
#include <utility>

#include "base_library/core/exceptions/ShmSegmentNotFound.h"
#include "base_library/core/models/SharedMemorySegment.h"
#include "base_library/core/services/ISharedMemoryService.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/core/services/PropertyRegistration.h"
#include "base_library/features/base/events/ShmSegmentAccessor.h"
#include "base_library/features/base/models/SharedMemorySegmentInfo.h"
#include "base_library/features/base/services/SchedulerService.h"
#include "base_library/features/base/services/SharedMemorySegmentManager.h"

/** Allocated state of a mapped shared memory segment. Defined in
 *  ShmConstructors.h, because it exposes the boost interprocess types. */
class SharedMemorySegmentHandle;

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
  // properties
  std::shared_ptr<Property<std::chrono::seconds>> m_scheduleRate;
  std::shared_ptr<Property<bool>> m_autoExtend;
  std::shared_ptr<Property<std::size_t>> m_autoExtendEpsilon;

  // initializer function
  static std::map<std::string, std::shared_ptr<SharedMemorySegmentHandle>> create(
    const std::vector<std::shared_ptr<SharedMemorySegment>> &set);

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

  /** Get the allocated state of the named shared memory segment.
   *  Internal: the caller has to hold segmentsMutexOf().
   * @param name the name of the segment
   * @return the segment handle
   * @throws ShmSegmentNotFound if the segment does not exist */
  SharedMemorySegmentHandle &segmentOf(const std::string &name);

  /** Get an accessor to allocate from the given shared memory segment. The
   * returned accessor is a value type and stays valid as long as the segment
   * is mapped by this service.
   * @param segment the shared memory segment
   * @return accessor of the segment
   * @throws ShmSegmentNotFound if the segment does not exist */
  ShmSegmentAccessor getSegmentAccessor(const std::shared_ptr<SharedMemorySegment> &segment);
};

#endif// CPP_BASE_LIBRARY_SHAREDMEMORYSERVICE_H
