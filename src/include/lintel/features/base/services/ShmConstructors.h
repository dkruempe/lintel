#ifndef LINTEL_SHMCONSTRUCTORS_H
#define LINTEL_SHMCONSTRUCTORS_H

#include <boost/container/map.hpp>
#include <boost/container/string.hpp>
#include <boost/container/vector.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include "lintel/core/models/SharedMemorySegment.h"
#include "lintel/core/platform/Platform.h"
#include "lintel/core/shm/ShmSegmentAccessor.h"
#include "lintel/features/base/services/SharedMemoryService.h"

namespace lintel {
/**
 * Sidecar of a shared memory segment that records which process currently holds
 * that segment's remap semaphore.
 *
 * The semaphore itself only has a binary state, so once it is stuck the holder
 * leaves no trace: "a previous process crashed while holding it" and "a live
 * process still works on it" are indistinguishable. Reclaiming in the second
 * case would hand out two independent mutexes for one critical section, so the
 * holder records its process id and start ticks here while it holds the lock and
 * clears the record again before releasing. `shm::recoverOrphanedSegmentLock()`
 * combines that with `processIdentityIsAlive()` and only reclaims when the
 * holder is provably gone.
 *
 * The name suffix has the same length as the semaphore's (`_remap_own` vs
 * `_remap_sem`): macOS caps named object names at 31 characters (PSEMNAMLEN) and
 * both have to fit the same segment names.
 */
class SegmentLockOwner
{
public:
  static constexpr std::string_view kNameSuffix = "_remap_own";

  /** Open (or create) the sidecar of a segment.
   * @param segmentName the segment the sidecar belongs to
   * @throws std::runtime_error if the sidecar cannot be mapped */
  explicit SegmentLockOwner(const std::string &segmentName) : m_name(segmentName + std::string(kNameSuffix))
  {
    try {
      boost::interprocess::shared_memory_object object(
        boost::interprocess::open_or_create, m_name.c_str(), boost::interprocess::read_write);
      object.truncate(static_cast<std::size_t>(kCellCount) * sizeof(std::int64_t));
      m_region = std::make_unique<boost::interprocess::mapped_region>(object, boost::interprocess::read_write);
    } catch (const boost::interprocess::interprocess_exception &error) {
      throw std::runtime_error(
        "cannot map the lock owner sidecar of shared memory segment '" + segmentName + "': " + error.what());
    }
  }

  ~SegmentLockOwner() = default;

  SegmentLockOwner(const SegmentLockOwner &) = delete;
  SegmentLockOwner &operator=(const SegmentLockOwner &) = delete;

  /** @return the recorded holder; processId is 0 when no holder is recorded */
  [[nodiscard]] ProcessIdentity identity() const noexcept
  {
    return ProcessIdentity{ .processId = std::atomic_ref<std::int64_t>(*cell(kPidCell)).load(std::memory_order_acquire),
      .startTicks = std::atomic_ref<std::int64_t>(*cell(kStartTicksCell)).load(std::memory_order_acquire) };
  }

  /** @return the id of the recorded holder, 0 when no holder is recorded */
  [[nodiscard]] ProcessId pid() const noexcept { return identity().processId; }

  /** Record a process as the current holder of the lock.
   * @param processId the holder, 0 clears the record */
  void set(ProcessId processId) noexcept
  {
    set(ProcessIdentity{ .processId = processId, .startTicks = processStartTicksOf(processId) });
  }

  /** Record a process as the current holder of the lock.
   * @param identity the holder, a processId of 0 clears the record */
  void set(const ProcessIdentity &identity) noexcept
  {
    // The start ticks belong to the id: a process id is recycled eventually, so
    // the pair is what tells "the recorded holder" from "some later process".
    // Ticks are stored first, so a reader that sees the new id also sees them.
    std::atomic_ref<std::int64_t>(*cell(kStartTicksCell)).store(identity.startTicks, std::memory_order_release);
    std::atomic_ref<std::int64_t>(*cell(kPidCell)).store(identity.processId, std::memory_order_release);
  }

  /** Forget the recorded holder (called right before the lock is released). */
  void clear() noexcept { set(0); }

