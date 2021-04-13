#include <base_library/configuration/AbstractShmConfig.h>
#include <base_library/repositories/AbstractShmRepository.h>
#include <base_library/services/SharedMemoryService.h>

#include <iostream>
#include <memory>
#include <utility>

/**
 * Example of an hardcoded configuration for SharedMemorySegments
 * -> note could be easy extended with Xml, json, etc. parser for a file
 * configuration, database, etc..
 */
class SharedMemorySegmentConfiguration : public AbstractShmConfig {
 private:
 public:
  SharedMemorySegmentConfiguration()
      : AbstractShmConfig(
            {
                SharedMemorySegment("/Users/dkruempe/shm", "set_shm",
                                    1ul << 30),
                SharedMemorySegment("/Users/dkruempe/shm", "map_shm",
                                    1ul << 30),
            },
            {{"ShmSet", "set_shm"}, {"ShmMap", "map_shm"}}){};
};

class ShmSet : public AbstractShmRepository {
 public:
  ShmVersion version{0, 1, 1};
  boost::interprocess::set<
      int32_t, std::less<int32_t>,
      boost::interprocess::allocator<
          int32_t, boost::interprocess::managed_mapped_file::segment_manager>>
      &set;
  std::function<void()> convertFunction = [&]() { set.insert(4777); };
  std::vector<Convert> converterFunctions = {
      Convert{ShmVersion{0, 1, 0}, ShmVersion{0, 1, 1}, convertFunction}};

  ShmSet(std::shared_ptr<SharedMemoryService> sharedMemoryService,
         std::shared_ptr<AbstractShmConfig> shmConfig)
      : AbstractShmRepository(std::move(sharedMemoryService),
                              shmConfig->of("ShmSet"), "ShmSet"),
        set(sharedMemoryService->constructSet<int32_t>("set_shm", getName())) {
    checkVersion(version, converterFunctions);
  }

  friend std::ostream &operator<<(std::ostream &os, const ShmSet &set) {
    for (auto &iter : set.set) {
      os << iter << ", ";
    }
    os << set.getCurrentVersion();
    return os;
  }
};

int main(int argc, char *argv[]) {
  std::shared_ptr<SharedMemorySegmentConfiguration> config =
      std::make_shared<SharedMemorySegmentConfiguration>();
  std::shared_ptr<SharedMemoryService> shmService =
      std::make_shared<SharedMemoryService>(config);
  ShmSet shmSet(shmService, config);
  shmSet.set.insert(4711);
  shmSet.set.insert(4712);
  std::cout << shmSet << std::endl;
  return 0;
}