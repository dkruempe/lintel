#include "base_library/core/services/SharedMemoryService.h"

#include <base_library/core/models/SharedMemorySegment.h>
#include <base_library/features/base/models/SharedMemorySegmentInfo.h>

SharedMemoryService::SharedMemoryService(
    const std::shared_ptr<SharedMemorySegmentManager>
        &sharedMemorySegmentManager,
    std::shared_ptr<SchedulerService> schedulerService,
    const std::shared_ptr<ProcessName> &processName)
    : AbstractService(processName->getProcessName()),
      m_segments(create(sharedMemorySegmentManager->allOf())),
      m_schedulerService(std::move(schedulerService)) {}

void SharedMemoryService::growOf(
    const std::shared_ptr<SharedMemorySegment> &segment, std::size_t grow) {
  boost::interprocess::managed_mapped_file::grow(segment->getPath().c_str(),
                                                 grow);
}
void SharedMemoryService::shrinkOf(
    const std::shared_ptr<SharedMemorySegment> &segment) {
  boost::interprocess::managed_mapped_file::shrink_to_fit(
      segment->getPath().c_str());
}
SharedMemorySegmentInfo SharedMemoryService::showStateOf(
    const std::shared_ptr<SharedMemorySegment> &segment) const {
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
void SharedMemoryService::onInitialize() {
  m_schedulerService->schedule_at_fixed_rate(m_scheduleRate->getValue(),
                                             m_scheduleRate->getValue(),
                                             [&]() { onCheck(); });
}
void SharedMemoryService::onCheck() {
  if (!m_running) {
    return;
  }
  // check all segments if max size is reached
  for (const auto &[name, mappedFile] : m_segments) {
    const std::shared_ptr<SharedMemorySegment> &sharedMemorySegment =
        mappedFile.m_sharedMemorySegment;
    if (!sharedMemorySegment->isAutoExtend()) {
      continue;
    }
    if (!m_running) {
      return;
    }
    const auto freeMemory = mappedFile.m_managedMappedFile->get_free_memory();
    if (freeMemory < m_autoExtendEpsilon->getValue()) {
      const uint64_t currentSize = mappedFile.m_managedMappedFile->get_size();
      growOf(sharedMemorySegment, sharedMemorySegment->getAutoExtendSize());
      LOG_INFO("{}: extend current {}/{} -> increase by {}", name,
               currentSize - freeMemory, currentSize,
               sharedMemorySegment->getAutoExtendSize());
    }
  }
}
void SharedMemoryService::onShutdown() { m_running = false; }