  /**
   * Forget the recorded holder only when it is still the given process.
   *
   * The record changes hands: the next holder writes its own id as soon as it
   * has the lock. Clearing unconditionally would then wipe a *live* holder's
   * record, and the next recovery would reclaim a lock that is in use. The id
   * alone is not enough either: an id is recycled eventually, so a record is
   * only the caller's own while the start ticks match as well.
   *
   * @param identity the process that wants to drop its own record
   * @return true if the record still belonged to that process and is cleared
   */
  bool clearIf(const ProcessIdentity &identity) noexcept
  {
    if (identity.processId <= 0) { return false; }
    auto startTicksCell = std::atomic_ref<std::int64_t>(*cell(kStartTicksCell));
    // The ticks are stored before the id, so they are the first half of a
    // record: a mismatch means the record is empty or already belongs to
    // another process - which may carry the same, recycled id, so the id alone
    // must not decide here.
    if (startTicksCell.load(std::memory_order_acquire) != identity.startTicks) { return false; }
    auto pidCell = std::atomic_ref<std::int64_t>(*cell(kPidCell));
    std::int64_t expected = identity.processId;
    if (!pidCell.compare_exchange_strong(expected, 0, std::memory_order_acq_rel)) { return false; }
    // Drop the ticks as well, but only while they are still ours: a new holder
    // stores its ticks first, so the cell may already hold the next record.
    std::int64_t ticks = identity.startTicks;
    static_cast<void>(startTicksCell.compare_exchange_strong(ticks, 0, std::memory_order_acq_rel));
    return true;
  }

  /** @return the sidecar name a segment is stored under */
  static std::string nameOf(const std::string &segmentName) { return segmentName + std::string(kNameSuffix); }

  /** Remove the sidecar of a segment. Teardown paths (tests, explicit cleanup)
   *  use it so a dead process id does not linger in /dev/shm. */
  static void removeOf(const std::string &segmentName)
  {
    boost::interprocess::shared_memory_object::remove(nameOf(segmentName).c_str());
  }

private:
  /** One cell holds the holder's process id, the other its start ticks. */
  static constexpr std::size_t kCellCount = 2;
  static constexpr std::size_t kPidCell = 0;
  static constexpr std::size_t kStartTicksCell = 1;

  [[nodiscard]] std::int64_t *cell(std::size_t index) const noexcept
  {
    return static_cast<std::int64_t *>(m_region->get_address()) + index;
  }

  std::string m_name;
  std::unique_ptr<boost::interprocess::mapped_region> m_region;
};

/**
 * Allocated state of one mapped shared memory segment. The type is defined
 * here, and only here, because it exposes the boost interprocess types;
 * SharedMemoryService.h merely forward declares it. That keeps the boost
 * headers out of the public service header.
 */
class SharedMemorySegmentHandle
{
public:
  std::shared_ptr<boost::interprocess::managed_mapped_file> m_managedMappedFile;
  std::shared_ptr<SharedMemorySegment> m_sharedMemorySegment;
  // Cross-process binary semaphore guarding the segment's remap and
  // construct critical sections. Acquired (as a mutex) around grow/shrink
  // and every find_or_construct so no process references the mapped file
  // while another process resizes/reopens it.
  std::unique_ptr<boost::interprocess::named_semaphore> m_remapSemaphore;
  // Which process holds m_remapSemaphore, see SegmentLockOwner. Created
  // together with the semaphore, so a recovery can tell a crashed holder
  // from a live-but-slow one.
  std::shared_ptr<SegmentLockOwner> m_lockOwner;
  // Typed allocator adapter of this segment. Created on demand and reset
  // whenever the mapping is replaced, so accessors always refer to the
  // segment manager of the current mapping. Accessors handed out by
  // SharedMemoryService::getSegmentAccessor() carry the segment name and bind
  // this allocator to it (ShmSegmentAccessor::bind), which is what makes a
  // copy of an accessor inside the segment usable from another process.
  std::shared_ptr<ShmSegmentAccessor::Allocator> m_segmentAllocator;
};

namespace shm {

  /** Segment manager of a mapped shared memory file. */
  using SegmentManager = boost::interprocess::managed_mapped_file::segment_manager;

  /** Allocator for data structures placed in shared memory. */
  template<class Object> using SegmentAllocator = boost::interprocess::allocator<Object, SegmentManager>;

  /** String placed in shared memory. */
  using String = boost::container::basic_string<char, std::char_traits<char>, SegmentAllocator<char>>;

