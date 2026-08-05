#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYSERVICE_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYSERVICE_H

#include <array>
#include <boost/container/map.hpp>
#include <boost/container/string.hpp>
#include <boost/container/vector.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <set>
#include <utility>

#include "base_library/core/exceptions/ShmSegmentNotFound.h"
#include "base_library/core/models/SharedMemorySegment.h"
#include "base_library/core/services/ISharedMemoryService.h"
#include "base_library/core/services/LoggerMacros.h"
#include "base_library/core/services/PropertyRegistration.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/features/base/models/SharedMemorySegmentInfo.h"
#include "base_library/features/base/services/SchedulerService.h"
#include "base_library/features/base/services/SharedMemorySegmentManager.h"

/** Service for managing shared memory segments and constructing data structures within them. */
class SharedMemoryService : public PropertyRegistration<SharedMemoryService>,
                            public ISharedMemoryService,
                            public std::enable_shared_from_this<SharedMemoryService> {
private:
    struct MappedFile {
        std::shared_ptr<boost::interprocess::managed_mapped_file>
                m_managedMappedFile;
        std::shared_ptr<SharedMemorySegment> m_sharedMemorySegment;
    };
    // variables
    std::map<std::string, MappedFile> m_segments;
    mutable std::mutex m_segmentsMutex;
    std::size_t m_generation = 0;
    std::shared_ptr<SchedulerService> m_schedulerService;
    // properties
    std::shared_ptr<Property<std::chrono::seconds>> m_scheduleRate;
    std::shared_ptr<Property<bool>> m_autoExtend;
    std::shared_ptr<Property<std::size_t>> m_autoExtendEpsilon;

    // initializer function
    static std::map<std::string, MappedFile> create(
            const std::vector<std::shared_ptr<SharedMemorySegment>> &set);

    void onCheck();

public:
    using charAllocator = boost::interprocess::allocator<
            char, boost::interprocess::managed_mapped_file::segment_manager>;
    using ShmString =
            boost::container::basic_string<char, std::char_traits<char>,
                    charAllocator>;

    /** Construct a SharedMemoryService.
     * @param sharedMemorySegmentManager the segment manager providing segment definitions
     * @param schedulerService           the scheduler for periodic maintenance tasks
     * @param processName                the process name */
    SharedMemoryService(const std::shared_ptr<SharedMemorySegmentManager>
                        &sharedMemorySegmentManager,
                        std::shared_ptr<SchedulerService> schedulerService,
                        const std::shared_ptr<ProcessName> &processName);

    ~SharedMemoryService() override = default;

    /** Initialize all shared memory segments by opening or creating them. */
    void onInitialize() override;

    /** Generation counter incremented on every grow/shrink. Repositories use it
     * to detect that a segment was remapped and re-fetch their references.
     * @return the current generation */
    [[nodiscard]] std::size_t getGeneration() const;

    /** grows the size of the mentioned shared memory block */
    void growOf(const std::shared_ptr<SharedMemorySegment> &segment,
                std::size_t grow) override;

    /** shrinks the shared memory size to the minimum */
    void shrinkOf(const std::shared_ptr<SharedMemorySegment> &segment) override;

    /**
     * shows state of the mentioned shared memory segment
     * @param segment
     * @return string with the printed information
     */
    [[nodiscard]] SharedMemorySegmentInfo showStateOf(
            const std::shared_ptr<SharedMemorySegment> &segment) const override;

    /** Find or construct a fixed-size array in the given shared memory segment.
     * @tparam Object the element type
     * @tparam Size   the array size
     * @param segment the shared memory segment
     * @param name    the name of the array object
     * @return reference to the constructed or found array
     * @throws ShmSegmentNotFound if the segment does not exist */
    template<class Object, std::size_t Size>
    std::array<Object, Size> &constructArray(
            const std::shared_ptr<SharedMemorySegment> &segment,
            const std::string &name) {
        std::lock_guard<std::mutex> lock(m_segmentsMutex);
        try {
            auto &segmentCopy = m_segments.at(segment->getName());
            return *(
                    segmentCopy.m_managedMappedFile
                            ->find_or_construct<std::array<Object, Size>>(name.c_str())());
        } catch (std::out_of_range &exception) {
            throw ShmSegmentNotFound(segment->getName());
        }
    }

    /** Find or construct a map in the given shared memory segment.
     * @tparam Key   the map key type
     * @tparam Value the map value type
     * @param segment the shared memory segment
     * @param name    the name of the map object
     * @return reference to the constructed or found map
     * @throws ShmSegmentNotFound if the segment does not exist */
    template<class Key, class Value>
    boost::container::map<
            Key, Value, std::less<Key>,
            boost::interprocess::allocator<
                    std::pair<const Key, Value>,
                    boost::interprocess::managed_mapped_file::segment_manager>>
    &constructMap(const std::shared_ptr<SharedMemorySegment> &segment,
                  const std::string &name) {
        using pairType = std::pair<const Key, Value>;
        using persistentMapAllocator = boost::interprocess::allocator<
                pairType, boost::interprocess::managed_mapped_file::segment_manager>;
        std::lock_guard<std::mutex> lock(m_segmentsMutex);
        try {
            auto &segmentCopy = m_segments.at(segment->getName());
            persistentMapAllocator allocator(
                    segmentCopy.m_managedMappedFile->get_segment_manager());
            boost::container::map<
                    Key, Value, std::less<Key>,
                    boost::interprocess::allocator<
                            std::pair<const Key, Value>,
                            boost::interprocess::managed_mapped_file::segment_manager>> *map =
                    segmentCopy.m_managedMappedFile
                            ->find_or_construct<boost::container::map<
                                    Key, Value, std::less<Key>, persistentMapAllocator>>(
                                    name.c_str())(std::less<Key>(), allocator);
            LOG_TRACE("map {}", map->size());
            return *map;
        } catch (std::out_of_range &exception) {
            throw ShmSegmentNotFound(segment->getName());
        }
    }

    /** Find or construct a vector in the given shared memory segment.
     * @tparam Object the element type
     * @param segment the shared memory segment
     * @param name    the name of the vector object
     * @return reference to the constructed or found vector
     * @throws ShmSegmentNotFound if the segment does not exist */
    template<class Object>
    boost::container::vector<
            Object,
            boost::interprocess::allocator<
                    Object, boost::interprocess::managed_mapped_file::segment_manager>>
    &constructVector(const std::shared_ptr<SharedMemorySegment> &segment,
                     const std::string &name) {
        using persistentVectorAllocator = boost::interprocess::allocator<
                Object, boost::interprocess::managed_mapped_file::segment_manager>;
        std::lock_guard<std::mutex> lock(m_segmentsMutex);
        try {
            auto &segmentCopy = m_segments.at(segment->getName());
            persistentVectorAllocator allocator(
                    segmentCopy.m_managedMappedFile->get_segment_manager());
            return *(segmentCopy.m_managedMappedFile->find_or_construct<
                    boost::container::vector<Object, persistentVectorAllocator>>(
                    name.c_str())(allocator));
        } catch (std::out_of_range &exception) {
            throw ShmSegmentNotFound(segment->getName());
        }
    }

    /** Find or construct a single object in the given shared memory segment.
     * @tparam Object the object type
     * @param segment the shared memory segment
     * @param name    the name of the object
     * @return reference to the constructed or found object
     * @throws ShmSegmentNotFound if the segment does not exist */
    template<class Object>
    Object &constructObject(const std::shared_ptr<SharedMemorySegment> &segment,
                            const std::string &name) {
        std::lock_guard<std::mutex> lock(m_segmentsMutex);
        try {
            auto &segmentCopy = m_segments.at(segment->getName());
            return *(segmentCopy.m_managedMappedFile->find_or_construct<Object>(
                    name.c_str())());
        } catch (std::out_of_range &exception) {
            throw ShmSegmentNotFound(segment->getName());
        }
    }

    /** Find or construct a string in the given shared memory segment.
     * @param segment the shared memory segment
     * @param name    the name of the string object
     * @return the constructed or found shared-memory string */
    ShmString constructString(const std::shared_ptr<SharedMemorySegment> &segment,
                              const std::string &name);
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYSERVICE_H
