#include "lintel/features/base/msg/MessageQueueShmCore.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace lintel {
namespace mqshm_detail {

  constexpr std::size_t kCacheLine = 64;
  /**
   * Per slot header: sequence number, stored byte count and padding.
   *
   * Kept at its natural alignment (16 bytes). Slots are separated by a
   * cache-line-padded stride instead of an `alignas` on the header: the stride
   * already guarantees that consecutive sequence handshakes live on different
   * cache lines, and MSVC warns (C4324, promoted to an error here) when a small
   * struct is padded by an alignment specifier.
   */
  constexpr std::size_t kSlotHeaderSize = 16;
  /** Granularity of the wait used by the blocking operations. */
  constexpr std::chrono::milliseconds kWaitSlice{ 1 };
  /** Prefix of the segment object name that holds a queue's header. */
  constexpr const char *kHeaderNamePrefix = "lintel_mq_";
  /** Upper bound for that object name, including the terminating NUL. */
  constexpr std::size_t kHeaderNameSize = 32;
  /** Marks an initialised header; "LMQSHM01" in ASCII. */
  constexpr std::uint64_t kQueueMagic = 0x4c4d5153484d3031ull;
  /** Magic value while one process is constructing the ring behind a fresh header. */
  constexpr std::uint64_t kQueueMagicInitialising = 1;
  /** Upper bound for waiting until another process finished that construction. */
  constexpr std::chrono::milliseconds kInitWaitTimeout{ 5000 };

  [[nodiscard]] std::size_t alignUp(std::size_t value, std::size_t alignment)
  {
    return (value + alignment - 1u) / alignment * alignment;
  }

  /**
   * Fixed-capacity MPMC ring of fixed-size byte slots inside shared memory.
   *
   * The algorithm is the Vyukov bounded MPMC queue used by RingBufferShm and
   * EventBus::ShmRing. The difference is the runtime slot size: every slot is
   * a sequence header followed by `slotSize` payload bytes, and the stride is
   * padded to a cache line so concurrent producers/consumers do not share a
   * cache line for the sequence handshake.
   *
   * The slot array is allocated from the shared segment and addressed through
   * a byte offset relative to this object, exactly like RingBufferShm.
   */
  class ShmRing
  {
  public:
    struct SlotHeader
    {
      std::atomic<std::uint64_t> sequence{ 0 };
      std::uint32_t length{ 0 };
      std::uint32_t padding{ 0 };
    };
    static_assert(sizeof(SlotHeader) == kSlotHeaderSize, "SlotHeader size must match kSlotHeaderSize");

    ShmRing(std::size_t capacity, std::size_t slotSize, ShmSegmentAccessor segment)
      : m_capacity(capacity), m_slotSize(slotSize), m_segment(segment)
    {
      if (capacity == 0) { throw std::invalid_argument("MessageQueueShmCore capacity must be positive"); }
      if (slotSize == 0) { throw std::invalid_argument("MessageQueueShmCore message size must be positive"); }
      if (slotSize > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument("MessageQueueShmCore message size is too large");
      }
      if (capacity > std::numeric_limits<std::size_t>::max() / (kSlotHeaderSize + slotSize)) {
        throw std::invalid_argument("MessageQueueShmCore slot layout is too large");
      }

      m_stride = alignUp(kSlotHeaderSize + slotSize, kCacheLine);
      if (capacity > std::numeric_limits<std::size_t>::max() / m_stride) {
        throw std::invalid_argument("MessageQueueShmCore capacity is too large");
      }

      const std::size_t bufferSize = capacity * m_stride;
      const std::size_t allocationSize = bufferSize + kCacheLine - 1u;
      void *raw = segment.allocate(allocationSize);
      if (raw == nullptr) { throw std::bad_alloc(); }
      m_allocation = static_cast<std::byte *>(raw);

      void *alignedRaw = raw;
      std::size_t alignmentSpace = allocationSize;
      if (std::align(kCacheLine, bufferSize, alignedRaw, alignmentSpace) == nullptr) {
        segment.deallocate(raw);
        m_allocation = nullptr;
        throw std::bad_alloc();
      }

      m_bufferOffset = reinterpret_cast<std::intptr_t>(alignedRaw) - reinterpret_cast<std::intptr_t>(this);
      for (std::size_t index = 0; index < capacity; ++index) {
        SlotHeader *slot = slotAt(index);
        new (slot) SlotHeader();
        slot->sequence.store(static_cast<std::uint64_t>(index), std::memory_order_relaxed);
      }
    }

