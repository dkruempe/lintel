#include "base_library/features/base/repositories/SharedMemoryRepository.h"
SharedMemoryRepository::SharedMemoryRepository(
    std::shared_ptr<SharedMemorySegment> sharedMemorySegment)
    : m_sharedMemorySegment(std::move(sharedMemorySegment)) {}
