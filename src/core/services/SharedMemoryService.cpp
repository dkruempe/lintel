#include "base_library/core/services/SharedMemoryService.h"

SharedMemoryService::SharedMemoryService(
    const std::shared_ptr<SharedMemorySegmentManager>
        &sharedMemorySegmentManager,
    std::shared_ptr<SchedulerService> schedulerService,
    const std::shared_ptr<ProcessName> &processName)
    : AbstractService(processName->getProcessName()),
      m_segments(create(sharedMemorySegmentManager->allOf())),
      m_schedulerService(std::move(schedulerService)) {}
void SharedMemoryService::growOf(const SharedMemorySegment &segment,
                                 std::size_t grow) {
  boost::interprocess::managed_mapped_file::grow(segment.getPath().c_str(),
                                                 grow);
}
void SharedMemoryService::shrinkOf(const SharedMemorySegment &segment) {
  boost::interprocess::managed_mapped_file::shrink_to_fit(
      segment.getPath().c_str());
}
std::string SharedMemoryService::showStateOf(
    const std::string &sharedMemoryName) const {
  try {
    auto &segment = m_segments.at(sharedMemoryName);
    std::string state = "SharedMemory: " + sharedMemoryName + "\n";
    state += "Sanity: " +
             std::to_string(segment.m_managedMappedFile->check_sanity()) + "\n";
    state +=
        "Size: " + std::to_string(segment.m_managedMappedFile->get_size()) +
        "\n";
    state += "Free: " +
             std::to_string(segment.m_managedMappedFile->get_free_memory()) +
             "\n";
    state +=
        "Num of Named Objects: " +
        std::to_string(segment.m_managedMappedFile->get_num_named_objects()) +
        "\n";
    state +=
        "Num of Unique Objects: " +
        std::to_string(segment.m_managedMappedFile->get_num_unique_objects()) +
        "\n";
    return state;
  } catch (std::out_of_range &exception) {
    throw ShmSegmentNotFound(sharedMemoryName);
  }
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
std::ostream &operator<<(std::ostream &os, const SharedMemoryService &service) {
  std::for_each(
      service.m_segments.begin(), service.m_segments.end(),
      [&](const auto &iter) { os << service.showStateOf(iter.first) << "\n"; });
  return os;
}
SharedMemoryService::ShmString SharedMemoryService::constructString(
    const std::string &sharedMemoryName, const std::string &string) {
  try {
    auto &segment = m_segments.at(sharedMemoryName);
    charAllocator charallocator(
        segment.m_managedMappedFile->get_segment_manager());
    ShmString myString(charallocator);
    myString = string.c_str();
    return myString;
  } catch (std::out_of_range &exception) {
    throw ShmSegmentNotFound(sharedMemoryName);
  }
}
void SharedMemoryService::onInitialize() {
  m_schedulerService->schedule_after(m_scheduleRate->getValue(),
                                     [&]() { onCheck(); });
}
void SharedMemoryService::onCheck() {
  // check all segments if max size is reached
  for (const auto &[name, mappedFile] : m_segments) {
    const std::shared_ptr<SharedMemorySegment> &sharedMemorySegment =
        mappedFile.m_sharedMemorySegment;
    if (!sharedMemorySegment->isAutoExtend()) {
      continue;
    }
    const auto freeMemory = mappedFile.m_managedMappedFile->get_free_memory();
    if (freeMemory < m_autoExtendEpsilon->getValue()) {
      const unsigned long currentSize =
          mappedFile.m_managedMappedFile->get_size();
      growOf(*sharedMemorySegment, sharedMemorySegment->getAutoExtendSize());
      LOG_INFO("{}: extend current {}/{} -> increase by {}", name,
               currentSize - freeMemory, currentSize,
               sharedMemorySegment->getAutoExtendSize());
    }
  }
  if (m_running) {
    m_schedulerService->schedule_after(m_scheduleRate->getValue(),
                                       [&]() { onCheck(); });
  }
}
void SharedMemoryService::onShutdown() { m_running.store(false); }