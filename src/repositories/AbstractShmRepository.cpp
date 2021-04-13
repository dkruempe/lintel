#include "base_library/repositories/AbstractShmRepository.h"

#include <utility>

AbstractShmRepository::AbstractShmRepository(
    const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
    const SharedMemorySegment &sharedMemorySegment, std::string_view name)
    : sharedMemoryService(sharedMemoryService),
      sharedMemorySegment(sharedMemorySegment),
      sharedMemorySegmentVersion(sharedMemoryService,
                                 this->sharedMemorySegment),
      name(name) {}
void AbstractShmRepository::convert(
    std::vector<AbstractShmRepository::Convert> &functions) {
  std::sort(functions.begin(), functions.end(),
            [](const Convert &rh, const Convert &lh) -> bool {
              return rh.fromVersion < lh.fromVersion;
            });
  for (auto &iter : functions) {
    if (iter.fromVersion < getCurrentVersion()) {
      continue;
    }
    if (getCurrentVersion() != iter.fromVersion) {
      break;
    }
    iter.convert();
    sharedMemorySegmentVersion.setVersion(iter.toVersion);
  }
}
const ShmVersion &AbstractShmRepository::getCurrentVersion() const {
  return sharedMemorySegmentVersion.getCurrentVersion();
}
const SharedMemorySegment &AbstractShmRepository::getShmSegment() const {
  return sharedMemorySegment;
}
const std::string &AbstractShmRepository::getName() const { return name; }
void AbstractShmRepository::checkVersion(const ShmVersion &version,
                                         std::vector<Convert> &functions) {
  if (getCurrentVersion() ==
      ShmVersion{.major = -1, .minor = -1, .patch = -1}) {
    sharedMemorySegmentVersion.setVersion(version);
    return;
  } else if (getCurrentVersion() == version) {
    return;
  }
  convert(functions);
}
