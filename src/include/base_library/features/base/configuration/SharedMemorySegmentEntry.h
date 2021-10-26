#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTENTRY_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTENTRY_H

#include <memory>
#include <ostream>

#include "base_library/core/models/SharedMemorySegment.h"
#include "base_library/features/base/configuration/Entry.h"

class SharedMemorySegmentEntry : public Entry {
 private:
  std::shared_ptr<SharedMemorySegment> m_sharedMemorySegment;
  std::shared_ptr<std::filesystem::path> m_sharedMemoryPath;

 public:
  SharedMemorySegmentEntry(
      std::string_view component,
      std::shared_ptr<SharedMemorySegment> sharedMemorySegment);

  SharedMemorySegmentEntry(std::string_view component,
                           std::shared_ptr<std::filesystem::path> path);

  [[nodiscard]] const std::shared_ptr<SharedMemorySegment>&
  getSharedMemorySegment() const;
  [[nodiscard]] const std::shared_ptr<std::filesystem::path>&
  getSharedMemoryPath() const;
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTENTRY_H