    ~ShmRing()
    {
      if (m_allocation != nullptr) {
        m_segment.deallocate(m_allocation);
        m_allocation = nullptr;
      }
    }

    ShmRing(const ShmRing &) = delete;
    ShmRing &operator=(const ShmRing &) = delete;

    [[nodiscard]] bool push(const void *message, std::size_t size)
    {
      // The sequence number handshake cannot distinguish full from empty when
      // the ring has exactly one slot (both states map to the same sequence),
      // so a single-slot ring uses an explicit state machine instead:
      // 0 = empty, 1 = readable, 2 = currently being written/read.
      if (m_capacity == 1u) {
        std::uint8_t expected = kSingleEmpty;
        if (!m_singleState.compare_exchange_strong(
              expected, kSingleBusy, std::memory_order_acq_rel, std::memory_order_acquire)) {
          return false;
        }
        SlotHeader *slot = slotAt(0);
        slot->length = static_cast<std::uint32_t>(size);
        std::memcpy(payloadOf(slot), message, size);
        m_singleState.store(kSingleReady, std::memory_order_release);
        return true;
      }

      std::uint64_t position = m_head.load(std::memory_order_relaxed);
      for (;;) {
        SlotHeader *slot = slotAt(position);
        const std::uint64_t sequence = slot->sequence.load(std::memory_order_acquire);
        const std::int64_t difference = static_cast<std::int64_t>(sequence - position);
        if (difference == 0) {
          std::uint64_t expected = position;
          if (m_head.compare_exchange_weak(expected, position + 1u, std::memory_order_relaxed)) {
            slot->length = static_cast<std::uint32_t>(size);
            std::memcpy(payloadOf(slot), message, size);
            slot->sequence.store(position + 1u, std::memory_order_release);
            return true;
          }
          position = expected;
        } else if (difference < 0) {
          return false;// full
        } else {
          position = m_head.load(std::memory_order_relaxed);
        }
      }
    }

    [[nodiscard]] bool pop(void *message, std::size_t size)
    {
      if (m_capacity == 1u) {
        std::uint8_t expected = kSingleReady;
        if (!m_singleState.compare_exchange_strong(
              expected, kSingleBusy, std::memory_order_acq_rel, std::memory_order_acquire)) {
          return false;
        }
        SlotHeader *slot = slotAt(0);
        const std::size_t copySize = std::min<std::size_t>(slot->length, size);
        if (message != nullptr && copySize > 0) { std::memcpy(message, payloadOf(slot), copySize); }
        m_singleState.store(kSingleEmpty, std::memory_order_release);
        return true;
      }

      std::uint64_t position = m_tail.load(std::memory_order_relaxed);
      for (;;) {
        SlotHeader *slot = slotAt(position);
        const std::uint64_t sequence = slot->sequence.load(std::memory_order_acquire);
        const std::int64_t difference = static_cast<std::int64_t>(sequence - (position + 1u));
        if (difference == 0) {
          std::uint64_t expected = position;
          if (m_tail.compare_exchange_weak(expected, position + 1u, std::memory_order_relaxed)) {
            const std::size_t copySize = std::min<std::size_t>(slot->length, size);
            if (message != nullptr && copySize > 0) { std::memcpy(message, payloadOf(slot), copySize); }
            slot->sequence.store(position + m_capacity, std::memory_order_release);
            return true;
          }
          position = expected;
        } else if (difference < 0) {
          return false;// empty
        } else {
          position = m_tail.load(std::memory_order_relaxed);
        }
      }
    }

    [[nodiscard]] bool popDiscard()
    {
      std::byte discard[1]{};
      return pop(discard, sizeof(discard));
    }