  /** Map placed in shared memory. */
  template<class Key, class Value>
  using Map = boost::container::map<Key, Value, std::less<Key>, SegmentAllocator<std::pair<const Key, Value>>>;

  /** Vector placed in shared memory. */
  template<class Object> using Vector = boost::container::vector<Object, SegmentAllocator<Object>>;

  /** RAII guard that acquires (default) the segment remap semaphore and
   *  releases it on destruction. Used as a cross-process mutex around the
   *  shared memory remap / construct critical sections.
   *
   *  The acquisition is bounded: a process that crashes while holding the
   *  semaphore leaves its count at zero forever, so an unbounded `wait()`
   *  would deadlock every other process. Instead the guard times out and
   *  throws, turning the silent hang into an actionable error. The holder's
   *  process id is recorded in the segment's SegmentLockOwner while the lock
   *  is held, which is what lets `SharedMemoryService::recover()` decide
   *  whether the stuck lock may be reclaimed. */
  class SegmentSemaphoreGuard
  {
  public:
    /** Default time to wait for the semaphore before giving up. Generous,
     *  because the guarded critical sections are short (remap/construct). */
    static constexpr std::chrono::seconds kDefaultWaitTimeout{ 30 };

    /** Guard a segment's lock and record this process as its holder.
     *  @param handle  the segment handle whose lock is taken
     *  @param timeout upper bound for acquiring the semaphore
     *  @throws std::runtime_error if the semaphore could not be acquired
     *          within the timeout (e.g. a previous holder crashed) */
    explicit SegmentSemaphoreGuard(SharedMemorySegmentHandle &handle,
      std::chrono::steady_clock::duration timeout = kDefaultWaitTimeout)
      : m_semaphore(handle.m_remapSemaphore.get()), m_owner(handle.m_lockOwner.get())
    {
      acquireOrThrow(timeout);
    }

    /** Guard a bare semaphore; no holder is recorded, so a stuck lock cannot be
     *  recovered automatically afterwards.
     *  @param semaphore semaphore to guard
     *  @param timeout   upper bound for acquiring the semaphore
     *  @throws std::runtime_error if the semaphore could not be acquired
     *          within the timeout (e.g. a previous holder crashed) */
    explicit SegmentSemaphoreGuard(boost::interprocess::named_semaphore &semaphore,
      std::chrono::steady_clock::duration timeout = kDefaultWaitTimeout)
      : m_semaphore(&semaphore), m_owner(nullptr)
    {
      acquireOrThrow(timeout);
    }

    ~SegmentSemaphoreGuard()
    {
      // A timed-out guard never took the semaphore (acquireOrThrow cleared it):
      // it must not post on destruction - that would hand the lock to the next
      // waiter even though we never owned it.
      if (m_semaphore == nullptr) { return; }
      // clearIf, not clear: the next holder writes its own record as soon as it
      // owns the lock, so an unconditional clear could wipe a live holder and
      // make the next recovery reclaim a lock that is in use.
      if (m_owner != nullptr) { m_owner->clearIf(m_holderIdentity); }
      m_semaphore->post();
    }

    SegmentSemaphoreGuard(const SegmentSemaphoreGuard &) = delete;

    SegmentSemaphoreGuard &operator=(const SegmentSemaphoreGuard &) = delete;

  private:
    void acquireOrThrow(std::chrono::steady_clock::duration timeout)
    {
      const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(timeout).count();
      const auto deadline =
        boost::posix_time::microsec_clock::universal_time() + boost::posix_time::microseconds(microseconds);
      if (!m_semaphore->timed_wait(deadline)) {
        m_semaphore = nullptr;
        throw std::runtime_error(
          "timed out acquiring the shared memory segment remap semaphore; "
          "a previous process may have crashed while holding it "
          "(SharedMemoryService::recover() reclaims it once the holder is gone)");
      }
      if (m_owner != nullptr) {
        const ProcessId holderPid = currentProcessId();
        m_holderIdentity = ProcessIdentity{ .processId = holderPid, .startTicks = processStartTicksOf(holderPid) };
        m_owner->set(m_holderIdentity);
      }
    }

    boost::interprocess::named_semaphore *m_semaphore;
    SegmentLockOwner *m_owner;
    /** The identity recorded in m_owner, so the release can check that the
     *  record is still ours (by id *and* start ticks) before clearing it. */
    ProcessIdentity m_holderIdentity;
  };

