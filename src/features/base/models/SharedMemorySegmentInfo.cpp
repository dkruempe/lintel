#include "lintel/features/base/models/SharedMemorySegmentInfo.h"

#include <utility>

SharedMemorySegmentInfo::SharedMemorySegmentInfo(
        std::shared_ptr<SharedMemorySegment> sharedMemorySegment,
        size_t currentSize, size_t freeSize, size_t amountNamedObjects,
        size_t amountUniqueObjects, bool sanity)
        : m_sharedMemorySegment(std::move(sharedMemorySegment)),
          m_currentSize(currentSize),
          m_freeSize(freeSize),
          m_amountNamedObjects(amountNamedObjects),
          m_amountUniqueObjects(amountUniqueObjects),
          m_sanity(sanity) {}

const std::shared_ptr<SharedMemorySegment> &
SharedMemorySegmentInfo::getSharedMemorySegment() const {
    return m_sharedMemorySegment;
}
