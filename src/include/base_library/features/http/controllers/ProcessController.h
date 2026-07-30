#ifndef CPP_BASE_LIBRARY_PROCESSCONTROLLER_H
#define CPP_BASE_LIBRARY_PROCESSCONTROLLER_H

#include "base_library/core/services/ProcessService.h"
#include "base_library/features/base/services/IAuthService.h"
#include "base_library/features/http/service/Controller.h"

/** HTTP controller for process lifecycle management */
class ProcessController : public Controller {
private:
    std::shared_ptr<ProcessService> m_processService;
    Group m_adminGroup;
    Group m_userGroup;

    ADD_HANDLER_METHOD(R"(/process/processes/([^\/]+))", Get, allProcessOf);

    ADD_HANDLER_METHOD(R"(/process/groups/([^\/]+))", Get, allProcessGroupsOf);

    ADD_HANDLER_METHOD(R"(/process/start)", Post, startProcess);

    ADD_HANDLER_METHOD(R"(/process/stop/([^\/]+))", Delete, stopProcess);

    ADD_HANDLER_METHOD(R"(/process/terminate/([^\/]+))", Delete, terminateProcess);

public:
    ProcessController(const std::shared_ptr<IAuthService> &authService,
                      std::shared_ptr<ProcessService> processService);
};

#endif  // CPP_BASE_LIBRARY_PROCESSCONTROLLER_H
