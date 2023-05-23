#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTMANAGER_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTMANAGER_H

#include <map>
#include <memory>

#include "base_library/core/models/SharedMemorySegment.h"
#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/configuration/SharedMemorySegmentEntry.h"

class SharedMemorySegmentManager {
private:
    std::map<std::string, std::shared_ptr<SharedMemorySegment>>
            m_sharedMemorySegments;

    static std::map<std::string, std::shared_ptr<SharedMemorySegment>> init(
            const std::vector<std::shared_ptr<Entry>> &entries);

public:
    explicit SharedMemorySegmentManager(
            const std::shared_ptr<Configuration> &configuration);

    std::shared_ptr<SharedMemorySegment> of(
            const std::string &sharedMemorySegmentName);

    std::vector<std::shared_ptr<SharedMemorySegment>> allOf(
            const std::string &segmentName = ".*");
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTMANAGER_H
