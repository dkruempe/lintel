#ifndef LINTEL_SHAREDMEMORYSEGMENTMANAGER_H
#define LINTEL_SHAREDMEMORYSEGMENTMANAGER_H

#include <map>
#include <memory>

#include "lintel/core/models/SharedMemorySegment.h"
#include "lintel/features/base/configuration/Configuration.h"
#include "lintel/features/base/configuration/SharedMemorySegmentEntry.h"
#include "lintel/features/base/services/ISharedMemorySegmentManager.h"

/**
 * Manages creation and access to named shared memory segments based on configuration.
 */
class SharedMemorySegmentManager : public ISharedMemorySegmentManager {
private:
    std::map<std::string, std::shared_ptr<SharedMemorySegment>>
            m_sharedMemorySegments;

    /**
     * Initialize segments from configuration entries.
     * @param entries configuration entries defining segments
     * @return map of segment name to SharedMemorySegment
     */
    static std::map<std::string, std::shared_ptr<SharedMemorySegment>> init(
            const std::vector<std::shared_ptr<Entry>> &entries);

public:
    /**
     * Constructor.
     * @param configuration application configuration containing segment definitions
     */
    explicit SharedMemorySegmentManager(
            const std::shared_ptr<Configuration> &configuration);

    /**
     * Get a shared memory segment by name.
     * @param sharedMemorySegmentName segment name
     * @return shared memory segment, or nullptr if not found
     */
    std::shared_ptr<SharedMemorySegment> of(
            const std::string &sharedMemorySegmentName);

    /**
     * Get all segments whose name matches the given pattern.
     * @param segmentName regex pattern (default ".*" for all)
     * @return matching segments
     */
    std::vector<std::shared_ptr<SharedMemorySegment>> allOf(
            const std::string &segmentName = ".*");
};

#endif  // LINTEL_SHAREDMEMORYSEGMENTMANAGER_H
