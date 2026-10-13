#include "lintel/core/models/SharedMemorySegment.h"

#include <utility>

namespace lintel {
SharedMemorySegment::SharedMemorySegment(std::filesystem::path sharedMemoryPath,
  std::string name,
  const std::size_t size)
  : m_path(std::move(sharedMemoryPath)), m_name(std::move(name)), m_size(size), m_isAutoExtend(false),
    m_autoExtendSize(0), m_maxSize(0)
{}

[[nodiscard]] std::filesystem::path SharedMemorySegment::getPath() const { return m_path; }

[[nodiscard]] const std::string &SharedMemorySegment::getName() const { return m_name; }

SharedMemorySegment::SharedMemorySegment(std::filesystem::path sharedMemoryPath,
  std::string name,
  std::size_t size,
  AutoExtendConfig autoExtendConfig)
  : m_path(std::move(sharedMemoryPath)), m_name(std::move(name)), m_size(size), m_isAutoExtend(true),
    m_autoExtendSize(autoExtendConfig.size), m_maxSize(autoExtendConfig.maxSize)
{}
}// namespace lintel
