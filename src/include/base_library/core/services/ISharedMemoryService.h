#ifndef CPP_BASE_LIBRARY_ISHAREDMEMORYSERVICE_H
#define CPP_BASE_LIBRARY_ISHAREDMEMORYSERVICE_H

#include <cstddef>
#include <memory>

class SharedMemorySegment;
class SharedMemorySegmentInfo;

class ISharedMemoryService {
public:
    virtual ~ISharedMemoryService() = default;

    virtual SharedMemorySegmentInfo showStateOf(
            const std::shared_ptr<SharedMemorySegment> &segment) const = 0;

    virtual void growOf(const std::shared_ptr<SharedMemorySegment> &segment,
                        std::size_t grow) = 0;

    virtual void shrinkOf(
            const std::shared_ptr<SharedMemorySegment> &segment) = 0;

    virtual void onInitialize() = 0;
};

#endif
