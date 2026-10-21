#include "lintel/core/shm/SharedMemoryRepository.h"

#include "lintel/core/models/SharedMemorySegment.h"
#include "lintel/core/shm/BoostSegmentAllocator.h"
#include "lintel/core/shm/ShmSegmentAccessor.h"
#include "lintel/features/base/services/SharedMemoryService.h"
#include "lintel/features/base/services/ShmConstructors.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

namespace lintel {
namespace {

  /**
   * Fixed size bookkeeping record of one cell inside a shared memory segment.
   *
   * The record is what find_or_construct locates by name; the payload itself
   * is a separate allocation of the segment and is referred to by a byte
   * offset relative to the record. Both live in the same segment file, so
   * both shift by the same amount in every mapping and the relative layout is
   * identical in every process (see ShmCell's class documentation).
   *
   * The record deliberately does not store the ShmSegmentAccessor it was
   * created with: the accessor points into the creating process' heap
   * (BoostSegmentAllocator owned by that process' SharedMemoryService) and
   * would be meaningless everywhere else. It is only needed while the payload
   * is allocated, which happens inside this constructor.
   *
   * The magic guards against a name collision with an unrelated object that
   * some other code placed in the segment under the same name: without it,
   * the payload offset of that object would be reinterpreted as cell data.
   */
  class ShmCellRecord
  {
  public:
    /** Marks a cell record; "LINTELSH" in ASCII. */
    static constexpr std::uint64_t kMagic = 0x4c494e54454c5348ull;

    /** Allocate the payload from the segment and run the payload constructor.
     *  @param segment       accessor of the segment the payload is allocated from
     *  @param layout        payload size (> 0) and alignment (power of two)
     *  @param init          payload constructor, run on the allocated memory
     *  @param initArgument  constructor arguments for init
     *  @throws std::bad_alloc when the segment cannot hold the payload
     *  @throws anything init throws (the payload allocation is released first) */
    ShmCellRecord(ShmSegmentAccessor segment,
      ShmCell::Layout layout,
      ShmCell::InitFunction init,
      ShmCell::InitArgument initArgument)
    {
      const std::size_t bytes = layout.size + layout.alignment - 1u;
      void *raw = segment.allocate(bytes);
      if (raw == nullptr) { throw std::bad_alloc(); }
      void *aligned = raw;
      std::size_t space = bytes;
      if (std::align(layout.alignment, layout.size, aligned, space) == nullptr) {
        segment.deallocate(raw);
        throw std::bad_alloc();
      }
      // The record is already placed at its final address when this
      // constructor runs, so the offset is stable for the lifetime of the
      // cell - and identical in every other mapping of the segment.
      m_payloadOffset = reinterpret_cast<std::intptr_t>(aligned) - reinterpret_cast<std::intptr_t>(this);
      m_payloadSize = layout.size;
      m_alignment = layout.alignment;
      try {
        init(aligned, initArgument);
      } catch (...) {
        segment.deallocate(raw);
        m_payloadOffset = 0;
        throw;
      }
    }

    ShmCellRecord(const ShmCellRecord &) = delete;
    ShmCellRecord &operator=(const ShmCellRecord &) = delete;

    /** @return pointer to the payload in this process' mapping */
    [[nodiscard]] void *payload() const
    {
      // The payload offset is stored in the record inside the segment (its
      // address differs per mapping), so it has to be materialised here. The
      // base is a std::byte pointer: byte arithmetic has element size 1, so
      // neither an integer-to-pointer cast nor a widened scale is involved.
      return reinterpret_cast<std::byte *>(const_cast<ShmCellRecord *>(this)) + m_payloadOffset;
    }

    [[nodiscard]] std::uint64_t magic() const { return m_magic; }
    [[nodiscard]] std::size_t payloadSize() const { return m_payloadSize; }
    [[nodiscard]] std::size_t alignment() const { return m_alignment; }

  private:
    std::uint64_t m_magic = kMagic;
    std::intptr_t m_payloadOffset = 0;
    std::size_t m_payloadSize = 0;
    std::size_t m_alignment = 0;
  };

