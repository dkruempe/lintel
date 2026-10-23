#ifndef LINTEL_SHM_OBJECT_H
#define LINTEL_SHM_OBJECT_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "lintel/core/shm/ShmObjectHeader.h"

namespace lintel {

class SharedMemoryService;
class SharedMemorySegment;

namespace shm {

  /**
   * Process local binding of one named, versioned object inside a segment.
   *
   * The named object in the segment is a ShmObjectHeader; the payload itself is
   * a separate allocation, addressed by an offset relative to the header. The
   * binding resolves the header, creates the payload on first use and migrates
   * it when the stored payloadVersion differs from the version this build
   * expects.
   *
   * The header is boost free: finding/constructing it, allocating the payload
   * and running the migration live in the out-of-line methods in
   * src/features/base/services/ShmObject.cpp.
   *
   * A binding lives as long as its owner (a repository or a construct helper).
   * When the segment is remapped, resolve() re-reads the header from the new
   * mapping; the payload pointer is only valid until that remap.
   */
  class ShmObject
  {
  public:
    /** Highest payload alignment the relative addressing supports. */
    static constexpr std::size_t kMaxAlignment = 4096;

    /** Payload size and alignment (a single aggregate so the two values cannot
     *  be swapped at a call site). */
    using Layout = ShmPayloadLayout;

    /** Type-erased pointer to the payload constructor's arguments. */
    struct InitArgument
    {
      const void *value = nullptr;
    };

    /** Runs the payload constructor of T exactly once, when the object is
     *  created or recreated.
     *  @param payload      raw, not yet constructed payload memory
     *  @param initArgument type erased constructor arguments */
    using InitFunction = void (*)(void *payload, InitArgument initArgument);

    /** Bind to the named object without touching the segment yet. */
    ShmObject(std::shared_ptr<SharedMemoryService> service,
      std::shared_ptr<SharedMemorySegment> segment,
      std::string name,
      Layout layout,
      std::uint32_t typeId,
      ShmObjectOptions options,
      InitFunction init,
      std::shared_ptr<void> initArgument);

    ShmObject(const ShmObject &) = delete;
    ShmObject &operator=(const ShmObject &) = delete;

    /** Locate the payload, creating or migrating it on first use and
     *  re-reading the header after the segment was remapped.
     *  @return pointer to the payload, valid until the next remap */
    [[nodiscard]] void *resolve() const;

    /** @return the header of the object; resolves the object first */
    [[nodiscard]] const ShmObjectHeader &header() const;

    /** @return persistence class recorded in the segment */
    [[nodiscard]] ShmPersistence persistence() const { return header().persistenceClass(); }

    /** @return payload version recorded in the segment */
    [[nodiscard]] std::uint16_t payloadVersion() const { return header().payloadVersion; }

    /** @return true when the segment was remapped since the last resolve */
    [[nodiscard]] bool isStale() const;

    /** @return the service generation observed by the last resolve */
    [[nodiscard]] std::size_t generation() const;

    /** @return name of the object inside the segment */
    [[nodiscard]] const std::string &getName() const { return m_name; }

    /** @return segment the object lives in */
    [[nodiscard]] const std::shared_ptr<SharedMemorySegment> &getSegment() const { return m_segment; }

  private:
    std::shared_ptr<SharedMemoryService> m_service;
    std::shared_ptr<SharedMemorySegment> m_segment;
    std::string m_name;
    std::size_t m_payloadSize;
    std::size_t m_alignment;
    std::uint32_t m_typeId;
    ShmObjectOptions m_options;
    InitFunction m_init;
    std::shared_ptr<void> m_initArgument;
    /** Header inside the segment; opaque because creating it is an
     *  implementation detail of ShmObject.cpp. */
    mutable void *m_record = nullptr;
    mutable std::size_t m_generation = 0;
  };

}// namespace shm
}// namespace lintel

#endif// LINTEL_SHM_OBJECT_H
