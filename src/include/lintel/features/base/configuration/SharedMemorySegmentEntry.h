#ifndef LINTEL_SHAREDMEMORYSEGMENTENTRY_H
#define LINTEL_SHAREDMEMORYSEGMENTENTRY_H

#include <memory>
#include <ostream>

#include "lintel/core/models/SharedMemorySegment.h"
#include "lintel/features/base/configuration/Entry.h"

/** Configuration entry for a shared memory segment */
class SharedMemorySegmentEntry : public Entry {
private:
    /** The shared memory segment model (if this is a segment entry) */
    std::shared_ptr<SharedMemorySegment> m_sharedMemorySegment;
    /** The shared memory path (if this is a path entry) */
    std::shared_ptr<std::filesystem::path> m_sharedMemoryPath;

public:
    /** Construct a shared memory segment entry
     * @param component The configuration component name
     * @param sharedMemorySegment The shared memory segment model */
    SharedMemorySegmentEntry(
            std::string_view component,
            std::shared_ptr<SharedMemorySegment> sharedMemorySegment);

    /** Construct a shared memory path entry
     * @param component The configuration component name
     * @param path The shared memory filesystem path */
    SharedMemorySegmentEntry(std::string_view component,
                             std::shared_ptr<std::filesystem::path> path);

    /** Get the shared memory segment model
     * @return The segment shared pointer */
    [[nodiscard]] const std::shared_ptr<SharedMemorySegment> &
    getSharedMemorySegment() const;

    /** Get the shared memory filesystem path
     * @return The path shared pointer */
    [[nodiscard]] const std::shared_ptr<std::filesystem::path> &
    getSharedMemoryPath() const;
};

#endif  // LINTEL_SHAREDMEMORYSEGMENTENTRY_H
