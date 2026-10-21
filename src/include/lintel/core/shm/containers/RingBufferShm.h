#ifndef LINTEL_RINGBUFFERSHM_H
#define LINTEL_RINGBUFFERSHM_H

#include "lintel/core/shm/ShmSegmentAccessor.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>

namespace lintel {
/**
 * Bounded multi producer / multi consumer ring buffer in shared memory.
 *
 * Implementation is the Vyukov bounded MPMC queue, the same algorithm (and
 * layout) EventBus::ShmRing uses: every slot carries a sequence number, the
 * producers claim a head position and the consumers a tail position, and the
 * per-slot sequence decides whether the position was free or taken. No mutex,
 * no condition variable - push()/pop() are lock free as long as 64 bit
 * atomics are, which the static_assert below demands.
 *
 * Storage layout: the ring object itself (head, tail, bookkeeping) lives
 * wherever the ring is constructed - typically as the payload of a
 * SharedMemoryRepository inside a segment - while the slot array is allocated
 * separately from the segment through ShmSegmentAccessor and addressed via a
 * byte offset relative to the ring object (same pattern as
 * EventBus::ShmRing). The relative layout is identical in every mapping of
 * the segment, so the queue works from every process regardless of the
 * virtual address it mapped the segment at.
 *
 * The ring is usable both as a shared memory object (constructed once by the
 * first process, found by the others) and as a process local object on the
 * stack in tests - the accessor decides where the slots come from.
 *
 * Not movable and not copyable: the slot buffer is addressed relative to
 * `this`, so moving the ring would invalidate m_bufferOffset.
 *
 * Requirements on T: default constructible (slot array initialization),
 * copy assignable (push) and copy constructible (pop) - in other words
 * trivially copyable types such as integers, POD structs or fixed size
 * records. T must be identical in every process that shares the ring.
 *
 * @tparam T element type
 */
template<typename T> class RingBufferShm
{
  static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
    "RingBufferShm needs lock-free 64 bit atomics in shared memory");

public:
  /** Cache line size used to separate hot atomics and align slots. */
  static constexpr std::size_t CACHE_LINE = 64;

  /** A single ring slot; padded to one cache line. */
  struct alignas(CACHE_LINE) Slot
  {
    std::atomic<std::uint64_t> sequence{ 0 };
    T value{};
  };

  /** Construct a ring with a fixed capacity.
   *  @param capacity the maximum number of queued elements (must be > 0)
   *  @param segment  accessor to the segment the slot array is allocated from
   *  @throws std::invalid_argument on a zero capacity
   *  @throws std::bad_alloc when the segment cannot hold the slot array */
  RingBufferShm(std::size_t capacity, ShmSegmentAccessor segment) : m_capacity(capacity), m_segment(segment)
  {
    if (capacity == 0) { throw std::invalid_argument("RingBufferShm capacity must be positive"); }
    if (capacity > SIZE_MAX / sizeof(Slot)) { throw std::invalid_argument("RingBufferShm capacity is too large"); }
    const std::size_t bufferSize = capacity * sizeof(Slot);
    auto *raw = static_cast<std::byte *>(segment.allocate(bufferSize + CACHE_LINE - 1u));
    if (raw == nullptr) { throw std::bad_alloc(); }
    m_allocation = raw;
    std::byte *aligned = nullptr;
    {
      void *alignedRaw = raw;
      std::size_t alignmentSpace = bufferSize + CACHE_LINE - 1u;
      if (std::align(CACHE_LINE, bufferSize, alignedRaw, alignmentSpace) == nullptr) {
        // Not reached through the repository's payload constructor (a throw
        // there skips this object's destructor), so release the block here.
        segment.deallocate(raw);
        m_allocation = nullptr;
        throw std::bad_alloc();
      }
      aligned = static_cast<std::byte *>(alignedRaw);
    }
    m_bufferOffset = aligned - reinterpret_cast<std::byte *>(this);
    Slot *slots = buffer();
    for (std::size_t i = 0; i < capacity; ++i) {
      new (&slots[i]) Slot();
      slots[i].sequence.store(static_cast<std::uint64_t>(i), std::memory_order_relaxed);
    }
  }

  /** Deallocates the slot array (only runs when the ring is destroyed by the
   *  process that constructed it - a repository never destroys its payload). */
  ~RingBufferShm()
  {
    if (m_allocation != nullptr) {
      m_segment.deallocate(m_allocation);
      m_allocation = nullptr;
    }
  }

  RingBufferShm(const RingBufferShm &) = delete;
  RingBufferShm &operator=(const RingBufferShm &) = delete;

