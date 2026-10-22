#include "lintel/core/shm/ShmView.h"

#include <bit>
#include <cstdint>
#include <cstring>

#include "lintel/core/shm/ShmObjectHeader.h"
#include "lintel/features/base/services/ShmConstructors.h"

namespace lintel {
namespace shm {

  void *findNamedObject(SharedMemoryService &service,
    const std::shared_ptr<SharedMemorySegment> &segment,
    const std::string &name)
  {
    std::lock_guard<std::mutex> lock(service.segmentsMutexOf());
    auto &handle = service.segmentOf(segment->getName());
    SegmentSemaphoreGuard gate(handle);
    // Look the object up as a byte range: the view factory does not need the
    // element type to find the address, only to interpret the bytes afterwards.
    const auto named = handle.m_managedMappedFile->find<char>(name.c_str());
    if (named.first == nullptr || named.second < sizeof(ShmObjectHeader)) {
      // Missing, or too small to carry a header: a pre-header object. Its bytes
      // are not reinterpreted - viewOf() reports the object as absent.
      return nullptr;
    }
    // Read the header as bytes first: a pre-header object is not guaranteed to
    // be aligned like a ShmObjectHeader. Only after the magic proved that boost
    // created the block as one does the address become a header.
    ShmObjectHeader header{};
    std::memcpy(&header, named.first, sizeof(header));
    if (!header.isCompatible() || !header.isReady()) { return nullptr; }
    if (header.payloadOffset == 0) { return nullptr; }
    // The payload is a separate allocation, addressed relative to the header so
    // that the offset survives every mapping of the segment.
    const std::uintptr_t payloadAddress =
      reinterpret_cast<std::uintptr_t>(named.first) + static_cast<std::uintptr_t>(header.payloadOffset);
    return std::bit_cast<void *>(payloadAddress);
  }

  std::size_t generationOf(const std::weak_ptr<SharedMemoryService> &service) noexcept
  {
    const std::shared_ptr<SharedMemoryService> locked = service.lock();
    return locked == nullptr ? ShmView<char>::kInvalidGeneration : locked->getGeneration();
  }

  std::size_t generationOf(const std::shared_ptr<SharedMemoryService> &service) noexcept
  {
    return service == nullptr ? ShmView<char>::kInvalidGeneration : service->getGeneration();
  }

}// namespace shm
}// namespace lintel
