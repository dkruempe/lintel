#ifndef LINTEL_SHAREDMEMORYAPI_H
#define LINTEL_SHAREDMEMORYAPI_H

#include <cstddef>
#include <memory>
#include <optional>

#include "lintel/features/base/controller/SharedMemoryRepositoryDto.h"
#include "lintel/features/base/controller/SharedMemorySegmentDto.h"
#include "lintel/features/base/models/SharedMemorySegmentStats.h"
#include "lintel/features/http/provider/ClientProvider.h"

namespace lintel {
/** Counts returned by the shared memory cleanup endpoint.
 *  The endpoint only reports numbers, not the individual orphaned objects. */
struct SharedMemoryCleanupSummary
{
  /** true when the run only reported and did not remove anything */
  bool dryRun = false;
  /** number of registry entries inspected */
  std::size_t scanned = 0;
  /** orphaned objects found by this run */
  std::size_t orphans = 0;
  /** orphaned objects actually removed by this run */
  std::size_t removed = 0;
  /** removals that failed and are retried by the next run */
  std::size_t failed = 0;
};

/** API client for shared memory management via HTTP */
class SharedMemoryApi
{
private:
  std::shared_ptr<Client> m_client;

public:
  explicit SharedMemoryApi(const std::shared_ptr<ClientProvider> &clientProvider);

  // segment
  /** @param segmentName regex filter; return all matching segments */
  std::vector<SharedMemorySegmentDto> allSegmentsOf(const std::string &segmentName = ".*");

  /** Shrink a shared memory segment by name */
  void shrinkOf(const std::string &segmentName);

  /** Grow a shared memory segment by name and size */
  void growOf(const std::string &segmentName, const std::string &sizeStr);

  /**
   * Read the runtime statistics of one segment.
   * @param segmentName exact segment name
   * @return the statistics, or nullopt when the segment is unknown or the request was rejected
   */
  std::optional<SharedMemorySegmentStats> statsOf(const std::string &segmentName);

  /**
   * Remove the shared memory objects whose owner process is gone.
   * @return the counts of the run, or nullopt when the request was rejected
   */
  std::optional<SharedMemoryCleanupSummary> cleanupOf();

  // repositories
  /** @return all repositories matching the given filters */
  std::vector<SharedMemoryRepositoryDto> allRepositoriesOf(const std::string &repositoryName = ".*",
    const std::string &segmentName = ".*");

  /** Export a repository by UUID; return its content as string */
  std::string repositoryOf(const std::string &uuid);
};

}// namespace lintel
#endif// LINTEL_SHAREDMEMORYAPI_H
