#ifndef LINTEL_SHMSEGMENTACCESSOR_H
#define LINTEL_SHMSEGMENTACCESSOR_H

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

#include "lintel/core/services/LoggerService.h"
#include "lintel/core/shm/ShmObjectHeader.h"

namespace lintel {
/**
 * Typed access to the allocator of a shared memory segment.
 *
 * The accessor is a small value type that keeps the boost interprocess headers
 * out of EventBus.h: objects stored in shared memory hold a copy of the
 * accessor, while the process local allocator it points to is owned by
 * SharedMemoryService. Only the raw block allocation itself is untyped,
 * everything above it stays typed.
 *
 * Copying is intentional: the copy lives inside the shared memory object and
 * therefore keeps a stable address for the whole lifetime of that object.
 *
 * A pointer alone cannot survive the process boundary: the copy inside the
 * segment would point at the heap of the process that created the object, and
 * that address means nothing in any other process (it may not even be mapped
 * there). The accessor therefore carries the *name* of its segment, which is
 * stable across processes, and resolves that name to a process local allocator
 * on every allocation. `bind()` is what fills the process local side; it is
 * called by SharedMemoryService whenever it hands out an accessor.
 *
 * An accessor constructed without a name (the single argument constructor)
 * keeps working on the allocator it was built with. That is the right thing
 * for objects that never leave the process - a segment used as a plain buffer,
 * tests and benchmarks - and it is what the named form falls back to when the
 * name was never bound in this process.
 */
class ShmSegmentAccessor
{
public:
  /** Capacity of the stored segment name in bytes, including the terminating NUL. */
  static constexpr std::size_t NAME_SIZE = 32;

  /** Upper bound for waiting until the creator of a payload published it. */
  static constexpr std::chrono::milliseconds kPayloadInitTimeout{ 5000 };

  /** Granularity of the wait for that publication. */
  static constexpr std::chrono::milliseconds kPayloadInitWaitSlice{ 1 };

  /** Process local allocator an accessor forwards to. */
  class Allocator
  {
  public:
    virtual ~Allocator() = default;

    /**
     * @param bytes to allocate
     * @return pointer to the allocated block
     */
    virtual void *allocate(std::size_t bytes) = 0;

    /**
     * @param pointer block previously returned by allocate
     */
    virtual void deallocate(void *pointer) = 0;

    /**
     * Find or construct a named byte array inside the segment.
     *
     * The name is the cross-process identity of the object: the first caller
     * creates the array, every later caller - in this or in any other process
     * - gets the same bytes back. A fresh array is value initialised, so it
     * starts zeroed.
     *
     * The default implementation does not support named objects and reports
     * that with nullptr, which keeps the allocator interface usable for
     * segment stand-ins that only need allocate/deallocate.
     *
     * @param name  name of the array inside the segment
     * @param bytes size of the array in bytes (must be positive)
     * @return pointer to the array, or nullptr when named objects are not supported
     */
    virtual void *findOrConstructArray([[maybe_unused]] const char *name, [[maybe_unused]] std::size_t bytes)
    {
      return nullptr;
    }
  };

  /**
   * Construct an accessor that is bound to its allocator only, without a
   * segment name. The resulting accessor is process local: a copy stored in
   * the segment is not usable from another process.
   *
   * @param allocator process local allocator of the segment
   */
  explicit ShmSegmentAccessor(Allocator &allocator) noexcept : m_allocator(&allocator) {}

  /**
   * Construct an accessor that identifies its segment by name.
   *
   * The name is stored in the accessor (and therefore in every copy that ends
   * up inside the segment) so that another process can resolve it to its own
   * allocator. The name is not bound here; call bind() for that, which
   * SharedMemoryService does before it hands the accessor out.
   *
   * @param segmentName name of the shared memory segment
   * @param allocator   process local allocator of that segment
   * @throws std::invalid_argument if the name does not fit into NAME_SIZE bytes
   */
  ShmSegmentAccessor(std::string_view segmentName, Allocator &allocator) : m_allocator(&allocator)
  {
    validateName(segmentName);
    std::memcpy(m_segmentName, segmentName.data(), segmentName.size());
    m_segmentName[segmentName.size()] = '\0';
  }

  /**
   * Bind a segment name to the allocator of this process.
   *
   * Every accessor that carries this name resolves it to @p allocator, no
   * matter which process created the accessor or how old the copy is. Binding
   * again replaces the previous allocator, which is what a remapped segment
   * needs: the old allocator belongs to the closed mapping.
   *
   * The registry lives for the whole process; @p allocator has to outlive
   * every accessor that is used in this process, exactly like the allocator of
   * an unnamed accessor.
   *
   * @param segmentName name of the shared memory segment
   * @param allocator   this process' allocator of that segment
   * @throws std::invalid_argument if the name does not fit into NAME_SIZE bytes
   */
  static void bind(std::string_view segmentName, Allocator &allocator)
  {
    validateName(segmentName);
    std::lock_guard<std::mutex> lock(registry().mutex);
    registry().allocators.insert_or_assign(std::string(segmentName), &allocator);
  }

