#include "lintel/features/base/configuration/SharedMemorySegmentEntry.h"

#include <utility>

const std::shared_ptr<SharedMemorySegment> &
SharedMemorySegmentEntry::getSharedMemorySegment() const {
    return m_sharedMemorySegment;
}

const std::shared_ptr<std::filesystem::path> &
SharedMemorySegmentEntry::getSharedMemoryPath() const {
    return m_sharedMemoryPath;
}

SharedMemorySegmentEntry::SharedMemorySegmentEntry(
        std::string_view component,
        std::shared_ptr<SharedMemorySegment> sharedMemorySegment)
        : Entry(component), m_sharedMemorySegment(std::move(sharedMemorySegment)) {}

SharedMemorySegmentEntry::SharedMemorySegmentEntry(
        std::string_view component, std::shared_ptr<std::filesystem::path> path)
        : Entry(component), m_sharedMemoryPath(std::move(path)) {}
