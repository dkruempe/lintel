#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYCONTROLLER_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYCONTROLLER_H

#include "base_library/core/services/ISharedMemoryService.h"
#include "base_library/features/base/repositories/SharedMemoryRepository.h"
#include "base_library/features/base/services/IAuthService.h"
#include "base_library/features/base/services/ISharedMemorySegmentManager.h"
#include "base_library/features/http/service/Controller.h"
#include "base_library/features/http/service/ContentType.h"

/** HTTP controller for shared memory segment and repository management */
class SharedMemoryController : public Controller {
private:
    std::shared_ptr<ISharedMemoryService> m_sharedMemoryService;
    std::shared_ptr<ISharedMemorySegmentManager> m_sharedMemorySegmentManager;
    std::vector<std::shared_ptr<SharedMemoryRepository>>
            m_sharedMemoryRepositories;
    std::map<std::string, std::shared_ptr<SharedMemoryRepository>> m_uuidSharedMemoryRepositories;
    Group m_adminGroup;
    Group m_userGroup;

    /** Build UUID-to-repository map from repository vector */
    static std::map<std::string, std::shared_ptr<SharedMemoryRepository>>
    build(const std::vector<std::shared_ptr<SharedMemoryRepository>> &repositories);

public:
    // Shared Memory Segment handler methods
    ADD_HANDLER_METHOD(R"(/shm/segments/([^\/]+))", Get, allSegmentsOf);
    ADD_HANDLER_METHOD(R"(/shm/segments/shrink/([^\/]+))", Put, shrinkSegmentOf);
    ADD_HANDLER_METHOD(R"(/shm/segments/grow/([^\/]+)/([^\/]+))", Put, growSegmentOf);

    // Shared Memory Repository handler methods
    ADD_HANDLER_METHOD(R"(/shm/repositories/([^\/]+)/([^\/]+))", Get,
                       allRepositoriesOf);
    ADD_HANDLER_METHOD(R"(/shm/repository/([^\/]+))", Get, exportRepositoryOf);


    SharedMemoryController(
            const std::shared_ptr<IAuthService> &authServicie,
            std::shared_ptr<ISharedMemoryService> sharedMemoryService,
            std::shared_ptr<ISharedMemorySegmentManager> sharedMemorySegmentManager,
            std::vector<std::shared_ptr<SharedMemoryRepository>> sharedMemoryRepositories);
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYCONTROLLER_H
