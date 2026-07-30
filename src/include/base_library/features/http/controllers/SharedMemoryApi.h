#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYAPI_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYAPI_H

#include <memory>

#include "base_library/features/base/controller/SharedMemoryRepositoryDto.h"
#include "base_library/features/base/controller/SharedMemorySegmentDto.h"
#include "base_library/features/http/provider/ClientProvider.h"

/** API client for shared memory management via HTTP */
class SharedMemoryApi {
private:
    std::shared_ptr<Client> m_client;

public:
    explicit SharedMemoryApi(
            const std::shared_ptr<ClientProvider> &clientProvider);

    // segment
    /** @param segmentName regex filter; return all matching segments */
    std::vector<SharedMemorySegmentDto> allSegmentsOf(
            const std::string &segmentName = ".*");

    /** Shrink a shared memory segment by name */
    void shrinkOf(const std::string &segmentName);

    /** Grow a shared memory segment by name and size */
    void growOf(const std::string &segmentName, const std::string &sizeStr);

    // repositories
    /** @return all repositories matching the given filters */
    std::vector<SharedMemoryRepositoryDto> allRepositoriesOf(
            const std::string &repositoryName = ".*",
            const std::string &segmentName = ".*");

    /** Export a repository by UUID; return its content as string */
    std::string repositoryOf(const std::string &uuid);
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYAPI_H