  /** Name of a segment's remap semaphore. Single source so the recovery, the
   *  removal and the reopen cannot drift apart. */
  inline std::string remapSemaphoreNameOf(const std::string &segmentName) { return segmentName + "_remap_sem"; }

  /** How long the recovery waits before it decides that an unrecorded holder is
   *  a crashed one - see recoverOrphanedSegmentLock(). */
  inline constexpr std::chrono::milliseconds kRecoveryProbeDelay{ 200 };

  /** Upper bound for the recovery mutex. The mutex only serializes the
   *  destructive reclaim itself, so a holder that does not release it within
   *  this bound is a fault the recovery must not stall on - it gives up and
   *  leaves the lock alone (fail-safe). */
  inline constexpr std::chrono::seconds kRecoveryLockTimeout{ 10 };

  /**
   * Reclaim a segment's stuck remap semaphore when the process that held it is gone.
   *
   * Removing the semaphore while a live process still holds it would give two
   * processes independent locks for one critical section, so this refuses to act
   * unless it can rule that out:
   *
   * - The recovery itself is destructive (remove + recreate), so concurrent runs
   *   are serialized with a time bounded file lock on the segment file. Two
   *   recoveries that each removed the semaphore the other had just created
   *   would both keep a lock of their own. A file lock is released by the kernel
   *   when its holder dies, so this mutex cannot become an orphan itself.
   * - A recorded holder that is still alive (compared by process id *and* start
   *   time, because ids are recycled) means "leave it alone".
   * - The semaphore is probed with a non-blocking acquire first: a free lock has
   *   nothing to recover, and recreating it would orphan the handles of every
   *   other process, which would then wait on an object nobody else uses.
   * - The lock can be taken while the record is still empty: a live holder
   *   records itself right after the acquisition. A re-check after
   *   kRecoveryProbeDelay closes that window. It reads the record *fresh*, from
   *   the shared memory cells: the record read before the probe may meanwhile
   *   have been replaced by a process that took the stuck lock over, and a
   *   re-check against that stale read would reclaim a lock that is in use.
   *
   * A reclaimed lock lets every waiter continue; a process that already opened
   * the removed semaphore still talks to the orphaned one and will time out with
   * the usual error instead of racing.
   *
   * @param segmentName the segment whose lock may be stuck
   * @param segmentPath the segment file, used as the recovery mutex
   * @param owner       the segment's holder record
   * @param probeDelay  how long to wait before the holder record is re-read;
   *                    tests widen it to make the re-check window deterministic
   * @return true if the lock was reclaimed
   */
  inline bool recoverOrphanedSegmentLock(const std::string &segmentName,
    const std::filesystem::path &segmentPath,
    SegmentLockOwner &owner,
    std::chrono::milliseconds probeDelay = kRecoveryProbeDelay)
  {
    const std::string semaphoreName = remapSemaphoreNameOf(segmentName);
    // Tracks whether the old semaphore is already gone, so a failure *after*
    // the removal is still reported as a reclaim (the caller then has to reopen
    // the lock, even though the recreate failed).
    bool semaphoreRemoved = false;
    try {
      const std::string lockPath = segmentPath.string();
      boost::interprocess::file_lock recoveryLock(lockPath.c_str());
      // Bounded wait: hitting the bound means the recovery mutex is held
      // without being released, which a file lock does not do while its holder
      // lives. Give up instead of stalling - not reclaiming is always the
      // fail-safe direction. scoped_lock keeps the RAII release for the case
      // that is acquired.
      const boost::posix_time::ptime lockDeadline =
        boost::posix_time::microsec_clock::universal_time() + boost::posix_time::seconds(kRecoveryLockTimeout.count());
      boost::interprocess::scoped_lock<boost::interprocess::file_lock> serialized(recoveryLock, lockDeadline);
      if (!serialized.owns()) {
        LOG_WARN("giving up the lock recovery of shared memory segment '{}': the recovery mutex stayed busy for {}s",
          segmentName,
          kRecoveryLockTimeout.count());
        return false;
      }

      // Fast path: a living holder means "leave the lock alone", checked
      // without touching the semaphore. This snapshot is deliberately *not*
      // used for the re-check below - the record may change hands meanwhile.
      const ProcessIdentity snapshot = owner.identity();
      if (snapshot.processId > 0 && processIdentityIsAlive(snapshot)) { return false; }

      // open_or_create: the semaphore may not exist yet when a segment is
      // opened for the first time, and this probe must not fail on that.
      boost::interprocess::named_semaphore lock(boost::interprocess::open_or_create, semaphoreName.c_str(), 1u);
      if (lock.try_wait()) {
        // The lock is free: nothing to recover. Hand it back untouched and drop
        // the stale record of a holder that died after it released the lock
        // (guarded by id *and* start ticks, so a changed record survives).
        owner.clearIf(owner.identity());
        lock.post();
        return false;
      }

      // The lock is taken and the record named no living holder when it was
      // read - but in between a live process may have taken the lock over and
      // not yet recorded itself, or recorded itself already. Wait that window
      // out, then look at the record again, freshly read from the cells.
      std::this_thread::sleep_for(probeDelay);
      const ProcessIdentity holder = owner.identity();
      if (holder.processId > 0 && processIdentityIsAlive(holder)) { return false; }
      if (lock.try_wait()) {
        // The holder released the lock during the wait: it is not stuck after
        // all, so hand it back untouched and drop the now stale record.
        owner.clearIf(holder);
        lock.post();
        return false;
      }

      semaphoreRemoved = boost::interprocess::named_semaphore::remove(semaphoreName.c_str());
      // open_or_create: another process may have recreated the lock in between,
      // in which case we open it instead of failing on a duplicate.
      boost::interprocess::named_semaphore(boost::interprocess::open_or_create, semaphoreName.c_str(), 1u);
      // clearIf, not clear: a process that took the lock in the meantime has
      // recorded itself and must keep that record.
      owner.clearIf(holder);
      return true;
    } catch (const std::exception &error) {
      if (semaphoreRemoved) {
        // The orphaned semaphore is gone, so the state *is* reclaimed even
        // though the fresh one could not be opened. Reporting "not reclaimed"
        // would make the caller skip its reopen and keep the orphaned handle.
        LOG_ERROR(
          "cannot recreate the removed lock of shared memory segment '{}': {} "
          "- the next open of the segment recreates it",
          segmentName,
          error.what());
        return true;
      }
      LOG_ERROR("cannot reclaim the orphaned lock of shared memory segment '{}': {}", segmentName, error.what());
      return false;
    }
  }

