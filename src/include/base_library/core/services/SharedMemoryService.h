#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYSERVICE_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYSERVICE_H

#include <array>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <filesystem>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <utility>

#include "base_library/core/exceptions/ShmSegmentNotFound.h"
#include "base_library/core/models/SharedMemorySegment.h"
#include "base_library/core/services/AbstractService.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/features/base/models/SharedMemorySegmentInfo.h"
#include "base_library/features/base/services/SchedulerService.h"
#include "base_library/features/base/services/SharedMemorySegmentManager.h"
#include "base_library/features/property/models/Property.h"

class SharedMemoryService : public AbstractService<SharedMemoryService> {
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
    DEFINE_PROPERTY(m_scheduleRate, std::chrono::seconds, std::chrono::seconds(2),
                    "schedule rate of user tokens checks in seconds", true);
    DEFINE_PROPERTY(m_autoExtend, bool, true, "auto extend shared memory", true);
    DEFINE_PROPERTY(m_autoExtendEpsilon, std::size_t, 1000,
                    "epsilon when auto extend gets triggered", true);

    // initializer function
    static std::map<std::string, MappedFile> create(
            const std::vector<std::shared_ptr<SharedMemorySegment>> &set);

    void onCheck() const;

public:
    using charAllocator = boost::interprocess::allocator<
            char, boost::interprocess::managed_mapped_file::segment_manager>;
    using ShmString =
            boost::interprocess::basic_string<char, std::char_traits<char>,
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
    boost::interprocess::map<
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
            boost::interprocess::map<
                    Key, Value, std::less<Key>,
                    boost::interprocess::allocator<
                            std::pair<const Key, Value>,
                            boost::interprocess::managed_mapped_file::segment_manager>> *map =
                    segmentCopy.m_managedMappedFile
                            ->find_or_construct<boost::interprocess::map<
                                    Key, Value, std::less<Key>, persistentMapAllocator>>(
                                    name.c_str())(std::less<Key>(), allocator);
            LOG_TRACE("map {}", map->size());
            return *map;
        } catch (std::out_of_range &exception) {
            throw ShmSegmentNotFound(segment->getName());
        }
    }

    template<class Object>
    boost::interprocess::vector<
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
                    boost::interprocess::vector<Object, persistentVectorAllocator>>(
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