    [[nodiscard]] bool empty() const
    {
      if (m_capacity == 1u) { return m_singleState.load(std::memory_order_acquire) != kSingleReady; }
      return m_tail.load(std::memory_order_acquire) == m_head.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::size_t size() const
    {
      if (m_capacity == 1u) { return m_singleState.load(std::memory_order_acquire) == kSingleReady ? 1u : 0u; }
      const std::uint64_t head = m_head.load(std::memory_order_acquire);
      const std::uint64_t tail = m_tail.load(std::memory_order_acquire);
      const std::uint64_t used = head >= tail ? head - tail : 0u;
      return used >= m_capacity ? m_capacity : static_cast<std::size_t>(used);
    }

    [[nodiscard]] bool full() const { return size() >= m_capacity; }

    [[nodiscard]] std::size_t capacity() const { return m_capacity; }

    [[nodiscard]] std::size_t slotSize() const { return m_slotSize; }

    /** @return distance in bytes between two consecutive slots */
    [[nodiscard]] std::size_t stride() const { return m_stride; }

    [[nodiscard]] static bool isLockFree() { return std::atomic<std::uint64_t>::is_always_lock_free; }

  private:
    static constexpr std::uint8_t kSingleEmpty = 0;
    static constexpr std::uint8_t kSingleReady = 1;
    static constexpr std::uint8_t kSingleBusy = 2;

    std::atomic<std::uint64_t> m_head{ 0 };
    // Padding keeps m_head and m_tail on separate cache lines to avoid false
    // sharing between producer and consumer. Clang's -Wunused-private-field
    // would flag the arrays, so the intent is documented here and the
    // attributes keep the layout while silencing the warning.
    [[maybe_unused]] std::byte m_headPadding[kCacheLine - sizeof(std::uint64_t)]{};
    std::atomic<std::uint64_t> m_tail{ 0 };
    [[maybe_unused]] std::byte m_tailPadding[kCacheLine - sizeof(std::uint64_t)]{};
    std::atomic<std::uint8_t> m_singleState{ kSingleEmpty };
    std::size_t m_capacity = 0;
    std::size_t m_slotSize = 0;
    std::size_t m_stride = 0;
    ShmSegmentAccessor m_segment;
    std::byte *m_allocation = nullptr;
    std::intptr_t m_bufferOffset = 0;

    /** Slot at a logical ring position.
     *
     *  The slot array lives in the shared segment, not inside this object, and
     *  is addressed by the byte offset m_bufferOffset: the stride is not a
     *  multiple of sizeof(SlotHeader) either, so element arithmetic is not an
     *  option and the address is materialised in the integer domain.
     *  std::bit_cast restores it: a plain reinterpret_cast from the integer
     *  would be reported as performance-no-int-to-ptr, byte pointer arithmetic
     *  as cpp/suspicious-pointer-scaling. */
    [[nodiscard]] SlotHeader *slotAt(std::uint64_t position) const
    {
      const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(this)
                                     + static_cast<std::uintptr_t>(m_bufferOffset) + (position % m_capacity) * m_stride;
      return std::bit_cast<SlotHeader *>(address);
    }

    /** Payload of a slot lives directly behind its header. `slot + 1` advances
     *  by exactly kSlotHeaderSize (see the static_assert on SlotHeader), so the
     *  offset is expressed as typed pointer arithmetic. */
    [[nodiscard]] static std::byte *payloadOf(SlotHeader *slot) { return reinterpret_cast<std::byte *>(slot + 1); }
  };

  // The block comes from find_or_construct<char>, whose allocation is aligned
  // to the segment's memory alignment (max_align_t), and the ring is placed at
  // a multiple of its own alignment behind the header - which only works while
  // that alignment does not exceed the block alignment.
  static_assert(alignof(ShmRing) <= alignof(std::max_align_t),
    "the named queue block must be aligned enough for the ring");

  /**
   * Cross-process anchor of one queue inside the segment.
   *
   * The header is a named object of the segment ("lintel_mq_" + queue name) and
   * is what a second process finds first: it describes where the ring sits and
   * which layout it has. Everything in here is either a scalar or a byte
   * offset, never a pointer - the segment is mapped at a different virtual
   * address in every process.
   *
   * The header is never constructed as a C++ object: the allocator hands out a
   * zero initialised block (magic == 0), which is exactly the "not
   * initialised" state. A writer claims the header with a compare exchange
   * (magic == kQueueMagicInitialising), fills the fields and publishes them by
   * storing kQueueMagic with release semantics; a reader waits until the claim
   * is resolved and then loads the magic with acquire, after which the fields
   * are valid.
   */
  struct QueueHeader
  {
    /** 0 = not initialised, kQueueMagicInitialising = being initialised,
     *  kQueueMagic = the fields below are valid */
    std::uint64_t magic;
    int32_t capacity;
    std::size_t slotSize;
    std::size_t stride;
    /** Byte offset of the ShmRing relative to this header */
    std::intptr_t ringOffset;
  };

