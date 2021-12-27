#include "base_library/features/base/services/SharedMemorySegmentManager.h"

#include <regex>

#include "base_library/core/services/LoggerService.h"
#include "base_library/features/base/configuration/SharedMemorySegmentComponent.h"
#include "base_library/features/base/configuration/SharedMemorySegmentEntry.h"

std::shared_ptr<SharedMemorySegment> SharedMemorySegmentManager::of(
    const std::string& sharedMemorySegmentName) {
  try {
    const auto& sharedMemorySegment =
        m_sharedMemorySegments.at(sharedMemorySegmentName);
    return sharedMemorySegment;
  } catch (std::out_of_range& exception) {
    LOG_ERROR("Shared Memory Segment {} not found {}", sharedMemorySegmentName,
              exception.what());
  }
  return nullptr;
}
SharedMemorySegmentManager::SharedMemorySegmentManager(
    const std::shared_ptr<Configuration>& configuration)
    : m_sharedMemorySegments(init(
          configuration->configurationOf<SharedMemorySegmentComponent>())) {}

std::map<std::string, std::shared_ptr<SharedMemorySegment>>
SharedMemorySegmentManager::init(
    const std::vector<std::shared_ptr<Entry>>& entries) {
  std::map<std::string, std::shared_ptr<SharedMemorySegment>> map;
  for (const auto& entry : entries) {
    std::shared_ptr<SharedMemorySegmentEntry> temp =
        std::static_pointer_cast<SharedMemorySegmentEntry>(entry);
    if (temp->getSharedMemoryPath() != nullptr) {
      continue;
    }
    map.insert({temp->getSharedMemorySegment()->getName(),
                temp->getSharedMemorySegment()});
  }
  return map;
}
std::vector<std::shared_ptr<SharedMemorySegment>>
SharedMemorySegmentManager::allOf(const std::string& segmentName) {
  std::regex segmentRegex(segmentName);
  std::vector<std::shared_ptr<SharedMemorySegment>> segments;
  for (const auto& [name, segment] : m_sharedMemorySegments) {
    if (!std::regex_match(segment->getName(), segmentRegex)) {
      continue;
    }
    segments.push_back(segment);
  }
  return segments;
}
