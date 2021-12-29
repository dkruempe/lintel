#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYAPI_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYAPI_H

#include <memory>

#include "base_library/features/base/controller/SharedMemoryRepositoryDto.h"
#include "base_library/features/base/controller/SharedMemorySegmentDto.h"
#include "base_library/features/http/provider/ClientProvider.h"

class SharedMemoryApi {
 private:
  std::shared_ptr<Client> m_client;

 public:
  explicit SharedMemoryApi(
      const std::shared_ptr<ClientProvider> &clientProvider);

  // segment
  std::vector<SharedMemorySegmentDto> allSegmentsOf(
      const std::string &segmentName = ".*");
  void shrinkOf(const std::string &segmentName);
  void growOf(const std::string &segmentName, const std::string &sizeStr);
  // repositories
  std::vector<SharedMemoryRepositoryDto> allRepositoriesOf(
      const std::string &repositoryName = ".*",
      const std::string &segmentName = ".*");
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYAPI_H
