#ifndef CPP_SYSTEM_LIBRARY_SHAREDMEMORYSEGMENTVERSION_H
#define CPP_SYSTEM_LIBRARY_SHAREDMEMORYSEGMENTVERSION_H

#include "base_library/models/ShmVersion.h"
#include "base_library/services/SharedMemoryService.h"

class SharedMemorySegmentVersion {
private:
  std::string_view objectName = "ShmSegmentVersion";
  std::shared_ptr<SharedMemoryService> sharedMemoryService;
  const SharedMemorySegment sharedMemorySegment;
  ShmVersion &currentVersion;

public:
  explicit SharedMemorySegmentVersion(
      const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
      SharedMemorySegment sharedMemorySegment);
  [[nodiscard]] ShmVersion &getCurrentVersion() const;
  void setVersion(const ShmVersion &newVersion);
};

#endif // CPP_SYSTEM_LIBRARY_SHAREDMEMORYSEGMENTVERSION_H