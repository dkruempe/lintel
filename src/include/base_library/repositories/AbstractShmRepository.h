#ifndef CPP_SYSTEM_LIBRARY_ABSTRACTSHMREPOSITORY_H
#define CPP_SYSTEM_LIBRARY_ABSTRACTSHMREPOSITORY_H

#include <filesystem>
#include <functional>
#include <string>

#include "base_library/models/SharedMemorySegment.h"
#include "base_library/models/SharedMemorySegmentVersion.h"
#include "base_library/models/ShmVersion.h"
#include "base_library/services/SharedMemoryService.h"

/**
 * class is to wrap shared memory object classes and provide an easy way to
 * - versioning the shm object with std::function<void(ShmService)> function
 *   * creates for each object an version object
 *   * for each object can be a lambda function defined to change the structure
 *   * function can be used to transform from an old structure to a new one
 *   * lambda function can be used to expand the shm
 * - provide SharedMemorySegment to the SharedMemoryService
 */
class AbstractShmRepository {
protected:
  struct Convert {
    ShmVersion fromVersion;
    ShmVersion toVersion;
    std::function<void()> convert;
  };
  std::shared_ptr<SharedMemoryService> sharedMemoryService;

  void checkVersion(const ShmVersion &version,
                    std::vector<AbstractShmRepository::Convert> &functions);

private:
  const SharedMemorySegment &sharedMemorySegment;
  SharedMemorySegmentVersion sharedMemorySegmentVersion;
  const std::string name;
  void convert(std::vector<AbstractShmRepository::Convert> &functions);

public:
  AbstractShmRepository(
      const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
      const SharedMemorySegment &sharedMemorySegment, std::string_view name);
  [[nodiscard]] const ShmVersion &getCurrentVersion() const;
  [[nodiscard]] const SharedMemorySegment &getShmSegment() const;
  [[nodiscard]] const std::string &getName() const;
};

#endif // CPP_SYSTEM_LIBRARY_ABSTRACTSHMREPOSITORY_H
