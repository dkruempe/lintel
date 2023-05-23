#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTINFO_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTINFO_H

#include <memory>

#include "base_library/core/models/SharedMemorySegment.h"

class SharedMemorySegmentInfo {
private:
    std::shared_ptr<SharedMemorySegment> m_sharedMemorySegment;
    std::size_t m_currentSize;
    std::size_t m_freeSize;
    std::size_t m_amountNamedObjects;
    std::size_t m_amountUniqueObjects;
    bool m_sanity;

public:
    SharedMemorySegmentInfo(
            std::shared_ptr<SharedMemorySegment> sharedMemorySegment,
            size_t currentSize, size_t freeSize, size_t amountNamedObjects,
            size_t amountUniqueObjects, bool sanity);

    [[nodiscard]] const std::shared_ptr<SharedMemorySegment> &
    getSharedMemorySegment() const;

    [[nodiscard]] size_t getCurrentSize() const;

    [[nodiscard]] size_t getFreeSize() const;

    [[nodiscard]] size_t getAmountNamedObjects() const;

    [[nodiscard]] size_t getAmountUniqueObjects() const;

    [[nodiscard]] bool isSanity() const;
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTINFO_H