  /**
   * @param bytes to allocate from the segment
   * @return pointer to the allocated block
   */
  [[nodiscard]] void *allocate(std::size_t bytes) const { return allocator()->allocate(bytes); }

  /**
   * @param pointer block previously returned by allocate
   */
  void deallocate(void *pointer) const { allocator()->deallocate(pointer); }

  /**
   * Find or construct a named byte array in the segment of this accessor.
   *
   * @param name  name of the array inside the segment
   * @param bytes size of the array in bytes
   * @return pointer to the array, or nullptr when the allocator does not support named objects
   */
  [[nodiscard]] void *findOrConstructArray(const char *name, std::size_t bytes) const
  {
    return allocator()->findOrConstructArray(name, bytes);
  }

  /**
   * Find or construct the versioned payload of a named object.
   *
   * The named object in the segment is a shm::ShmObjectHeader (allocated by
   * findOrConstructArray()), the payload itself is a separate allocation
   * addressed by the header. That separation is what keeps the payload in place
   * when the library's bookkeeping changes: a build that can read the header
   * finds the same payload, a build that cannot leaves the bytes untouched and
   * reports it.
   *
   * A freshly created header is claimed with a compare exchange, so exactly one
   * process allocates and publishes the payload; everybody else waits for the
   * header to become ready. A ready payload that this build cannot use is
   * recreated (zeroed) when its persistence class allows it - Volatile or
   * Derived - and rejected otherwise. No destructor of the old payload runs:
   * the old type may not even exist in this build, so nested allocations of it
   * stay in the segment until the segment is recreated.
   *
   * The alignment is limited to alignof(std::max_align_t): the segment
   * allocator returns blocks aligned to that value, so the aligned payload is
   * the block start and a later deallocate() on the payload pointer is valid.
   *
   * @param name      name of the header object inside the segment
   * @param bytes     payload size in bytes (must be positive)
   * @param alignment payload alignment in bytes (power of two, at most
   *                  alignof(std::max_align_t))
   * @param typeId    id of the payload type, stored in the header
   * @param options   payload version and persistence class
   * @return pointer to the payload, or nullptr when the allocator does not
   *         support named objects
   * @throws std::invalid_argument on an invalid request, or when an
   *         incompatible payload may not be recreated because of its
   *         persistence class
   * @throws std::runtime_error when the named object was written by an
   *         incompatible build or its initialisation does not finish in time
   */
  [[nodiscard]] void *findOrConstructPayload(const char *name,
    std::size_t bytes,
    std::size_t alignment,
    std::uint32_t typeId,
    shm::ShmObjectOptions options) const
  {
    validatePayloadRequest(name, shm::ShmPayloadLayout{ bytes, alignment });
    auto *header = static_cast<shm::ShmObjectHeader *>(findOrConstructArray(name, sizeof(shm::ShmObjectHeader)));
    if (header == nullptr) { return nullptr; }

    if (isBlankHeader(*header)) {
      // Fresh header: the allocator zero initialised the block, so nothing was
      // published yet. Claim it, so no second process allocates a payload of
      // its own behind the same header.
      std::atomic_ref<std::uint32_t> state(header->state);
      std::uint32_t expected = 0;
      if (state.compare_exchange_strong(
            expected, shm::ShmObjectHeader::kStateConstructing, std::memory_order_acq_rel, std::memory_order_acquire)) {
        try {
          initialisePayload(*header, shm::ShmPayloadLayout{ bytes, alignment }, typeId, options);
        } catch (...) {
          // Release the claim: a header stuck in "constructing" would make
          // every later open wait for a payload that will never be published.
          state.store(0u, std::memory_order_release);
          throw;
        }
        return shm::payloadAddressOf(header);
      }
      // Another process claimed the header between our lookup and the claim:
      // fall through and wait for its payload.
    }

    awaitReadyHeader(*header, name);
    reconcilePayload(*header, name, bytes, alignment, typeId, options);
    return shm::payloadAddressOf(header);
  }

  /** @return the segment name this accessor identifies its segment by; empty for an unnamed accessor */
  [[nodiscard]] std::string_view segmentName() const noexcept { return std::string_view(m_segmentName); }

private:
  /** Process wide name -> allocator map, see bind(). */
  struct Registry
  {
    std::mutex mutex;
    /** std::less<> makes the lookup accept a string_view without building a temporary string. */
    std::map<std::string, Allocator *, std::less<>> allocators;
  };

