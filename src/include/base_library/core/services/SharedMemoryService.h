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
#include <ostream>
#include <set>
#include <utility>

#include "base_library/core/exceptions/ShmSegmentNotFound.h"
#include "base_library/core/models/SharedMemorySegment.h"
#include "base_library/core/services/PropertyRegistration.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/features/base/models/SharedMemorySegmentInfo.h"
#include "base_library/features/base/services/SchedulerService.h"
#include "base_library/features/base/services/SharedMemorySegmentManager.h"

class SharedMemoryService : public PropertyRegistration<SharedMemoryService> {
private:
    struct MappedFile {
        std::shared_ptr<boost::interprocess::managed_mapped_file>
                m_managedMappedFile;
        std::shared_ptr<SharedMemorySegment> m_sharedMemorySegment;
    };
    // variables
    std::map<std::string, MappedFile> m_segments;
    std::shared_ptr<SchedulerService> m_schedulerService;
    // properties
    std::shared_ptr<Property<std::chrono::seconds>> m_scheduleRate;
    std::shared_ptr<Property<bool>> m_autoExtend;
    std::shared_ptr<Property<std::size_t>> m_autoExtendEpsilon;

    // initializer function
    static std::map<std::string, MappedFile> create(
            const std::vector<std::shared_ptr<SharedMemorySegment>> &set);

    void onCheck() const;

public:
    using charAllocator = boost::interprocess::allocator<
            char, boost::interprocess::managed_mapped_file::segment_manager>;
    using ShmString =
            boost::container::basic_string<char, std::char_traits<char>,
                    charAllocator>;

    SharedMemoryService(const std::shared_ptr<SharedMemorySegmentManager>
                        &sharedMemorySegmentManager,
                        std::shared_ptr<SchedulerService> schedulerService,
                        const std::shared_ptr<ProcessName> &processName);

    ~SharedMemoryService() override = default;

    void onInitialize() override;

    /**
     * grows the size of the mentioned shared memory block
     * @param sharedMemoryName
     * @param grow size to be added to shared memory itself
     */
    static void growOf(const std::shared_ptr<SharedMemorySegment> &segment,
                       std::size_t grow);

    /**
     * shrinks the shared memory size to the minimum
     * @param sharedMemoryName
     */
    static void shrinkOf(const std::shared_ptr<SharedMemorySegment> &segment);

    /**
     * shows state of the mentioned shared memory segment
     * @param segment
     * @return string with the printed information
     */
    [[nodiscard]] SharedMemorySegmentInfo showStateOf(
            const std::shared_ptr<SharedMemorySegment> &segment) const;

    template<class Object, std::size_t Size>
    std::array<Object, Size> &constructArray(
            const std::shared_ptr<SharedMemorySegment> &segment,
            const std::string &name) {
        try {
            auto &segmentCopy = m_segments.at(segment->getName());
            return *(
                    segmentCopy.m_managedMappedFile
                            ->find_or_construct<std::array<Object, Size>>(name.c_str())());
        } catch (std::out_of_range &exception) {
            throw ShmSegmentNotFound(segment->getName());
        }
    }

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

    template<class Object>
    boost::container::vector<
            Object,
            boost::interprocess::allocator<
                    Object, boost::interprocess::managed_mapped_file::segment_manager>>
    &constructVector(const std::shared_ptr<SharedMemorySegment> &segment,
                     const std::string &name) {
        using persistentVectorAllocator = boost::interprocess::allocator<
                Object, boost::interprocess::managed_mapped_file::segment_manager>;
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

    template<class Object>
    Object &constructObject(const std::shared_ptr<SharedMemorySegment> &segment,
                            const std::string &name) {
        try {
            auto &segmentCopy = m_segments.at(segment->getName());
            return *(segmentCopy.m_managedMappedFile->find_or_construct<Object>(
                    name.c_str())());
        } catch (std::out_of_range &exception) {
            throw ShmSegmentNotFound(segment->getName());
        }
    }

    ShmString constructString(const std::shared_ptr<SharedMemorySegment> &segment,
                              const std::string &name);
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYSERVICE_H
