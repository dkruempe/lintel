#include "base_library/models/SharedMemorySegment.h"

#include <utility>

SharedMemorySegment::SharedMemorySegment(std::filesystem::path sharedMemoryPath,
                                         std::string name,
                                         const std::size_t size)
    : sharedMemoryPath(std::move(sharedMemoryPath)), name(std::move(name)),
      size(size) {}

[[nodiscard]] std::filesystem::path SharedMemorySegment::getPath() const {
  return std::filesystem::path(sharedMemoryPath.string() +
                               std::filesystem::path::preferred_separator +
                               name + std::string(fileEnding));
}

[[nodiscard]] const std::string &SharedMemorySegment::getName() const {
  return name;
}

[[nodiscard]] size_t SharedMemorySegment::getSize() const { return size; }