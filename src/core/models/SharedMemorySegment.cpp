#include "base_library/core/models/SharedMemorySegment.h"

#include <utility>

SharedMemorySegment::SharedMemorySegment(std::filesystem::path sharedMemoryPath,
                                         std::string name,
                                         const std::size_t size)
        : m_path(std::move(sharedMemoryPath)),
          m_name(std::move(name)),
          m_size(size),
          m_isAutoExtend(false),
          m_autoExtendSize(0),
          m_maxSize(0) {}

[[nodiscard]] std::filesystem::path SharedMemorySegment::getPath() const {
    return m_path;
}

[[nodiscard]] const std::string &SharedMemorySegment::getName() const {
    return m_name;
}

[[nodiscard]] size_t SharedMemorySegment::getSize() const { return m_size; }

SharedMemorySegment::SharedMemorySegment(std::filesystem::path sharedMemoryPath,
                                         std::string name, std::size_t size,
                                         std::size_t autoExtendSize,
                                         std::size_t maxSize)
        : m_path(std::move(sharedMemoryPath)),
          m_name(std::move(name)),
          m_size(size),
          m_isAutoExtend(true),
          m_autoExtendSize(autoExtendSize),
          m_maxSize(maxSize) {}

bool SharedMemorySegment::isAutoExtend() const { return m_isAutoExtend; }

size_t SharedMemorySegment::getAutoExtendSize() const {
    return m_autoExtendSize;
}

size_t SharedMemorySegment::getMaxSize() const { return m_maxSize; }
