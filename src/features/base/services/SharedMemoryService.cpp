#include "lintel/features/base/services/SharedMemoryService.h"

#include "lintel/features/base/models/SharedMemorySegmentStats.h"
#include <lintel/core/models/SharedMemorySegment.h>
#include <lintel/core/models/SharedMemorySegmentInfo.h>
#include <lintel/core/services/ReadinessRegistry.h>
#include <lintel/core/shm/BoostSegmentAllocator.h>
#include <lintel/core/shm/SegmentLayoutPolicy.h>
#include <lintel/core/shm/ShmObjectHeader.h>
#include <lintel/features/base/services/ShmConstructors.h>

#include <string>
#include <vector>

namespace lintel {
namespace {
  /** Layout marker of the objects stored inside a segment.
   *
   * Bumped whenever the in-segment layout of a lintel object changes (the
   * ShmSegmentAccessor grew from one pointer to a named 40 byte value in 0.2,
   * for example). A segment written by an older build is recreated instead of
   * being reinterpreted with the new layout, which would corrupt it silently.
   *
   * Version 3 introduced the per-object ShmObjectHeader. Objects written by
   * earlier builds have no readable header and are therefore still treated as
   * authoritative unless the operator opts in.
   */
  constexpr std::uint64_t kSegmentLayoutVersion = 3;
  constexpr char kSegmentLayoutName[] = "lintel_segment_layout";

  /** What a stale segment contains, as far as the stable object headers say. */
  struct SegmentContent
  {
    /** number of named objects besides the layout marker */
    std::size_t objects = 0;
    /** true when every object has a readable header and is Volatile/Derived */
    bool allDiscardable = true;
    /** names that prevent an automatic discard, for the error message */
    std::vector<std::string> blocking;
  };

  /** Inspect the named objects of a mapped segment.
   *
   * Only the stable ShmObjectHeader is read; the payload types are irrelevant
   * for the decision. An object without a compatible header (a segment written
   * before version 3) is unknown and therefore blocks an automatic discard.
   *
   * @param file mapped segment
   * @return summary used for the persistence-class decision */
  SegmentContent inspectContent(boost::interprocess::managed_mapped_file &file)
  {
    SegmentContent content;
    auto *manager = file.get_segment_manager();
    for (auto iterator = manager->named_begin(); iterator != manager->named_end(); ++iterator) {
      const std::string name(iterator->name());
      if (name == kSegmentLayoutName) { continue; }
      ++content.objects;
      // The named index does not hand out the allocation size, so ask the
      // segment manager: a pre-header object can be smaller than the header and
      // must not be read past its end.
      const auto *bytes = static_cast<const char *>(iterator->value());
      const auto length = manager->get_instance_length(bytes);
      if (bytes == nullptr || length < sizeof(shm::ShmObjectHeader)) {
        content.allDiscardable = false;
        content.blocking.push_back(name + " (no object header)");
        continue;
      }
      const auto *header = static_cast<const shm::ShmObjectHeader *>(iterator->value());
      if (header == nullptr) {
        content.allDiscardable = false;
        content.blocking.push_back(name + " (no object header)");
        continue;
      }
      if (!header->isCompatible() || !header->isReady()) {
        content.allDiscardable = false;
        content.blocking.push_back(name + " (unreadable object header)");
        continue;
      }
      if (header->persistenceClass() == shm::ShmPersistence::Persistent) {
        content.allDiscardable = false;
        content.blocking.push_back(name + " (persistent)");
      }
    }
    return content;
  }

  /** @param file mapped segment
   *  @return the layout version stored in the segment; empty for a segment that
   *          was never used, 0 for one written before the marker existed */
  std::optional<std::uint64_t> layoutVersionOf(boost::interprocess::managed_mapped_file &file)
  {
    const auto found = file.find<std::uint64_t>(kSegmentLayoutName);
    if (found.first != nullptr) { return *found.first; }
    // No marker: either a segment that was never used, or one written before
    // the marker existed (which always has named objects by then).
    if (file.get_num_named_objects() == 0) { return std::nullopt; }
    return 0;
  }