  /** State shared by every core of one process that opens the same queue. */
  struct QueueState
  {
    ShmRing *ring = nullptr;
    /** Header of this queue in the segment; valid in every process that opened it. */
    QueueHeader *header = nullptr;
    ShmSegmentAccessor accessor;
    std::string name;
    int32_t capacity = 0;
    std::size_t slotSize = 0;
    /** True only for the core that constructed the ring (see the destructor). */
    bool owner = false;
    std::mutex waitMutex;
    std::condition_variable waitCondition;

    explicit QueueState(ShmSegmentAccessor segment) : accessor(segment) {}

    ~QueueState()
    {
      // Only the process that constructed the ring may tear it down: the ring
      // and its slot buffer belong to the segment, and the process local
      // pointer to the slot buffer inside the ring means nothing in any other
      // process. An adopting core leaves both alone.
      if (!owner) { return; }
      if (header != nullptr) {
        // Mark the header as uninitialised *before* the ring disappears, so a
        // later open re-creates the ring instead of adopting a destroyed one.
        // The header block itself stays in the segment: it is a named object,
        // and a plain deallocate would leave the name in the segment's index
        // pointing at freed memory.
        std::atomic_ref<std::uint64_t>(header->magic).store(0u, std::memory_order_release);
      }
      if (ring != nullptr) {
        ring->~ShmRing();
        ring = nullptr;
      }
    }

    void notifyWaiters() { waitCondition.notify_all(); }

    /** Wait a slice for progress on the queue.
     *
     *  This is a poll, not a wakeup: the condition variable only knows the
     *  waiters of this process, and a producer in another process has no way to
     *  signal it. Every waiter therefore re-checks the ring after kWaitSlice.
     *  A cross-process wakeup (named semaphore or a futex inside the segment)
     *  is deliberately left out; it would add a second synchronisation object
     *  to the queue for a latency gain of at most one slice. */
    void waitForProgress(const std::chrono::steady_clock::time_point &deadline, bool bounded)
    {
      std::unique_lock<std::mutex> lock(waitMutex);
      if (!bounded) {
        waitCondition.wait_for(lock, kWaitSlice);
        return;
      }
      const auto now = std::chrono::steady_clock::now();
      if (now >= deadline) { return; }
      const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
      waitCondition.wait_for(lock, std::min(remaining, kWaitSlice));
    }
  };

  std::mutex g_registryMutex;
  /** Process local cache of the open queues; the segment header is the source of truth. */
  std::map<std::string, std::weak_ptr<QueueState>> g_registry;

  /** Name of the segment object that holds a queue's header.
   *  @param name of the queue
   *  @return name of the header object inside the segment
   *  @throws std::invalid_argument if the composed name does not fit into kHeaderNameSize bytes */
  [[nodiscard]] std::string headerNameOf(const std::string &name)
  {
    std::string headerName = std::string(kHeaderNamePrefix) + name;
    if (headerName.size() >= kHeaderNameSize) {
      throw std::invalid_argument("MessageQueueShmCore queue name '" + name
                                  + "' is too long: the shared memory object name '" + headerName + "' exceeds "
                                  + std::to_string(kHeaderNameSize - 1) + " characters");
    }
    return headerName;
  }

  /** @return the ring at a byte offset from the header block */
  [[nodiscard]] ShmRing *ringAt(void *block, std::size_t offset)
  {
    // The ring follows the header inside the same segment allocation, so its
    // address is block + offset. The offset is not a multiple of sizeof(ShmRing),
    // element arithmetic is not an option, and the address is therefore
    // materialised in the integer domain. std::bit_cast restores it: a plain
    // reinterpret_cast from the integer would be reported as
    // performance-no-int-to-ptr.
    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(block) + static_cast<std::uintptr_t>(offset);
    return std::bit_cast<ShmRing *>(address);
  }