  /** Enqueue an element; fails when the ring is full.
   *  @param value the element to queue
   *  @return true if the element was enqueued */
  bool push(const T &value)
  {
    Slot *slots = buffer();
    std::uint64_t pos = m_head.load(std::memory_order_relaxed);
    for (;;) {
      Slot &slot = slots[pos % m_capacity];
      const std::uint64_t seq = slot.sequence.load(std::memory_order_acquire);
      const std::int64_t dif = static_cast<std::int64_t>(seq - pos);
      if (dif == 0) {
        std::uint64_t expected = pos;
        if (m_head.compare_exchange_weak(expected, pos + 1u, std::memory_order_relaxed)) {
          slot.value = value;
          slot.sequence.store(pos + 1u, std::memory_order_release);
          return true;
        }
        pos = expected;
      } else if (dif < 0) {
        return false;
      } else {
        pos = m_head.load(std::memory_order_relaxed);
      }
    }
  }

  /** Dequeue the oldest element; fails when the ring is empty.
   *  @param out receives the dequeued element
   *  @return true if an element was dequeued */
  bool pop(T &out)
  {
    Slot *slots = buffer();
    std::uint64_t pos = m_tail.load(std::memory_order_relaxed);
    for (;;) {
      Slot &slot = slots[pos % m_capacity];
      const std::uint64_t seq = slot.sequence.load(std::memory_order_acquire);
      const std::int64_t dif = static_cast<std::int64_t>(seq - (pos + 1u));
      if (dif == 0) {
        std::uint64_t expected = pos;
        if (m_tail.compare_exchange_weak(expected, pos + 1u, std::memory_order_relaxed)) {
          out = slot.value;
          slot.sequence.store(pos + m_capacity, std::memory_order_release);
          return true;
        }
        pos = expected;
      } else if (dif < 0) {
        return false;
      } else {
        pos = m_tail.load(std::memory_order_relaxed);
      }
    }
  }

  /** Dequeue the oldest element into an optional.
   *  @return the dequeued element, or std::nullopt when the ring is empty */
  [[nodiscard]] std::optional<T> tryPop()
  {
    T value{};
    if (!pop(value)) { return std::nullopt; }
    return value;
  }

  /** Best-effort emptiness check; an in-flight push may be invisible.
   *  @return true if the ring is (likely) empty */
  [[nodiscard]] bool empty() const
  {
    return m_tail.load(std::memory_order_acquire) == m_head.load(std::memory_order_acquire);
  }

  /** Number of queued elements as observed by this thread; under concurrent
   *  access the value is a snapshot and may already be stale.
   *  @return the element count, clamped to capacity */
  [[nodiscard]] std::size_t size() const
  {
    const std::uint64_t head = m_head.load(std::memory_order_acquire);
    const std::uint64_t tail = m_tail.load(std::memory_order_acquire);
    const std::uint64_t used = head >= tail ? head - tail : 0u;
    return used >= m_capacity ? m_capacity : static_cast<std::size_t>(used);
  }

  /** Best-effort fullness check; an in-flight pop may be invisible.
   *  @return true if the ring is (likely) full */
  [[nodiscard]] bool full() const { return size() >= m_capacity; }

  /** @return the maximum number of queued elements */
  [[nodiscard]] std::size_t capacity() const { return m_capacity; }

  /** @return true if the queue operations are lock-free */
  [[nodiscard]] static bool isLockFree() { return std::atomic<std::uint64_t>::is_always_lock_free; }

private:
  std::atomic<std::uint64_t> m_head{ 0 };
  std::byte m_headPadding[CACHE_LINE - sizeof(std::uint64_t)]{};
  std::atomic<std::uint64_t> m_tail{ 0 };
  std::byte m_tailPadding[CACHE_LINE - sizeof(std::uint64_t)]{};
  std::size_t m_capacity = 0;
  ShmSegmentAccessor m_segment;
  /** First byte of the slot array allocation; nullptr when construction of
   *  the array failed (the ring then never gets used). */
  std::byte *m_allocation = nullptr;
  /** Byte offset from this object to the aligned slot array. */
  std::intptr_t m_bufferOffset = 0;

  [[nodiscard]] Slot *buffer()
  {
    // The slot array lives in the shared segment, not inside this object, and is addressed by the
    // byte offset m_bufferOffset. The addition runs in the integer domain instead of as
    // `reinterpret_cast<std::byte *>(this) + m_bufferOffset`: that byte pointer arithmetic is
    // exactly the intent, yet CodeQL reports it as cpp/suspicious-pointer-scaling (a RingBufferShm *
    // scaled by bytes). The aligned buffer is not necessarily a multiple of sizeof(Slot), so
    // element arithmetic is not an option either.
    const std::intptr_t bufferAddress = reinterpret_cast<std::intptr_t>(this) + m_bufferOffset;
    // NOLINTNEXTLINE(performance-no-int-to-ptr) — address in this process' mapping, restored verbatim.
    return reinterpret_cast<Slot *>(bufferAddress);
  }

  [[nodiscard]] const Slot *buffer() const
  {
    const std::intptr_t bufferAddress = reinterpret_cast<std::intptr_t>(this) + m_bufferOffset;
    // NOLINTNEXTLINE(performance-no-int-to-ptr) — see the non-const overload.
    return reinterpret_cast<const Slot *>(bufferAddress);
  }
};

}// namespace lintel
#endif// LINTEL_RINGBUFFERSHM_H