  /** @return true when the operator allowed a segment with an older layout to be discarded
   *
   * Recreating a segment throws its contents away, and only the operator can
   * decide whether that is acceptable. The default therefore refuses and names
   * the file, so data loss never happens silently.
   */
  bool recreateAllowed()
  {
    const char *value = std::getenv("LINTEL_SHM_RECREATE");
    if (value == nullptr) { return false; }
    const std::string_view flag(value);
    return flag == "1" || flag == "true" || flag == "TRUE" || flag == "yes" || flag == "YES";
  }

  /** Write the layout marker into a freshly created segment. */
  void stampLayout(boost::interprocess::managed_mapped_file &file)
  {
    if (file.find<std::uint64_t>(kSegmentLayoutName).first != nullptr) { return; }
    file.construct<std::uint64_t>(kSegmentLayoutName)(kSegmentLayoutVersion);
  }
}// namespace

SharedMemoryService::SharedMemoryService(const std::shared_ptr<SharedMemorySegmentManager> &sharedMemorySegmentManager,
  std::shared_ptr<SchedulerService> schedulerService,
  const std::shared_ptr<ProcessName> &processName)
  : PropertyRegistration(processName->getProcessName()), m_segments(create(sharedMemorySegmentManager->allOf())),
    m_schedulerService(std::move(schedulerService))
{
  m_scheduleRate = registerProperty<std::chrono::seconds>(
    "m_scheduleRate", std::chrono::seconds(2), "schedule rate of user tokens checks in seconds", true);
  m_autoExtend = registerProperty<bool>("m_autoExtend", true, "auto extend shared memory", true);
  m_autoExtendEpsilon = registerProperty<std::size_t>(
    "m_autoExtendEpsilon", static_cast<std::size_t>(1000), "epsilon when auto extend gets triggered", true);
}

void SharedMemoryService::growOf(const std::shared_ptr<SharedMemorySegment> &segment, std::size_t grow)
{
  notifyHooks(segment->getName(), SegmentLifecycle::BeforeGrow);
  std::lock_guard<std::mutex> lock(m_segmentsMutex);
  auto found = m_segments.find(segment->getName());
  if (found == m_segments.end()) { throw ShmSegmentNotFound(segment->getName()); }
  if (grow == 0) {
    notifyHooks(segment->getName(), SegmentLifecycle::AfterGrow);
    return;
  }
  const std::size_t maxSize = segment->getMaxSize();
  if (maxSize != 0) {
    const std::size_t currentSize = found->second->m_managedMappedFile->get_size();
    if (currentSize >= maxSize) {
      throw std::runtime_error("shared memory segment '" + segment->getName() + "' already at its maximum size");
    }
    // cap the request to the remaining headroom to avoid overflow and
    // to respect the configured resource limit
    const std::size_t headroom = maxSize - currentSize;
    if (grow > headroom) { grow = headroom; }
  }
  // The static grow() extends the file and updates the segment-manager header
  // stored inside the file. The live mapping's length, however, is fixed at
  // mmap() time, so it must be reopened to observe the larger file.
  // Hold the cross-process remap semaphore so no other process references the
  // segment (find_or_construct / getSegmentAccessor) while it is being resized.
  shm::SegmentSemaphoreGuard gate(*found->second);
  // boost::interprocess takes a narrow file name; std::filesystem::path::c_str()
  // is wchar_t on Windows, so the conversion has to be explicit.
  const std::string filePath = segment->getPath().string();
  if (!boost::interprocess::managed_mapped_file::grow(filePath.c_str(), grow)) {
    throw std::runtime_error("failed to grow shared memory segment '" + segment->getName() + "'");
  }
  // Reopen the segment so the new mapping covers the grown file. This
  // invalidates references into the old mapping - repositories detect the
  // bump of the generation counter and re-fetch their references.
  found->second->m_segmentAllocator.reset();
  found->second->m_managedMappedFile =
    std::make_shared<boost::interprocess::managed_mapped_file>(boost::interprocess::open_only, filePath.c_str());
  ++m_generation;
  m_growCount[segment->getName()]++;
  notifyHooks(segment->getName(), SegmentLifecycle::OnRemap);
  notifyHooks(segment->getName(), SegmentLifecycle::AfterGrow);
}

void SharedMemoryService::shrinkOf(const std::shared_ptr<SharedMemorySegment> &segment)
{
  notifyHooks(segment->getName(), SegmentLifecycle::BeforeShrink);
  std::lock_guard<std::mutex> lock(m_segmentsMutex);
  auto found = m_segments.find(segment->getName());
  if (found == m_segments.end()) { throw ShmSegmentNotFound(segment->getName()); }
  // Hold the cross-process remap semaphore so no other process references the
  // segment while it is being resized and reopened (see growOf).
  shm::SegmentSemaphoreGuard gate(*found->second);
  const std::string filePath = segment->getPath().string();
  // Windows refuses to truncate a file while a view of it is still mapped ("The
  // requested operation cannot be performed on a file with a user-mapped section
  // open"), so our own mapping has to go before the file is shrunk.
  found->second->m_segmentAllocator.reset();
  found->second->m_managedMappedFile.reset();
  const bool shrunk = boost::interprocess::managed_mapped_file::shrink_to_fit(filePath.c_str());
  // Reopen the segment so the new mapping covers the shrunk file; this also restores a
  // usable mapping when shrinking failed.
  found->second->m_managedMappedFile =
    std::make_shared<boost::interprocess::managed_mapped_file>(boost::interprocess::open_only, filePath.c_str());
  if (!shrunk) { throw std::runtime_error("failed to shrink shared memory segment '" + segment->getName() + "'"); }
  ++m_generation;
  m_growCount[segment->getName()]++;
  notifyHooks(segment->getName(), SegmentLifecycle::OnRemap);
  notifyHooks(segment->getName(), SegmentLifecycle::AfterGrow);
}

std::size_t SharedMemoryService::getGeneration() const
{
  std::lock_guard<std::mutex> lock(m_segmentsMutex);
  return m_generation;
}

SharedMemorySegmentInfo SharedMemoryService::showStateOf(const std::shared_ptr<SharedMemorySegment> &segment) const
{
  std::lock_guard<std::mutex> lock(m_segmentsMutex);
  auto found = m_segments.find(segment->getName());
  if (found == m_segments.end()) { throw ShmSegmentNotFound(segment->getName()); }
  std::size_t currentSize = found->second->m_managedMappedFile->get_size();
  std::size_t freeSize = found->second->m_managedMappedFile->get_free_memory();
  std::size_t namedObjects = found->second->m_managedMappedFile->get_num_named_objects();
  std::size_t uniqueObjects = found->second->m_managedMappedFile->get_num_unique_objects();
  bool sanity = found->second->m_managedMappedFile->check_sanity();

  SharedMemorySegmentInfo info(segment, currentSize, freeSize, namedObjects, uniqueObjects, sanity);
  return info;
}

std::map<std::string, std::shared_ptr<SharedMemorySegmentHandle>> SharedMemoryService::create(
  const std::vector<std::shared_ptr<SharedMemorySegment>> &set)
{
  std::map<std::string, std::shared_ptr<SharedMemorySegmentHandle>> map;
  std::transform(set.begin(),
    set.end(),
    std::inserter(map, map.end()),
    [](const std::shared_ptr<SharedMemorySegment> &segment)
      -> std::pair<std::string, std::shared_ptr<SharedMemorySegmentHandle>> {
      auto handle = std::make_shared<SharedMemorySegmentHandle>();
      const std::string filePath = segment->getPath().string();
      handle->m_managedMappedFile = std::make_shared<boost::interprocess::managed_mapped_file>(
        boost::interprocess::open_or_create, filePath.c_str(), segment->getSize());
      const std::optional<std::uint64_t> storedVersion = layoutVersionOf(*handle->m_managedMappedFile);
      if (storedVersion.has_value() && *storedVersion != kSegmentLayoutVersion) {
        // The layout marker covers the library's bookkeeping. On top of that,
        // every object carries its own persistence class. A segment whose
        // objects are all Volatile or Derived may be recreated without asking
        // the operator; a Persistent object (or one without a readable header,
        // i.e. written before object headers existed) keeps the old fail-safe
        // behaviour.
        // See docs/shm-payload-abi-plan.md.
        bool mayDiscard = false;
        const SegmentContent content = inspectContent(*handle->m_managedMappedFile);
        if (shm::SegmentLayoutPolicy::hasMigrator()) {
          LOG_WARN(
            "shared memory segment '{}' was written with layout {} (current {}) - asking the registered "
            "migrator for '{}'",
            segment->getName(),
            *storedVersion,
            kSegmentLayoutVersion,
            filePath);
          mayDiscard = shm::SegmentLayoutPolicy::migrate(segment->getPath(), *storedVersion, kSegmentLayoutVersion);
          if (!mayDiscard) {
            throw std::runtime_error("the migrator refused to hand over shared memory segment '" + segment->getName()
                                     + "' at '" + filePath + "'");
          }
        } else if (*storedVersion < kSegmentLayoutVersion && content.allDiscardable) {
          mayDiscard = true;
          LOG_WARN("recreating shared memory segment '{}' at '{}': all {} object(s) are volatile or derived",
            segment->getName(),
            filePath,
            content.objects);
        } else if (shm::SegmentLayoutPolicy::policy() == shm::LayoutMismatchPolicy::Recreate || recreateAllowed()) {
          mayDiscard = true;
        }
        if (!mayDiscard) {
          std::string blocking;
          for (const std::string &name : content.blocking) {
            if (!blocking.empty()) { blocking.append(", "); }
            blocking.append(name);
          }
          throw std::runtime_error("shared memory segment '" + segment->getName() + "' at '" + filePath
                                   + "' was written with an older object layout and cannot be opened"
                                   + (blocking.empty() ? std::string() : " (blocking objects: " + blocking + ")")
                                   + ". Delete the file to start with an empty segment, register a migrator "
                                     "(lintel::shm::SegmentLayoutPolicy) or set LINTEL_SHM_RECREATE=1 to have it "
                                     "discarded automatically.");
        }
        LOG_WARN(
          "shared memory segment '{}' was written with an older object layout - recreating '{}' "
          "(its contents are discarded, not migrated)",
          segment->getName(),
          filePath);
        handle->m_managedMappedFile.reset();
        std::error_code removeError;
        std::filesystem::remove(segment->getPath(), removeError);
        handle->m_managedMappedFile = std::make_shared<boost::interprocess::managed_mapped_file>(
          boost::interprocess::open_or_create, filePath.c_str(), segment->getSize());
      }
      stampLayout(*handle->m_managedMappedFile);
      // Holder record of the remap semaphore, plus the automatic recovery: a
      // previous run that crashed while holding the lock is visible here as a
      // stuck semaphore whose recorded holder is gone. The record is created
      // first and the semaphore opened afterwards, so after a reclaim this
      // process maps the new semaphore instead of the orphaned one.
      handle->m_lockOwner = std::make_shared<SegmentLockOwner>(segment->getName());
      const bool reclaimed =
        shm::recoverOrphanedSegmentLock(segment->getName(), segment->getPath(), *handle->m_lockOwner);
      handle->m_remapSemaphore = std::make_unique<boost::interprocess::named_semaphore>(
        boost::interprocess::open_or_create, shm::remapSemaphoreNameOf(segment->getName()).c_str(), 1u);
      if (reclaimed) {
        LOG_WARN(
          "reclaimed the orphaned remap semaphore of shared memory segment '{}' - "
          "the process that held it is gone",
          segment->getName());
      }
      handle->m_sharedMemorySegment = segment;
      return std::make_pair(segment->getName(), handle);
    });
  return map;
}

std::size_t SharedMemoryService::recover()
{
  std::lock_guard<std::mutex> lock(m_segmentsMutex);
  std::size_t reclaimed = 0;
  for (const auto &[name, handle] : m_segments) {
    if (handle == nullptr || handle->m_lockOwner == nullptr) { continue; }
    if (handle->m_sharedMemorySegment == nullptr) { continue; }
    if (!shm::recoverOrphanedSegmentLock(name, handle->m_sharedMemorySegment->getPath(), *handle->m_lockOwner)) {
      continue;
    }
    // The old semaphore object is orphaned by the removal; reopen it so this
    // process keeps using the one every other process will see as well.
    try {
      handle->m_remapSemaphore = std::make_unique<boost::interprocess::named_semaphore>(
        boost::interprocess::open_or_create, shm::remapSemaphoreNameOf(name).c_str(), 1u);
    } catch (const boost::interprocess::interprocess_exception &error) {
      // Nothing else to do: the next open of the segment maps the new object.
      LOG_ERROR("cannot reopen the reclaimed remap semaphore of segment '{}': {}", name, error.what());
      continue;
    }
    LOG_WARN(
      "reclaimed the orphaned remap semaphore of shared memory segment '{}' - "
      "the process that held it is gone",
      name);
    m_recoveryCount[name]++;
    ++reclaimed;
  }
  return reclaimed;
}

SharedMemorySegmentHandle &SharedMemoryService::segmentOf(const std::string &name)
{
  auto found = m_segments.find(name);
  if (found == m_segments.end()) { throw ShmSegmentNotFound(name); }
  return *found->second;
}

ShmSegmentAccessor SharedMemoryService::getSegmentAccessor(const std::shared_ptr<SharedMemorySegment> &segment)
{
  std::lock_guard<std::mutex> lock(m_segmentsMutex);
  auto &handle = segmentOf(segment->getName());
  shm::SegmentSemaphoreGuard gate(handle);
  if (handle.m_segmentAllocator == nullptr) {
    handle.m_segmentAllocator =
      std::make_shared<BoostSegmentAllocator>(*handle.m_managedMappedFile->get_segment_manager());
  }
  // Bind the name to *this* process' allocator and hand out an accessor that
  // carries the name: a copy of it that is stored inside the segment resolves
  // to the allocator of whichever process reads it, instead of pointing at the
  // heap of the process that created the object (see ShmSegmentAccessor).
  ShmSegmentAccessor accessor(segment->getName(), *handle.m_segmentAllocator);
  ShmSegmentAccessor::bind(segment->getName(), *handle.m_segmentAllocator);
  return accessor;
}

void SharedMemoryService::onInitialize()
{
  std::weak_ptr<SharedMemoryService> weakSelf = shared_from_this();
  // `GET /ready` reports the segments this service mapped: a segment that is
  // gone (file removed, mapping closed) makes the process unable to serve the
  // data structures it stores there. The weak_ptr keeps the registry free of
  // the service lifetime; an expired reference means the service was destroyed
  // and the process is shutting down anyway.
  ReadinessRegistry::instance().registerCheck("shared_memory", [weakSelf](std::string &detail) {
    const std::shared_ptr<SharedMemoryService> self = weakSelf.lock();
    if (self == nullptr) {
      detail = "shared memory service is not running";
      return false;
    }
    std::lock_guard<std::mutex> lock(self->m_segmentsMutex);
    for (const auto &entry : self->m_segments) {
      if (entry.second == nullptr || entry.second->m_managedMappedFile == nullptr) {
        detail = "segment '" + entry.first + "' is not mapped";
        return false;
      }
    }
    return true;
  });
  m_schedulerService->schedule_at_fixed_rate(m_scheduleRate->getValue(), m_scheduleRate->getValue(), [weakSelf]() {
    if (auto self = weakSelf.lock()) { self->onCheck(); }
  });
}

void SharedMemoryService::onCheck()
{
  if (!m_autoExtend->getValue()) { return; }
  std::vector<std::shared_ptr<SharedMemorySegment>> segments;
  {
    std::lock_guard<std::mutex> lock(m_segmentsMutex);
    segments.reserve(m_segments.size());
    for (const auto &entry : m_segments) { segments.push_back(entry.second->m_sharedMemorySegment); }
  }
  for (const auto &segment : segments) {
    if (!segment->isAutoExtend()) { continue; }
    std::size_t freeMemory = 0;
    std::size_t currentSize = 0;
    try {
      std::lock_guard<std::mutex> lock(m_segmentsMutex);
      auto found = m_segments.find(segment->getName());
      if (found == m_segments.end()) { continue; }
      freeMemory = found->second->m_managedMappedFile->get_free_memory();
      currentSize = found->second->m_managedMappedFile->get_size();
    } catch (const std::exception &exception) {
      LOG_ERROR("{}: reading state failed: {}", segment->getName(), exception.what());
      continue;
    }
    if (freeMemory < m_autoExtendEpsilon->getValue()) {
      const auto maxSize = segment->getMaxSize();
      const auto autoExtendSize = segment->getAutoExtendSize();
      // compute the grow size without overflowing the addition in
      // `currentSize + autoExtendSize`
      std::size_t grow = autoExtendSize;
      if (maxSize != 0) {
        if (currentSize >= maxSize) {
          LOG_WARN("{}: segment reached its maximum size {}/{}", segment->getName(), currentSize, maxSize);
          continue;
        }
        const std::size_t headroom = maxSize - currentSize;
        if (grow == 0 || grow > headroom) { grow = headroom; }
      }
      if (grow == 0) { continue; }
      try {
        growOf(segment, grow);
        LOG_INFO("{}: extend current {}/{} -> increase by {}",
          segment->getName(),
          currentSize - freeMemory,
          currentSize,
          grow);
      } catch (const std::exception &exception) {
        LOG_ERROR("{}: auto extend failed: {}", segment->getName(), exception.what());
      }
    }
  }
}
}// namespace lintel