  /** Wait until another process resolved the initialisation claim of a header.
   *
   *  A header whose magic is kQueueMagicInitialising belongs to a process that
   *  is constructing the ring right now. The wait is bounded: a process that
   *  died in that window leaves the claim behind, and taking it over would
   *  mean touching a ring somebody else may still be building. Failing loudly
   *  is the safe direction - the segment has to be removed by an operator.
   *
   *  @param name   of the queue, for the error message
   *  @param header the header to watch
   *  @return the magic the header settled on
   *  @throws std::runtime_error if it did not settle within kInitWaitTimeout */
  [[nodiscard]] std::uint64_t waitForHeader(const std::string &name, QueueHeader &header)
  {
    const auto deadline = std::chrono::steady_clock::now() + kInitWaitTimeout;
    for (;;) {
      const std::uint64_t magic = std::atomic_ref<std::uint64_t>(header.magic).load(std::memory_order_acquire);
      if (magic != kQueueMagicInitialising) { return magic; }
      if (std::chrono::steady_clock::now() >= deadline) {
        throw std::runtime_error("timed out waiting for the initialisation of the shared memory queue '" + name
                                 + "': the process that started to create it did not finish");
      }
      std::this_thread::sleep_for(kWaitSlice);
    }
  }

  [[nodiscard]] std::shared_ptr<QueueState>
    openOrCreateState(const std::string &name, int32_t msgCount, std::size_t msgSize, ShmSegmentAccessor segment)
  {
    if (msgCount <= 0) { throw std::invalid_argument("MessageQueueShmCore message count must be positive"); }
    if (msgSize == 0) { throw std::invalid_argument("MessageQueueShmCore message size must be positive"); }
    if (segment.segmentName().empty()) {
      throw std::invalid_argument(
        "MessageQueueShmCore needs a named segment accessor so the queue is found across "
        "processes; use SharedMemoryService::getSegmentAccessor()");
    }

    std::lock_guard<std::mutex> lock(g_registryMutex);
    const auto found = g_registry.find(name);
    if (found != g_registry.end()) {
      if (auto existing = found->second.lock()) {
        if (existing->capacity != msgCount || existing->slotSize != msgSize) {
          throw std::invalid_argument("MessageQueueShmCore already exists with a different layout: " + name);
        }
        return existing;
      }
      g_registry.erase(found);
    }

    // The header is the cross-process anchor of the queue: it lives in the
    // segment under a name derived from the queue name, so a second process
    // finds the very same ring instead of creating one of its own. The ring
    // and its slot array follow the header inside the same segment, which is
    // why only byte offsets are stored - a pointer would be process local.
    const std::size_t ringOffset = alignUp(sizeof(QueueHeader), alignof(ShmRing));
    const std::size_t blockSize = ringOffset + sizeof(ShmRing);
    const std::string headerName = headerNameOf(name);
    void *block = segment.findOrConstructArray(headerName.c_str(), blockSize);
    if (block == nullptr) {
      throw std::runtime_error("cannot create the queue header '" + headerName
                               + "' in the shared memory segment: the allocator does not support named objects");
    }

    auto *header = static_cast<QueueHeader *>(block);
    auto state = std::make_shared<QueueState>(segment);
    state->name = name;
    state->capacity = msgCount;
    state->slotSize = msgSize;
    state->header = header;

    const std::uint64_t magic = std::atomic_ref<std::uint64_t>(header->magic).load(std::memory_order_acquire);
    if (magic == 0u) {
      // Fresh header: the allocator zero initialised the block, so no ring has
      // been placed behind it yet. Claim the initialisation with a compare
      // exchange, so a second process cannot build a second ring at the same
      // address, then construct the ring, publish the layout and set the magic
      // last - a reader that sees the magic sees the fields too.
      std::uint64_t expected = 0u;
      if (std::atomic_ref<std::uint64_t>(header->magic)
            .compare_exchange_strong(expected, kQueueMagicInitialising, std::memory_order_acq_rel)) {
        try {
          state->ring = new (ringAt(block, ringOffset)) ShmRing(static_cast<std::size_t>(msgCount), msgSize, segment);
        } catch (...) {
          // Release the claim: a header stuck in "initialising" would make
          // every later open wait for a ring that will never be published.
          std::atomic_ref<std::uint64_t>(header->magic).store(0u, std::memory_order_release);
          throw;
        }
        header->capacity = msgCount;
        header->slotSize = msgSize;
        header->stride = state->ring->stride();
        header->ringOffset = static_cast<std::intptr_t>(ringOffset);
        std::atomic_ref<std::uint64_t>(header->magic).store(kQueueMagic, std::memory_order_release);
        state->owner = true;
        g_registry.emplace(name, state);
        return state;
      }
      // Another process claimed the header first: fall through and adopt the
      // ring once it publishes it.
    }

    const std::uint64_t settled = waitForHeader(name, *header);
    if (settled != kQueueMagic) {
      throw std::runtime_error("the shared memory queue '" + name
                               + "' is not initialised: its creating process removed it or the header is corrupt");
    }
    const std::intptr_t storedOffset = header->ringOffset;
    if (storedOffset < static_cast<std::intptr_t>(sizeof(QueueHeader))
        || static_cast<std::size_t>(storedOffset) + sizeof(ShmRing) > blockSize) {
      throw std::runtime_error(
        "the shared memory queue '" + name + "' has a corrupt header (ring offset out of range)");
    }
    if (header->capacity != msgCount || header->slotSize != msgSize) {
      throw std::invalid_argument("MessageQueueShmCore already exists with a different layout: " + name);
    }
    // An existing header: the ring is already constructed by the creating
    // process, adopt it instead of constructing a second one.
    state->ring = ringAt(block, static_cast<std::size_t>(storedOffset));
    state->owner = false;

    g_registry.emplace(name, state);
    return state;
  }

