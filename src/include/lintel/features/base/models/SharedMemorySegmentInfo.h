#ifndef LINTEL_SHAREDMEMORYSEGMENTINFO_H
#define LINTEL_SHAREDMEMORYSEGMENTINFO_H

#include <memory>

#include "lintel/core/models/SharedMemorySegment.h"

/**
 * Provides information about a shared memory segment, including its current size, free space, and object counts.
 */
class SharedMemorySegmentInfo {
private:
    std::shared_ptr<SharedMemorySegment> m_sharedMemorySegment;
    std::size_t m_currentSize;
    std::size_t m_freeSize;
    std::size_t m_amountNamedObjects;
    std::size_t m_amountUniqueObjects;
    bool m_sanity;

public:
    /**
     * Constructor.
     * @param sharedMemorySegment the underlying shared memory segment
     * @param currentSize currently used size in bytes
     * @param freeSize free space in bytes
     * @param amountNamedObjects number of named objects in the segment
     * @param amountUniqueObjects number of unique (unnamed) objects
     * @param sanity whether the segment passes sanity checks
     */
    SharedMemorySegmentInfo(
            std::shared_ptr<SharedMemorySegment> sharedMemorySegment,
            size_t currentSize, size_t freeSize, size_t amountNamedObjects,
            size_t amountUniqueObjects, bool sanity);

    /** @return the underlying shared memory segment */
    [[nodiscard]] const std::shared_ptr<SharedMemorySegment> &
    getSharedMemorySegment() const;

    /** @return currently used size in bytes */
    [[nodiscard]] constexpr size_t getCurrentSize() const { return m_currentSize; }

    /** @return free space in bytes */
    [[nodiscard]] constexpr size_t getFreeSize() const { return m_freeSize; }

    /** @return number of named objects */
    [[nodiscard]] constexpr size_t getAmountNamedObjects() const { return m_amountNamedObjects; }

    /** @return number of unique (unnamed) objects */
    [[nodiscard]] constexpr size_t getAmountUniqueObjects() const { return m_amountUniqueObjects; }

    /** @return true if the segment passes sanity checks */
    [[nodiscard]] constexpr bool isSanity() const { return m_sanity; }
};

#endif  // LINTEL_SHAREDMEMORYSEGMENTINFO_H
