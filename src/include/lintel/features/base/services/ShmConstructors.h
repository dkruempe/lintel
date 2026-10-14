#ifndef LINTEL_SHMCONSTRUCTORS_H
#define LINTEL_SHMCONSTRUCTORS_H

#include <boost/container/map.hpp>
#include <boost/container/string.hpp>
#include <boost/container/vector.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

#include "lintel/core/models/SharedMemorySegment.h"
#include "lintel/core/shm/ShmSegmentAccessor.h"
#include "lintel/features/base/services/SharedMemoryService.h"

namespace lintel {
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
  // Typed allocator adapter of this segment. Created on demand and reset
  // whenever the mapping is replaced, so accessors always refer to the
  // segment manager of the current mapping.
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
   *  throws, turning the silent hang into an actionable error (the orphaned
   *  handle can be inspected/removed out of band). */
  class SegmentSemaphoreGuard
  {
  public:
    /** Default time to wait for the semaphore before giving up. Generous,
     *  because the guarded critical sections are short (remap/construct). */
    static constexpr std::chrono::seconds kDefaultWaitTimeout{ 30 };

    /** @param semaphore semaphore of the segment to guard
     *  @param timeout   upper bound for acquiring the semaphore
     *  @throws std::runtime_error if the semaphore could not be acquired
     *          within the timeout (e.g. a previous holder crashed) */
    explicit SegmentSemaphoreGuard(boost::interprocess::named_semaphore &semaphore,
      std::chrono::steady_clock::duration timeout = kDefaultWaitTimeout)
      : m_semaphore(&semaphore)
    {
      const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(timeout).count();
      const auto deadline =
        boost::posix_time::microsec_clock::universal_time() + boost::posix_time::microseconds(microseconds);
      if (!m_semaphore->timed_wait(deadline)) {
        // A timed-out guard must not post on destruction - that would hand
        // the lock to the next waiter even though we never owned it.
        m_semaphore = nullptr;
        throw std::runtime_error(
          "timed out acquiring the shared memory segment remap semaphore; "
          "a previous process may have crashed while holding it");
      }
    }

    ~SegmentSemaphoreGuard()
    {
      if (m_semaphore != nullptr) { m_semaphore->post(); }
    }

    SegmentSemaphoreGuard(const SegmentSemaphoreGuard &) = delete;

    SegmentSemaphoreGuard &operator=(const SegmentSemaphoreGuard &) = delete;

  private:
    boost::interprocess::named_semaphore *m_semaphore;
  };

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
    SegmentSemaphoreGuard gate(*handle.m_remapSemaphore);
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
    SegmentSemaphoreGuard gate(*handle.m_remapSemaphore);
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
    SegmentSemaphoreGuard gate(*handle.m_remapSemaphore);
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
    SegmentSemaphoreGuard gate(*handle.m_remapSemaphore);
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
    SegmentSemaphoreGuard gate(*handle.m_remapSemaphore);
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
    SegmentSemaphoreGuard gate(*handle.m_remapSemaphore);
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
    SegmentSemaphoreGuard gate(*found->second->m_remapSemaphore);
    return found->second->m_managedMappedFile->get_segment_manager();
  }

}// namespace shm

}// namespace lintel
#endif// LINTEL_SHMCONSTRUCTORS_H
