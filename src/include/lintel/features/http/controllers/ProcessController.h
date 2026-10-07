#ifndef LINTEL_PROCESSCONTROLLER_H
#define LINTEL_PROCESSCONTROLLER_H

#include "lintel/core/services/ProcessService.h"
#include "lintel/features/base/services/IAuthService.h"
#include "lintel/features/http/service/Controller.h"

/** HTTP controller for process lifecycle management */
class ProcessController : public Controller
{
private:
  std::shared_ptr<ProcessService> m_processService;
  Group m_adminGroup;
  Group m_userGroup;

  ADD_HANDLER_METHOD(R"(/process/processes/([^\/]+))", Get, allProcessOf);

  ADD_HANDLER_METHOD(R"(/process/groups/([^\/]+))", Get, allProcessGroupsOf);

  ADD_HANDLER_METHOD(R"(/process/start)", Post, startProcess);

  ADD_HANDLER_METHOD(R"(/process/stop/([^\/]+))", Delete, stopProcess);

  ADD_HANDLER_METHOD(R"(/process/terminate/([^\/]+))", Delete, terminateProcess);

  ADD_HANDLER_METHOD(R"(/process/restart/([^\/]+))", Put, restartProcess);

  ADD_HANDLER_METHOD(R"(/process/reset/([^\/]+))", Post, resetProcess);

  ADD_HANDLER_METHOD(R"(/process/health/([^\/]+))", Get, healthProcess);

public:
  ProcessController(const std::shared_ptr<IAuthService> &authService, std::shared_ptr<ProcessService> processService);
};

#endif// LINTEL_PROCESSCONTROLLER_H