  /** Find or construct a fixed-size array in the given shared memory segment.
   * @tparam Object the element type
   * @tparam Size   the array size
   * @param service the shared memory service
   * @param segment the shared memory segment
   * @param name    the name of the array object
   * @return reference to the constructed or found array
   * @throws ShmSegmentNotFound if the segment does not exist */
  template<class Object, std::size_t Size>
  std::array<Object, Size> &constructArray(SharedMemoryService &service,
    const std::shared_ptr<SharedMemorySegment> &segment,
    const std::string &name)
  {
    std::lock_guard<std::mutex> lock(service.segmentsMutexOf());
    auto &handle = service.segmentOf(segment->getName());
    SegmentSemaphoreGuard gate(handle);
    return *(handle.m_managedMappedFile->find_or_construct<std::array<Object, Size>>(name.c_str())());
  }

  /** Find or construct a map in the given shared memory segment.
   * @tparam Key   the map key type
   * @tparam Value the map value type
   * @param service the shared memory service
   * @param segment the shared memory segment
   * @param name    the name of the map object
   * @return reference to the constructed or found map
   * @throws ShmSegmentNotFound if the segment does not exist */
  template<class Key, class Value>
  Map<Key, Value> &constructMap(SharedMemoryService &service,
    const std::shared_ptr<SharedMemorySegment> &segment,
    const std::string &name)
  {
    using pairType = std::pair<const Key, Value>;
    using persistentMapAllocator = SegmentAllocator<pairType>;
    std::lock_guard<std::mutex> lock(service.segmentsMutexOf());
    auto &handle = service.segmentOf(segment->getName());
    SegmentSemaphoreGuard gate(handle);
    persistentMapAllocator allocator(handle.m_managedMappedFile->get_segment_manager());
    Map<Key, Value> *map =
      handle.m_managedMappedFile->find_or_construct<Map<Key, Value>>(name.c_str())(std::less<Key>(), allocator);
    LOG_TRACE("map {}", map->size());
    return *map;
  }