  /** @return the registry of this process; a function local static is shared by every translation unit */
  static Registry &registry()
  {
    static Registry instance;
    return instance;
  }

  /** @param segmentName name to check against NAME_SIZE
   *  @throws std::invalid_argument if the name does not fit, including its NUL */
  static void validateName(std::string_view segmentName)
  {
    if (segmentName.size() < NAME_SIZE) { return; }
    throw std::invalid_argument("shared memory segment name '" + std::string(segmentName)
                                + "' is too long: a ShmSegmentAccessor stores at most " + std::to_string(NAME_SIZE - 1)
                                + " characters");
  }

  /** @param name   name of the payload object
   *  @param layout requested payload size and alignment
   *  @throws std::invalid_argument on a request the segment cannot serve */
  static void validatePayloadRequest(const char *name, shm::ShmPayloadLayout layout)
  {
    if (name == nullptr || name[0] == '\0') {
      throw std::invalid_argument("a named shared memory payload needs a name");
    }
    if (layout.size == 0) {
      throw std::invalid_argument("the payload of shared memory object '" + std::string(name) + "' must not be empty");
    }
    if (layout.alignment == 0 || (layout.alignment & (layout.alignment - 1u)) != 0) {
      throw std::invalid_argument(
        "the payload alignment of shared memory object '" + std::string(name) + "' must be a power of two");
    }
    if (layout.alignment > alignof(std::max_align_t)) {
      // The segment allocator aligns to max_align_t, so a larger alignment would
      // make the payload pointer differ from the allocated block start - and a
      // later deallocate() on the payload pointer would then be invalid.
      throw std::invalid_argument("the payload alignment of shared memory object '" + std::string(name)
                                  + "' must not exceed " + std::to_string(alignof(std::max_align_t)) + " bytes");
    }
  }

  /** @param header header of a named object
   *  @return true when the header was created but nothing was published yet;
   *          a header claimed by another process still counts as blank */
  static bool isBlankHeader(const shm::ShmObjectHeader &header) noexcept
  {
    return header.magic == 0 && header.headerVersion == 0 && header.payloadBytes == 0 && header.payloadOffset == 0;
  }

  /** Wait until the creator of a payload published the header.
   *
   *  The wait is bounded: a process that died while claiming the header leaves
   *  it in the constructing state forever, and taking it over could free a
   *  payload somebody else is about to publish. Failing loudly is the safe
   *  direction.
   *
   *  @param header header of the object
   *  @param name   name of the object, for the error message
   *  @throws std::runtime_error when the object has no compatible header or the
   *          publication does not finish within kPayloadInitTimeout */
  static void awaitReadyHeader(shm::ShmObjectHeader &header, const char *name)
  {
    const auto deadline = std::chrono::steady_clock::now() + kPayloadInitTimeout;
    for (;;) {
      if (header.isCompatible() && header.isReady()) { return; }
      if (!isBlankHeader(header) && !header.isCompatible()) {
        throw std::runtime_error("shared memory object '" + std::string(name)
                                 + "' has no compatible object header (written by an older build or with a different "
                                   "layout); register a migrator or recreate the segment");
      }
      if (std::chrono::steady_clock::now() >= deadline) {
        throw std::runtime_error("timed out waiting for the initialisation of shared memory object '"
                                 + std::string(name) + "': the process that started to create it did not finish");
      }
      std::this_thread::sleep_for(kPayloadInitWaitSlice);
    }
  }

  /** Validate a ready header against this build's request and recreate the
   *  payload when its persistence class allows that.
   *
   *  @param header    ready header of the object
   *  @param name      name of the object, for the error message
   *  @param bytes     payload size this build expects
   *  @param alignment payload alignment this build expects
   *  @param typeId    payload type id this build expects
   *  @param options   version and persistence class this build registers
   *  @throws std::invalid_argument when the persistence class does not allow
   *          recreating an incompatible payload */
  void reconcilePayload(shm::ShmObjectHeader &header,
    const char *name,
    std::size_t bytes,
    std::size_t alignment,
    std::uint32_t typeId,
    const shm::ShmObjectOptions &options) const
  {
    const bool samePayload = header.payloadVersion == options.payloadVersion && header.payloadTypeId == typeId
                             && header.payloadBytes == static_cast<std::uint64_t>(bytes)
                             && header.payloadAlignment == static_cast<std::uint64_t>(alignment);
    if (samePayload) { return; }
    // Discard only when *both* the stored and the requested class allow it: an
    // object the old build wrote as Volatile may still be authoritative for a
    // build that registers it as Persistent.
    const bool discardable = header.persistenceClass() != shm::ShmPersistence::Persistent
                             && options.persistence != shm::ShmPersistence::Persistent;
    if (!discardable) {
      throw std::invalid_argument("shared memory object '" + std::string(name) + "' is incompatible: it stores payload "
                                  "type " + std::to_string(header.payloadTypeId) + " version "
                                  + std::to_string(header.payloadVersion) + " with "
                                  + std::to_string(header.payloadBytes) + " bytes, but this build expects type "
                                  + std::to_string(typeId) + " version " + std::to_string(options.payloadVersion)
                                  + " with " + std::to_string(bytes) + " bytes, and its persistence class does not allow "
                                  "recreating it");
    }
    // Volatile or Derived: recreate the payload zeroed. No destructor of the
    // old payload runs - the old type may not exist in this build.
    initialisePayload(header, shm::ShmPayloadLayout{ bytes, alignment }, typeId, options);
  }