  [[nodiscard]] std::shared_ptr<QueueState> findStateByName(const std::string &name)
  {
    std::lock_guard<std::mutex> lock(g_registryMutex);
    const auto found = g_registry.find(name);
    if (found == g_registry.end()) { return nullptr; }
    return found->second.lock();
  }

  [[nodiscard]] bool pushBlocking(const std::shared_ptr<QueueState> &state, const void *message, std::size_t size)
  {
    for (;;) {
      if (state->ring->push(message, size)) {
        state->notifyWaiters();
        return true;
      }
      state->waitForProgress({}, false);
    }
  }

  [[nodiscard]] bool pushWithTimeout(const std::shared_ptr<QueueState> &state,
    const void *message,
    std::size_t size,
    const std::chrono::milliseconds &timeout)
  {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    for (;;) {
      if (state->ring->push(message, size)) {
        state->notifyWaiters();
        return true;
      }
      if (std::chrono::steady_clock::now() >= deadline) { return false; }
      state->waitForProgress(deadline, true);
    }
  }

  [[nodiscard]] bool popBlocking(const std::shared_ptr<QueueState> &state, void *message, std::size_t size)
  {
    for (;;) {
      if (state->ring->pop(message, size)) {
        state->notifyWaiters();
        return true;
      }
      state->waitForProgress({}, false);
    }
  }

  [[nodiscard]] bool popWithTimeout(const std::shared_ptr<QueueState> &state,
    void *message,
    std::size_t size,
    const std::chrono::milliseconds &timeout)
  {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    for (;;) {
      if (state->ring->pop(message, size)) {
        state->notifyWaiters();
        return true;
      }
      if (std::chrono::steady_clock::now() >= deadline) { return false; }
      state->waitForProgress(deadline, true);
    }
  }

  void validateSize(const QueueState &state, std::size_t size)
  {
    if (size > state.slotSize) { throw std::length_error("message exceeds MessageQueueShmCore slot size"); }
  }

}// namespace mqshm_detail

using mqshm_detail::findStateByName;
using mqshm_detail::g_registry;
using mqshm_detail::g_registryMutex;
using mqshm_detail::openOrCreateState;
using mqshm_detail::popBlocking;
using mqshm_detail::popWithTimeout;
using mqshm_detail::pushBlocking;
using mqshm_detail::pushWithTimeout;
using mqshm_detail::QueueState;
using mqshm_detail::ShmRing;
using mqshm_detail::validateSize;

struct MessageQueueShmCore::Impl
{
  std::shared_ptr<QueueState> state;
  MessageQueueOverflowPolicy policy = MessageQueueOverflowPolicy::BlockWithTimeout;
  std::chrono::milliseconds timeout{ 100 };
  std::string deadLetterQueueName;
  /** Kept open so the forwarded messages stay visible/receivable after this call. */
  std::shared_ptr<QueueState> deadLetterState;

  Impl(const std::string &name, int32_t msgCount, std::size_t msgSize, ShmSegmentAccessor segment)
    : state(openOrCreateState(name, msgCount, msgSize, segment))
  {}

