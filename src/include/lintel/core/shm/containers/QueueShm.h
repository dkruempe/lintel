#ifndef LINTEL_QUEUESHM_H
#define LINTEL_QUEUESHM_H

#include "lintel/core/shm/ShmSegmentAccessor.h"
#include "lintel/core/shm/containers/RingBufferShm.h"

#include <cstddef>
#include <optional>

namespace lintel {
/**
 * FIFO queue in shared memory.
 *
 * Thin composition over RingBufferShm: the queue is the ring, only the names
 * of the operations are queue vocabulary (enqueue/dequeue). Reusing the ring
 * keeps one algorithm, one set of memory orderings and one lock-freedom
 * argument instead of two - QueueShm adds no state of its own beyond what
 * RingBufferShm already stores.
 *
 * Typical use is as the payload of a SharedMemoryRepository<QueueShm<T>>:
 * the first process constructs the queue with a capacity and an accessor to
 * the segment, later processes find it again and use it through their own
 * accessor-backed view of the same bytes.
 *
 * Not copyable (RingBufferShm is not, see there for the reason).
 *
 * Requirements on T are RingBufferShm's: default constructible, copy
 * assignable, copy constructible, identical in every sharing process.
 *
 * @tparam T element type
 */
template<typename T> class QueueShm
{
public:
  /** Construct a queue with a fixed capacity.
   *  @param capacity the maximum number of queued elements (must be > 0)
   *  @param segment  accessor to the segment the ring storage is allocated from
   *  @throws std::invalid_argument on a zero capacity
   *  @throws std::bad_alloc when the segment cannot hold the ring storage */
  QueueShm(std::size_t capacity, ShmSegmentAccessor segment) : m_ring(capacity, segment) {}

  QueueShm(const QueueShm &) = delete;
  QueueShm &operator=(const QueueShm &) = delete;

  /** Enqueue an element at the back; fails when the queue is full.
   *  @param value the element to enqueue
   *  @return true if the element was enqueued */
  bool push(const T &value) { return m_ring.push(value); }

  /** Dequeue the oldest element; fails when the queue is empty.
   *  @param out receives the dequeued element
   *  @return true if an element was dequeued */
  bool pop(T &out) { return m_ring.pop(out); }

  /** Dequeue the oldest element into an optional.
   *  @return the dequeued element, or std::nullopt when the queue is empty */
  [[nodiscard]] std::optional<T> tryPop() { return m_ring.tryPop(); }

  /** Best-effort emptiness check; an in-flight push may be invisible.
   *  @return true if the queue is (likely) empty */
  [[nodiscard]] bool empty() const { return m_ring.empty(); }

  /** Number of queued elements as observed by this thread.
   *  @return the element count, clamped to capacity */
  [[nodiscard]] std::size_t size() const { return m_ring.size(); }

  /** Best-effort fullness check; an in-flight pop may be invisible.
   *  @return true if the queue is (likely) full */
  [[nodiscard]] bool full() const { return m_ring.full(); }

  /** @return the maximum number of queued elements */
  [[nodiscard]] std::size_t capacity() const { return m_ring.capacity(); }

  /** @return true if the queue operations are lock-free */
  [[nodiscard]] static bool isLockFree() { return RingBufferShm<T>::isLockFree(); }

private:
  RingBufferShm<T> m_ring;
};

}// namespace lintel
#endif// LINTEL_QUEUESHM_H
