#ifndef LINTEL_SHAREDMEMORYCONTROLLER_H
#define LINTEL_SHAREDMEMORYCONTROLLER_H

#include "lintel/core/services/ISharedMemoryService.h"
#include "lintel/features/base/repositories/SharedMemoryRepository.h"
#include "lintel/features/base/services/IAuthService.h"
#include "lintel/features/base/services/ISharedMemorySegmentManager.h"
#include "lintel/features/base/services/SharedMemoryCleanupService.h"
#include "lintel/features/http/service/ContentType.h"
#include "lintel/features/http/service/Controller.h"

namespace lintel {
/** HTTP controller for shared memory segment and repository management */
class SharedMemoryController : public Controller
{
private:
  std::shared_ptr<ISharedMemoryService> m_sharedMemoryService;
  std::shared_ptr<ISharedMemorySegmentManager> m_sharedMemorySegmentManager;
  std::shared_ptr<SharedMemoryCleanupService> m_sharedMemoryCleanupService;
  std::vector<std::shared_ptr<SharedMemoryRepository>> m_sharedMemoryRepositories;
  std::map<std::string, std::shared_ptr<SharedMemoryRepository>> m_uuidSharedMemoryRepositories;
  Group m_adminGroup;
  Group m_userGroup;

  /** Build UUID-to-repository map from repository vector */
  static std::map<std::string, std::shared_ptr<SharedMemoryRepository>> build(
    const std::vector<std::shared_ptr<SharedMemoryRepository>> &repositories);

  /** @return true if the user may read segment information */
  [[nodiscard]] bool canRead(const std::optional<UserToken> &user) const;

  /** @return true if the user may mutate segment or cleanup state */
  [[nodiscard]] bool canAdminister(const std::optional<UserToken> &user) const;

public:
  // Shared Memory Segment handler methods
  ADD_HANDLER_METHOD(R"(/shm/segments/([^\/]+))", Get, allSegmentsOf);
  ADD_HANDLER_METHOD(R"(/shm/segments/shrink/([^\/]+))", Put, shrinkSegmentOf);
  ADD_HANDLER_METHOD(R"(/shm/segments/grow/([^\/]+)/([^\/]+))", Put, growSegmentOf);

  // Shared Memory Repository handler methods
  ADD_HANDLER_METHOD(R"(/shm/repositories/([^\/]+)/([^\/]+))", Get, allRepositoriesOf);
  ADD_HANDLER_METHOD(R"(/shm/repository/([^\/]+))", Get, exportRepositoryOf);

  // Shared Memory Operations handler methods
  ADD_HANDLER_METHOD(R"(/shm/stats/([^\/]+))", Get, statsOf);
  ADD_HANDLER_METHOD(R"(/shm/cleanup)", Post, cleanupOf);
  ADD_HANDLER_METHOD(R"(/shm/recover)", Post, recoverOf);


  SharedMemoryController(const std::shared_ptr<IAuthService> &authServicie,
    std::shared_ptr<ISharedMemoryService> sharedMemoryService,
    std::shared_ptr<ISharedMemorySegmentManager> sharedMemorySegmentManager,
    std::vector<std::shared_ptr<SharedMemoryRepository>> sharedMemoryRepositories,
    std::shared_ptr<SharedMemoryCleanupService> sharedMemoryCleanupService);
};

}// namespace lintel
#endif// LINTEL_SHAREDMEMORYCONTROLLER_H