  /**
   * Locate the named cell record, constructing it (and its payload) on first use.
   *
   * Everything runs under the segment's construct lock: the process wide mutex
   * serializes the lookup against a concurrent remap of this process, the
   * cross-process semaphore serializes it against the remap / construct
   * critical sections of every other process. That is what makes the single
   * construction of the cell - record and payload - safe.
   *
   * @param service       service owning the segment
   * @param segment       segment the cell lives in
   * @param name          name of the cell inside the segment
   * @param layout        payload size and alignment
   * @param init          payload constructor, run when the cell is created
   * @param initArgument  constructor arguments for init
   * @return the record, valid until the segment is remapped
   * @throws ShmSegmentNotFound when the service does not know the segment
   * @throws std::invalid_argument when the name already holds an incompatible object
   */
  ShmCellRecord *findRecordOf(SharedMemoryService &service,
    const std::shared_ptr<SharedMemorySegment> &segment,
    const std::string &name,
    ShmCell::Layout layout,
    ShmCell::InitFunction init,
    ShmCell::InitArgument initArgument)
  {
    std::lock_guard<std::mutex> lock(service.segmentsMutexOf());
    auto &handle = service.segmentOf(segment->getName());
    shm::SegmentSemaphoreGuard gate(handle);
    // Same lazy allocator creation as SharedMemoryService::getSegmentAccessor,
    // but without calling it: that would deadlock on the mutex held here.
    if (handle.m_segmentAllocator == nullptr) {
      handle.m_segmentAllocator =
        std::make_shared<BoostSegmentAllocator>(*handle.m_managedMappedFile->get_segment_manager());
    }
    const ShmSegmentAccessor accessor(*handle.m_segmentAllocator);
    auto *record =
      handle.m_managedMappedFile->find_or_construct<ShmCellRecord>(name.c_str())(accessor, layout, init, initArgument);
    // Only reached with a freshly built record trivially, but also - after a
    // restart or from a second process - with one that was found again: the
    // constructor arguments of the finder are ignored there, so the layout
    // has to be compared instead of trusted.
    if (record->magic() != ShmCellRecord::kMagic || record->payloadSize() != layout.size
        || record->alignment() != layout.alignment) {
      throw std::invalid_argument("shared memory cell '" + name + "' of segment '" + segment->getName()
                                  + "' already exists with an incompatible payload layout");
    }
    return record;
  }

}// namespace

ShmCell::ShmCell(std::shared_ptr<SharedMemoryService> service,
  std::shared_ptr<SharedMemorySegment> segment,
  std::string name,
  Layout layout,
  InitFunction init,
  std::shared_ptr<void> initArgument)
  : m_service(std::move(service)), m_segment(std::move(segment)), m_name(std::move(name)), m_payloadSize(layout.size),
    m_alignment(layout.alignment), m_init(init), m_initArgument(std::move(initArgument))
{
  if (m_service == nullptr) { throw std::invalid_argument("ShmCell needs a shared memory service"); }
  if (m_segment == nullptr) { throw std::invalid_argument("ShmCell '" + m_name + "' needs a shared memory segment"); }
  if (m_name.empty()) { throw std::invalid_argument("ShmCell name must not be empty"); }
  if (m_payloadSize == 0) { throw std::invalid_argument("ShmCell '" + m_name + "' payload must not be empty"); }
  if (m_init == nullptr) { throw std::invalid_argument("ShmCell '" + m_name + "' needs a payload constructor"); }
  if (m_initArgument == nullptr) {
    throw std::invalid_argument("ShmCell '" + m_name + "' needs constructor arguments");
  }
  if (m_alignment == 0 || m_alignment > kMaxAlignment || (m_alignment & (m_alignment - 1u)) != 0) {
    throw std::invalid_argument("ShmCell '" + m_name + "' alignment must be a power of two of at most "
                                + std::to_string(kMaxAlignment) + " bytes");
  }
}

void *ShmCell::resolve() const
{
  // Generation read *before* the lookup and stored afterwards: a remap that
  // slips in between then shows up as a stale binding on the next access,
  // which costs one extra lookup - the other order would cache a pointer of
  // the old mapping as up to date, which is the dangerous direction.
  const std::size_t observed = m_service->getGeneration();
  if (m_record != nullptr && observed == m_generation) { return static_cast<ShmCellRecord *>(m_record)->payload(); }
  auto *record = findRecordOf(*m_service,
    m_segment,
    m_name,
    Layout{ m_payloadSize, m_alignment },
    m_init,
    ShmCell::InitArgument{ m_initArgument.get() });
  m_record = record;
  m_generation = observed;
  return record->payload();
}

bool ShmCell::isStale() const { return m_record == nullptr || m_service->getGeneration() != m_generation; }

std::size_t ShmCell::generation() const { return m_generation; }

}// namespace lintel