  [[nodiscard]] bool forwardToDeadLetter(const void *message, std::size_t size)
  {
    if (deadLetterQueueName.empty() || deadLetterQueueName == state->name) { return false; }
    try {
      if (deadLetterState == nullptr || deadLetterState->name != deadLetterQueueName) {
        deadLetterState = openOrCreateState(deadLetterQueueName, state->capacity, state->slotSize, state->accessor);
      }
      if (deadLetterState->ring->push(message, size)) {
        deadLetterState->notifyWaiters();
        return true;
      }
      return false;
    } catch (...) {
      return false;
    }
  }

  [[nodiscard]] bool applyOverflowPolicy(const void *message, std::size_t size)
  {
    switch (policy) {
    case MessageQueueOverflowPolicy::DropOldest: {
      while (state->ring->popDiscard()) {
        if (state->ring->push(message, size)) {
          state->notifyWaiters();
          return true;
        }
      }
      return false;
    }
    case MessageQueueOverflowPolicy::BlockWithTimeout:
      // Bounded wait configured via setOverflowTimeout(); if the queue stays
      // full for the whole window the message is dropped (returns false).
      return pushWithTimeout(state, message, size, timeout);
    case MessageQueueOverflowPolicy::DeadLetter:
      return forwardToDeadLetter(message, size);
    case MessageQueueOverflowPolicy::Fail:
    case MessageQueueOverflowPolicy::DropNewest:
    default:
      return false;
    }
  }
};

MessageQueueShmCore::MessageQueueShmCore(const std::string &name,
  int32_t msgCount,
  std::size_t msgSize,
  ShmSegmentAccessor segment)
  : m_impl(std::make_unique<Impl>(name, msgCount, msgSize, segment))
{}

MessageQueueShmCore::~MessageQueueShmCore() = default;

void MessageQueueShmCore::removeOf(const std::string &messageQueueName)
{
  std::lock_guard<std::mutex> lock(g_registryMutex);
  g_registry.erase(messageQueueName);
}

int32_t MessageQueueShmCore::numberMessagesOfIfExistsOf(const std::string &messageQueueName)
{
  const auto state = findStateByName(messageQueueName);
  if (state == nullptr) { return 0; }
  return static_cast<int32_t>(state->ring->size());
}

int32_t MessageQueueShmCore::numberMessagesOf() const { return static_cast<int32_t>(m_impl->state->ring->size()); }

const std::string &MessageQueueShmCore::getName() const { return m_impl->state->name; }

int32_t MessageQueueShmCore::getMsgCount() const { return m_impl->state->capacity; }

std::size_t MessageQueueShmCore::getMsgSize() const { return m_impl->state->slotSize; }

bool MessageQueueShmCore::isLockFree() { return ShmRing::isLockFree(); }

void MessageQueueShmCore::sendOf(const void *message, std::size_t size)
{
  validateSize(*m_impl->state, size);
  static_cast<void>(pushBlocking(m_impl->state, message, size));
}

bool MessageQueueShmCore::trySendOf(const void *message, std::size_t size)
{
  validateSize(*m_impl->state, size);
  if (m_impl->state->ring->push(message, size)) {
    m_impl->state->notifyWaiters();
    return true;
  }
  return m_impl->applyOverflowPolicy(message, size);
}

bool MessageQueueShmCore::sendOfWithTimeout(const void *message,
  std::size_t size,
  const std::chrono::milliseconds &timeout)
{
  validateSize(*m_impl->state, size);
  if (pushWithTimeout(m_impl->state, message, size, timeout)) { return true; }
  return m_impl->applyOverflowPolicy(message, size);
}

void MessageQueueShmCore::receiveOf(void *message, std::size_t size)
{
  static_cast<void>(popBlocking(m_impl->state, message, size));
}

bool MessageQueueShmCore::tryReceiveOf(void *message, std::size_t size)
{
  if (m_impl->state->ring->pop(message, size)) {
    m_impl->state->notifyWaiters();
    return true;
  }
  return false;
}

bool MessageQueueShmCore::receiveOfWithTimeout(void *message,
  std::size_t size,
  const std::chrono::milliseconds &timeout)
{
  return popWithTimeout(m_impl->state, message, size, timeout);
}

void MessageQueueShmCore::setOverflowPolicy(MessageQueueOverflowPolicy policy) { m_impl->policy = policy; }

void MessageQueueShmCore::setOverflowTimeout(const std::chrono::milliseconds &timeout) { m_impl->timeout = timeout; }

void MessageQueueShmCore::setDeadLetterQueueName(const std::string &name) { m_impl->deadLetterQueueName = name; }

}// namespace lintel
