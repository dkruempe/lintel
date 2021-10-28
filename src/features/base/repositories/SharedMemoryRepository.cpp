#include "base_library/features/base/repositories/SharedMemoryRepository.h"

SharedMemoryRepository::SharedMemoryRepository(
    std::shared_ptr<SharedMemorySegment> sharedMemorySegment,
    std::size_t sizeOfData, std::string_view sharedMemoryRepository,
    int32_t codeVersion)
    : m_sharedMemorySegment(std::move(sharedMemorySegment)),
      m_sizeOfData(sizeOfData),
      m_sharedMemoryRepository(sharedMemoryRepository),
      m_codeVersion(codeVersion) {}

const std::shared_ptr<SharedMemorySegment>&
SharedMemoryRepository::getSharedMemorySegment() const {
  return m_sharedMemorySegment;
}
std::size_t SharedMemoryRepository::getSizeOfData() const {
  return m_sizeOfData;
}
const std::string& SharedMemoryRepository::getSharedMemoryRepository() const {
  return m_sharedMemoryRepository;
}
int32_t SharedMemoryRepository::getCodeVersion() const { return m_codeVersion; }
