#include "base_library/core/services/SharedMemoryService.h"

#include <base_library/core/models/SharedMemorySegment.h>
#include <base_library/features/base/models/SharedMemorySegmentInfo.h>

SharedMemoryService::SharedMemoryService(
        const std::shared_ptr<SharedMemorySegmentManager>
        &sharedMemorySegmentManager,
        std::shared_ptr<SchedulerService> schedulerService,
        const std::shared_ptr<ProcessName> &processName)
        : PropertyRegistration(processName->getProcessName()),
          m_segments(create(sharedMemorySegmentManager->allOf())),
          m_schedulerService(std::move(schedulerService)) {
    m_scheduleRate = registerProperty<std::chrono::seconds>(
            "m_scheduleRate", std::chrono::seconds(2),
            "schedule rate of user tokens checks in seconds", true,
            __FILE__, __LINE__);
    m_autoExtend = registerProperty<bool>(
            "m_autoExtend", true, "auto extend shared memory", true,
            __FILE__, __LINE__);
    m_autoExtendEpsilon = registerProperty<std::size_t>(
            "m_autoExtendEpsilon", static_cast<std::size_t>(1000),
            "epsilon when auto extend gets triggered", true,
            __FILE__, __LINE__);
}

void SharedMemoryService::growOf(
        const std::shared_ptr<SharedMemorySegment> &segment, std::size_t grow) {
    std::lock_guard<std::mutex> lock(m_segmentsMutex);
    auto found = m_segments.find(segment->getName());
    if (found == m_segments.end()) {
        throw ShmSegmentNotFound(segment->getName());
    }
    if (grow == 0) {
        return;
    }
    const std::size_t maxSize = segment->getMaxSize();
    if (maxSize != 0) {
        const std::size_t currentSize =
                found->second.m_managedMappedFile->get_size();
        if (currentSize >= maxSize) {
            throw std::runtime_error("shared memory segment '" +
                                     segment->getName() +
                                     "' already at its maximum size");
        }
        // cap the request to the remaining headroom to avoid overflow and
        // to respect the configured resource limit
        const std::size_t headroom = maxSize - currentSize;
        if (grow > headroom) {
            grow = headroom;
        }
    }
    // The static grow() extends the file and updates the segment-manager header
    // stored inside the file. The live mapping's length, however, is fixed at
    // mmap() time, so it must be reopened to observe the larger file.
    if (!boost::interprocess::managed_mapped_file::grow(segment->getPath().c_str(),
                                                       grow)) {
        throw std::runtime_error("failed to grow shared memory segment '" +
                                 segment->getName() + "'");
    }
    // Reopen the segment so the new mapping covers the grown file. This
    // invalidates references into the old mapping - repositories detect the
    // bump of the generation counter and re-fetch their references.
    found->second.m_managedMappedFile =
            std::make_shared<boost::interprocess::managed_mapped_file>(
                    boost::interprocess::open_only, segment->getPath().c_str());
    ++m_generation;
}

void SharedMemoryService::shrinkOf(
        const std::shared_ptr<SharedMemorySegment> &segment) {
    std::lock_guard<std::mutex> lock(m_segmentsMutex);
    auto found = m_segments.find(segment->getName());
    if (found == m_segments.end()) {
        throw ShmSegmentNotFound(segment->getName());
    }
    if (!boost::interprocess::managed_mapped_file::shrink_to_fit(
                segment->getPath().c_str())) {
        throw std::runtime_error("failed to shrink shared memory segment '" +
                                 segment->getName() + "'");
    }
    // Reopen the segment so the new mapping covers the shrunk file.
    found->second.m_managedMappedFile =
            std::make_shared<boost::interprocess::managed_mapped_file>(
                    boost::interprocess::open_only, segment->getPath().c_str());
    ++m_generation;
}

std::size_t SharedMemoryService::getGeneration() const {
    std::lock_guard<std::mutex> lock(m_segmentsMutex);
    return m_generation;
}

SharedMemorySegmentInfo SharedMemoryService::showStateOf(
        const std::shared_ptr<SharedMemorySegment> &segment) const {
    std::lock_guard<std::mutex> lock(m_segmentsMutex);
    auto found = m_segments.find(segment->getName());
    if (found == m_segments.end()) {
        throw ShmSegmentNotFound(segment->getName());
    }
    std::size_t currentSize = found->second.m_managedMappedFile->get_size();
    std::size_t freeSize = found->second.m_managedMappedFile->get_free_memory();
    std::size_t namedObjects =
            found->second.m_managedMappedFile->get_num_named_objects();
    std::size_t uniqueObjects =
            found->second.m_managedMappedFile->get_num_unique_objects();
    bool sanity = found->second.m_managedMappedFile->check_sanity();

    SharedMemorySegmentInfo info(segment, currentSize, freeSize, namedObjects,
                                 uniqueObjects, sanity);
    return info;
}

