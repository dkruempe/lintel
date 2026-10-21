#include "lintel/core/shm/ShmView.h"

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
    return handle.m_managedMappedFile->find<char>(name.c_str()).first;
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
