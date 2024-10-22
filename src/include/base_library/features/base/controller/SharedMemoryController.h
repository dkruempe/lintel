#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYCONTROLLER_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYCONTROLLER_H

#include "base_library/core/services/SharedMemoryService.h"
#include "base_library/features/base/repositories/SharedMemoryRepository.h"
#include "base_library/features/http/service/Controller.h"
#include "base_library/features/http/service/ContentType.h"

class SharedMemoryController : public Controller {
private:
    std::shared_ptr<SharedMemoryService> m_sharedMemoryService;
    std::shared_ptr<SharedMemorySegmentManager> m_sharedMemorySegmentManager;
    std::vector<std::shared_ptr<SharedMemoryRepository>>
            m_sharedMemoryRepositories;
    std::map<std::string, std::shared_ptr<SharedMemoryRepository>> m_uuidSharedMemoryRepositories;
    Group m_adminGroup;
    Group m_userGroup;

    static std::map<std::string, std::shared_ptr<SharedMemoryRepository>>
    build(std::vector<std::shared_ptr<SharedMemoryRepository>> repositories);

    // Shared Memory Segment functions
    /**
     * shows all segment information with matching name
     */
    ADD_HANDLER_METHOD(R"(/shm/segments/([^\/]+))", Get, allSegmentsOf);
    /**
     * shrinks size of segment of given name
     */
    ADD_HANDLER_METHOD(R"(/shm/segments/shrink/([^\/]+))", Put, shrinkSegmentOf);
    /**
     * extends size of segment of given name
     */
    ADD_HANDLER_METHOD(R"(/shm/segments/grow/([^\/]+)/([^\/]+))", Put, growSegmentOf);

    // Shared Memory Repository functions
    /**
     * Shows all repositories with matching name
     * - show general information about repository
     * - shows linked SharedMemorySegment
     * - give possibility to search for all repositories of given segment
     */
    ADD_HANDLER_METHOD(R"(/shm/repositories/([^\/]+)/([^\/]+))", Get,
                       allRepositoriesOf);
    /**
     * Export Shared Memory Repository in Json
     */
    ADD_HANDLER_METHOD(R"(/shm/repository/([^\/]+))", Get, exportRepositoryOf);

public:
    SharedMemoryController(
            const std::shared_ptr<AuthService> &authServicie,
            std::shared_ptr<SharedMemoryService> sharedMemoryService,
            std::shared_ptr<SharedMemorySegmentManager> sharedMemorySegmentManager,
            std::vector<std::shared_ptr<SharedMemoryRepository>> sharedMemoryRepositories);
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYCONTROLLER_H
