#include "base_library/models/SharedMemorySegmentVersion.h"

#include <utility>

ShmVersion &SharedMemorySegmentVersion::getCurrentVersion() const {
  return currentVersion;
}
SharedMemorySegmentVersion::SharedMemorySegmentVersion(
    const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
    SharedMemorySegment sharedMemorySegment)
    : sharedMemoryService(sharedMemoryService),
      sharedMemorySegment(std::move(sharedMemorySegment)),
      currentVersion(sharedMemoryService->constructObject<ShmVersion>(
          this->sharedMemorySegment.getName(), std::string(objectName))) {}
void SharedMemorySegmentVersion::setVersion(const ShmVersion &newVersion) {
  currentVersion = newVersion;
}