std::map<std::string, SharedMemoryService::MappedFile>
SharedMemoryService::create(
        const std::vector<std::shared_ptr<SharedMemorySegment>> &set) {
    std::map<std::string, MappedFile> map;
    std::transform(
            set.begin(), set.end(), std::inserter(map, map.end()),
            [](const std::shared_ptr<SharedMemorySegment> &segment)
                    -> std::pair<std::string, MappedFile> {
                MappedFile mappedFile;
                mappedFile.m_managedMappedFile =
                        std::make_shared<boost::interprocess::managed_mapped_file>(
                                boost::interprocess::open_or_create, segment->getPath().c_str(),
                                segment->getSize());
                mappedFile.m_sharedMemorySegment = segment;
                return {segment->getName(), mappedFile};
            });
    return map;
}

SharedMemoryService::ShmString SharedMemoryService::constructString(
        const std::shared_ptr<SharedMemorySegment> &segment,
        const std::string &name) {
    std::lock_guard<std::mutex> lock(m_segmentsMutex);
    try {
        auto &segmentCopy = m_segments.at(segment->getName());
        charAllocator charallocator(
                segmentCopy.m_managedMappedFile->get_segment_manager());
        ShmString myString(charallocator);
        myString = name.c_str();
        return myString;
    } catch (std::out_of_range &exception) {
        throw ShmSegmentNotFound(segment->getName());
    }
}

boost::interprocess::managed_mapped_file::segment_manager *
SharedMemoryService::getSegmentManager(
        const std::shared_ptr<SharedMemorySegment> &segment) {
    std::lock_guard<std::mutex> lock(m_segmentsMutex);
    auto found = m_segments.find(segment->getName());
    if (found == m_segments.end()) {
        return nullptr;
    }
    return found->second.m_managedMappedFile->get_segment_manager();
}

void SharedMemoryService::onInitialize() {
    std::weak_ptr<SharedMemoryService> weakSelf = shared_from_this();
    m_schedulerService->schedule_at_fixed_rate(m_scheduleRate->getValue(),
                                               m_scheduleRate->getValue(),
                                               [weakSelf]() {
                                                   if (auto self = weakSelf.lock()) {
                                                       self->onCheck();
                                                   }
                                               });
}

void SharedMemoryService::onCheck() {
    if (!m_autoExtend->getValue()) {
        return;
    }
    std::vector<std::shared_ptr<SharedMemorySegment>> segments;
    {
        std::lock_guard<std::mutex> lock(m_segmentsMutex);
        segments.reserve(m_segments.size());
        for (const auto &entry : m_segments) {
            segments.push_back(entry.second.m_sharedMemorySegment);
        }
    }
    for (const auto &segment : segments) {
        if (!segment->isAutoExtend()) {
            continue;
        }
        std::size_t freeMemory = 0;
        std::size_t currentSize = 0;
        try {
            std::lock_guard<std::mutex> lock(m_segmentsMutex);
            auto found = m_segments.find(segment->getName());
            if (found == m_segments.end()) {
                continue;
            }
            freeMemory = found->second.m_managedMappedFile->get_free_memory();
            currentSize = found->second.m_managedMappedFile->get_size();
        } catch (const std::exception &exception) {
            LOG_ERROR("{}: reading state failed: {}", segment->getName(),
                      exception.what());
            continue;
        }
        if (freeMemory < m_autoExtendEpsilon->getValue()) {
            const auto maxSize = segment->getMaxSize();
            const auto autoExtendSize = segment->getAutoExtendSize();
            // compute the grow size without overflowing the addition in
            // `currentSize + autoExtendSize`
            std::size_t grow = autoExtendSize;
            if (maxSize != 0) {
                if (currentSize >= maxSize) {
                    LOG_WARN("{}: segment reached its maximum size {}/{}",
                             segment->getName(), currentSize, maxSize);
                    continue;
                }
                const std::size_t headroom = maxSize - currentSize;
                if (grow == 0 || grow > headroom) {
                    grow = headroom;
                }
            }
            if (grow == 0) {
                continue;
            }
            try {
                growOf(segment, grow);
                LOG_INFO("{}: extend current {}/{} -> increase by {}",
                         segment->getName(), currentSize - freeMemory,
                         currentSize, grow);
            } catch (const std::exception &exception) {
                LOG_ERROR("{}: auto extend failed: {}", segment->getName(),
                          exception.what());
            }
        }
    }
}