#include <cstddef>
#include <cstdint>
#include <string>

#include "lintel/features/base/controller/HistoryDtos.h"
#include "lintel/features/base/controller/MessageQueueDtos.h"
#include "lintel/features/base/controller/ProcessGroupsDto.h"
#include "lintel/features/base/controller/SharedMemoryRepositoriesDto.h"
#include "lintel/features/base/controller/SharedMemorySegmentsDto.h"
#include "lintel/features/property/controller/PropertiesDto.h"

// Feed the same bytes through every JSON DTO deserializer. Each one parses an
// untrusted HTTP body, so a crash or throw here is reachable from the network.
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, const std::size_t size)
{
  std::string input;
  if (size > 0) { input.assign(reinterpret_cast<const char *>(data), size); }
  try {
    lintel::HistoryDtos history;
    history.deserialize(input);

    lintel::MessageQueueDtos messageQueues;
    messageQueues.deserialize(input);

    lintel::MessageQueuesPageDto messageQueuePage;
    messageQueuePage.deserialize(input);

    lintel::ProcessGroupsDto processGroups;
    processGroups.deserialize(input);

    lintel::SharedMemoryRepositoriesDto sharedMemoryRepositories;
    sharedMemoryRepositories.deserialize(input);

    lintel::SharedMemorySegmentsDto sharedMemorySegments;
    sharedMemorySegments.deserialize(input);

    lintel::PropertiesDto properties;
    properties.deserialize(input);
  } catch (...) {
    // Non-throwing is the contract; a throw is a finding the fuzzer surfaces,
    // but it must not abort the whole run.
  }
  return 0;
}
