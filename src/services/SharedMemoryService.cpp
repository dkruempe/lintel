#include "base_library/services/SharedMemoryService.h"

void SharedMemoryService::growOf(const SharedMemorySegment &segment,
                                 std::size_t grow) {
  boost::interprocess::managed_mapped_file::grow(segment.getPath().c_str(),
                                                 grow);
}
void SharedMemoryService::shrinkOf(const SharedMemorySegment &segment) {
  boost::interprocess::managed_mapped_file::shrink_to_fit(
      segment.getPath().c_str());
}
std::string
SharedMemoryService::showStateOf(const std::string &sharedMemoryName) const {
  try {
    auto &segment = segments.at(sharedMemoryName);
    std::string state = "SharedMemory: " + sharedMemoryName + "\n";
    state += "Sanity: " + std::to_string(segment->check_sanity()) + "\n";
    state += "Size: " + std::to_string(segment->get_size()) + "\n";
    state += "Free: " + std::to_string(segment->get_free_memory()) + "\n";
    state += "Num of Named Objects: " +
             std::to_string(segment->get_num_named_objects()) + "\n";
    state += "Num of Unique Objects: " +
             std::to_string(segment->get_num_unique_objects()) + "\n";
    return state;
  } catch (std::out_of_range &exception) {
    throw ShmSegmentNotFound(sharedMemoryName);
  }
}
SharedMemoryService::SharedMemoryService(
    std::shared_ptr<AbstractShmConfig> abstractShmConfig)
    : segments(create(abstractShmConfig->getSegments())) {}

std::map<std::string, std::shared_ptr<boost::interprocess::managed_mapped_file>>
SharedMemoryService::create(const std::vector<SharedMemorySegment> &set) {
  std::map<std::string,
           std::shared_ptr<boost::interprocess::managed_mapped_file>>
      map;
  std::transform(
      set.begin(), set.end(), std::inserter(map, map.end()),
      [](const SharedMemorySegment &segment)
          -> std::pair<
              std::string,
              std::shared_ptr<boost::interprocess::managed_mapped_file>> {
        return {segment.getName(),
                std::make_shared<boost::interprocess::managed_mapped_file>(
                    boost::interprocess::open_or_create,
                    segment.getPath().c_str(), segment.getSize())};
      });
  return map;
}
std::ostream &operator<<(std::ostream &os, const SharedMemoryService &service) {
  std::for_each(
      service.segments.begin(), service.segments.end(),
      [&](const auto &iter) { os << service.showStateOf(iter.first) << "\n"; });
  return os;
}
SharedMemoryService::ShmString
SharedMemoryService::constructString(const std::string &sharedMemoryName,
                                     const std::string &string) {
  typedef boost::interprocess::allocator<
      ShmString, boost::interprocess::managed_mapped_file::segment_manager>
      StringAllocator;
  try {
    auto &segment = segments.at(sharedMemoryName);
    charAllocator charallocator(segment->get_segment_manager());
    ShmString myString(charallocator);
    myString = string.c_str();
    return myString;
  } catch (std::out_of_range &exception) {
    throw ShmSegmentNotFound(sharedMemoryName);
  }
}