namespace lintel {
void SharedMemoryService::notifyHooks(const std::string &segmentName, SegmentLifecycle action)
{
  std::lock_guard<std::mutex> lock(m_hooksMutex);
  for (const auto &hook : m_lifecycleHooks) {
    try {
      hook(segmentName, action);
    } catch (const std::exception &exception) {
      // A misbehaving lifecycle hook must not take the service down, but it
      // must not be invisible either.
      LOG_WARN("shared memory lifecycle hook for '{}' failed: {}", segmentName, exception.what());
    } catch (...) {
      LOG_WARN("shared memory lifecycle hook for '{}' failed with an unknown exception", segmentName);
    }
  }
}

void SharedMemoryService::registerLifecycleHook(SegmentLifecycleHook hook)
{
  std::lock_guard<std::mutex> lock(m_hooksMutex);
  m_lifecycleHooks.push_back(std::move(hook));
}

SharedMemorySegmentStats SharedMemoryService::statsOf(const std::shared_ptr<SharedMemorySegment> &segment) const
{
  std::lock_guard<std::mutex> lock(m_segmentsMutex);
  auto found = m_segments.find(segment->getName());
  if (found == m_segments.end()) { throw ShmSegmentNotFound(segment->getName()); }
  SharedMemorySegmentStats stats;
  stats.name = segment->getName();
  stats.allocatedBytes = found->second->m_managedMappedFile->get_size();
  stats.freeBytes = found->second->m_managedMappedFile->get_free_memory();
  stats.capacityBytes = segment->getMaxSize() == 0 ? stats.allocatedBytes : segment->getMaxSize();
  stats.growCount = m_growCount.count(segment->getName()) ? m_growCount.at(segment->getName()) : 0;
  stats.shrinkCount = m_shrinkCount.count(segment->getName()) ? m_shrinkCount.at(segment->getName()) : 0;
  stats.recoveryCount = m_recoveryCount.count(segment->getName()) ? m_recoveryCount.at(segment->getName()) : 0;
  stats.generation = m_generation;
  return stats;
}

}// namespace lintel
