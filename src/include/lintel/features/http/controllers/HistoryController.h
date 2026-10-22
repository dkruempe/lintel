#ifndef LINTEL_HISTORYCONTROLLER_H
#define LINTEL_HISTORYCONTROLLER_H

#include "lintel/features/base/repositories/GroupRepository.h"
#include "lintel/features/base/repositories/HistoryRepository.h"
#include "lintel/features/base/services/IAuthService.h"
#include "lintel/features/http/service/ContentType.h"
#include "lintel/features/http/service/Controller.h"

namespace lintel {
/** HTTP controller that serves history data for a given process/service/label */
class HistoryController : public Controller
{
private:
  std::shared_ptr<HistoryRepository> m_historyRepository;
  Group m_adminGroup;
  Group m_userGroup;

  ADD_HANDLER_METHOD(R"(/history/([^\/]+)/([^\/]+)/([^\/]+))", Get, historyOf);

public:
  explicit HistoryController(const std::shared_ptr<IAuthService> &authService,
    std::shared_ptr<HistoryRepository> historyRepository);
};

}// namespace lintel
#endif// LINTEL_HISTORYCONTROLLER_H
