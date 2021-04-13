#include "base_library/configuration/AbstractShmConfig.h"

#include <algorithm>
#include <utility>

#include "base_library/exceptions/ConfigShmSegmentNotFound.h"
#include "base_library/exceptions/ShmSegmentNotFound.h"
const std::vector<SharedMemorySegment> &AbstractShmConfig::getSegments() const {
  return segments;
}

const SharedMemorySegment &
AbstractShmConfig::of(const std::string_view &repository) const {
  try {
    return map.at(repository);
  } catch (std::out_of_range &exception) {
    throw ShmSegmentNotFound(repository);
  }
}
AbstractShmConfig::AbstractShmConfig(
    std::vector<SharedMemorySegment> segments,
    const std::map<std::string_view, std::string_view> &map)
    : segments(std::move(segments)), map(createMap(this->segments, map)) {}

std::map<std::string_view, SharedMemorySegment> AbstractShmConfig::createMap(
    std::vector<SharedMemorySegment> &segments,
    const std::map<std::string_view, std::string_view> &map) {
  std::map<std::string_view, SharedMemorySegment> nameToSegmentMap;
  std::transform(segments.begin(), segments.end(),
                 std::inserter(nameToSegmentMap, nameToSegmentMap.begin()),
                 [](const SharedMemorySegment &segment)
                     -> std::pair<std::string_view, SharedMemorySegment> {
                   return {segment.getName(), segment};
                 });
  std::map<std::string_view, SharedMemorySegment> repositoryToSegmentMap;
  std::transform(
      map.begin(), map.end(),
      std::inserter(repositoryToSegmentMap, repositoryToSegmentMap.end()),
      [&nameToSegmentMap](
          const std::pair<std::string_view, std::string_view> &pair)
          -> std::pair<std::string_view, SharedMemorySegment> {
        try {
          return {pair.first, nameToSegmentMap.at(pair.second)};
        } catch (std::out_of_range &exception) {
          throw ConfigShmSegmentNotFound(pair.second);
        }
      });
  return repositoryToSegmentMap;
}
void AbstractShmConfig::set(
    std::vector<SharedMemorySegment> segmentsIn,
    const std::map<std::string_view, std::string_view> &mapIn) {
  this->segments = std::move(segmentsIn);
  this->map = createMap(this->segments, mapIn);
}