  /** Release the payload block a header describes, if any, and put the header
   *  back into the freshly created state.
   *
   *  @param header header of the object */
  void releasePayload(shm::ShmObjectHeader &header) const noexcept
  {
    // Stop readers first: a header marked constructing is not dereferenced.
    header.state = shm::ShmObjectHeader::kStateConstructing;
    const bool hasPayload = header.payloadOffset != 0 && header.payloadBytes != 0;
    if (hasPayload) {
      void *payload = shm::payloadAddressOf(&header);
      try {
        deallocate(payload);
      } catch (const std::exception &error) {
        // The segment is being torn down or the block was already released;
        // leaking is the only safe option and the segment owns the bytes.
        LOG_DEBUG("cannot release a shared memory payload: {}", error.what());
      }
    }
    // Back to the blank state of a freshly created header: a failed
    // initialisation must not leave a header that every later open waits for.
    header.magic = 0;
    header.headerVersion = 0;
    header.payloadTypeId = 0;
    header.payloadVersion = 0;
    header.payloadOffset = 0;
    header.payloadBytes = 0;
    header.payloadAlignment = 1;
  }

  /** Allocate the payload, publish it into the header and mark the header ready.
   *  The caller claimed the header with state == kStateConstructing.
   *
   *  @param header  header of the object
   *  @param layout  payload size and alignment in bytes
   *  @param typeId  payload type id
   *  @param options version and persistence class to record
   *  @throws std::bad_alloc when the segment cannot serve the allocation */
  void initialisePayload(shm::ShmObjectHeader &header,
    shm::ShmPayloadLayout layout,
    std::uint32_t typeId,
    const shm::ShmObjectOptions &options) const
  {
    const std::size_t bytes = layout.size;
    const std::size_t alignment = layout.alignment;
    releasePayload(header);
    const std::size_t allocationBytes = bytes + alignment - 1u;
    void *raw = allocate(allocationBytes);
    if (raw == nullptr) { throw std::bad_alloc(); }
    void *aligned = raw;
    std::size_t space = allocationBytes;
    if (std::align(alignment, bytes, aligned, space) == nullptr) {
      deallocate(raw);
      throw std::bad_alloc();
    }
    // A fresh block carries whatever the segment held before: the payload must
    // start zeroed, like the named array of the old layout did.
    std::memset(aligned, 0, bytes);
    header.magic = shm::ShmObjectHeader::kMagic;
    header.headerVersion = shm::ShmObjectHeader::kHeaderVersion;
    header.flags = 0;
    header.payloadTypeId = typeId;
    header.payloadVersion = options.payloadVersion;
    header.persistence = static_cast<std::uint8_t>(options.persistence);
    header.reservedByte = 0;
    header.payloadBytes = static_cast<std::uint64_t>(bytes);
    header.payloadAlignment = static_cast<std::uint64_t>(alignment);
    header.payloadOffset = reinterpret_cast<std::intptr_t>(aligned) - reinterpret_cast<std::intptr_t>(&header);
    // Publish last: a reader that sees kStateReady sees every field above.
    header.state = shm::ShmObjectHeader::kStateReady;
  }

  /**
   * Allocator to forward to: the one bound to this accessor's segment name in
   * this process, otherwise the one captured at construction time.
   *
   * @return allocator of this process
   */
  [[nodiscard]] Allocator *allocator() const
  {
    if (m_segmentName[0] == '\0') { return m_allocator; }
    std::lock_guard<std::mutex> lock(registry().mutex);
    const auto found = registry().allocators.find(std::string_view(m_segmentName));
    if (found == registry().allocators.end()) { return m_allocator; }
    return found->second;
  }

  Allocator *m_allocator;
  /** Name of the segment this accessor belongs to; empty for an unnamed accessor. */
  char m_segmentName[NAME_SIZE]{};
};

}// namespace lintel
#endif// LINTEL_SHMSEGMENTACCESSOR_H