  /** Find or construct a vector in the given shared memory segment.
   * @tparam Object the element type
   * @param service the shared memory service
   * @param segment the shared memory segment
   * @param name    the name of the vector object
   * @return reference to the constructed or found vector
   * @throws ShmSegmentNotFound if the segment does not exist */
  template<class Object>
  Vector<Object> &constructVector(SharedMemoryService &service,
    const std::shared_ptr<SharedMemorySegment> &segment,
    const std::string &name)
  {
    using persistentVectorAllocator = SegmentAllocator<Object>;
    std::lock_guard<std::mutex> lock(service.segmentsMutexOf());
    auto &handle = service.segmentOf(segment->getName());
    SegmentSemaphoreGuard gate(handle);
    persistentVectorAllocator allocator(handle.m_managedMappedFile->get_segment_manager());
    return *(handle.m_managedMappedFile->find_or_construct<Vector<Object>>(name.c_str())(allocator));
  }

  /** Find or construct a single object in the given shared memory segment.
   * @tparam Object the object type
   * @param service the shared memory service
   * @param segment the shared memory segment
   * @param name    the name of the object
   * @return reference to the constructed or found object
   * @throws ShmSegmentNotFound if the segment does not exist */
  template<class Object>
  Object &constructObject(SharedMemoryService &service,
    const std::shared_ptr<SharedMemorySegment> &segment,
    const std::string &name)
  {
    std::lock_guard<std::mutex> lock(service.segmentsMutexOf());
    auto &handle = service.segmentOf(segment->getName());
    SegmentSemaphoreGuard gate(handle);
    return *(handle.m_managedMappedFile->find_or_construct<Object>(name.c_str())());
  }

  /** Find or construct a single object with constructor arguments in the
   * given shared memory segment.
   * @tparam Object the object type
   * @tparam Args   the constructor argument types
   * @param service the shared memory service
   * @param segment the shared memory segment
   * @param name    the name of the object
   * @param args    arguments forwarded to the constructor
   * @return reference to the constructed or found object
   * @throws ShmSegmentNotFound if the segment does not exist */
  template<class Object, class... Args>
  Object &constructObjectWith(SharedMemoryService &service,
    const std::shared_ptr<SharedMemorySegment> &segment,
    const std::string &name,
    Args &&...args)
  {
    std::lock_guard<std::mutex> lock(service.segmentsMutexOf());
    auto &handle = service.segmentOf(segment->getName());
    SegmentSemaphoreGuard gate(handle);
    return *(handle.m_managedMappedFile->find_or_construct<Object>(name.c_str())(std::forward<Args>(args)...));
  }

  /** Find or construct a string in the given shared memory segment.
   * @param service the shared memory service
   * @param segment the shared memory segment
   * @param name    the name of the string object
   * @return the constructed or found shared-memory string
   * @throws ShmSegmentNotFound if the segment does not exist */
  inline String constructString(SharedMemoryService &service,
    const std::shared_ptr<SharedMemorySegment> &segment,
    const std::string &name)
  {
    std::lock_guard<std::mutex> lock(service.segmentsMutexOf());
    auto &handle = service.segmentOf(segment->getName());
    SegmentSemaphoreGuard gate(handle);
    SegmentAllocator<char> charallocator(handle.m_managedMappedFile->get_segment_manager());
    String myString(charallocator);
    myString = name.c_str();
    return myString;
  }

  /** Get the raw segment manager of the given shared memory segment.
   * @param service the shared memory service
   * @param segment the shared memory segment
   * @return the segment manager, or nullptr if the segment does not exist */
  inline SegmentManager *segmentManagerOf(SharedMemoryService &service,
    const std::shared_ptr<SharedMemorySegment> &segment)
  {
    std::lock_guard<std::mutex> lock(service.segmentsMutexOf());
    auto found = service.segmentsOf().find(segment->getName());
    if (found == service.segmentsOf().end()) { return nullptr; }
    SegmentSemaphoreGuard gate(*found->second);
    return found->second->m_managedMappedFile->get_segment_manager();
  }

}// namespace shm

}// namespace lintel
#endif// LINTEL_SHMCONSTRUCTORS_H
