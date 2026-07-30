#ifndef CPP_BASE_LIBRARY_HISTORYCONTROLLER_H
#define CPP_BASE_LIBRARY_HISTORYCONTROLLER_H

#include "base_library/features/base/repositories/GroupRepository.h"
#include "base_library/features/base/services/IAuthService.h"
#include "base_library/features/http/service/Controller.h"
#include "base_library/features/base/repositories/HistoryRepository.h"
#include "base_library/features/http/service/ContentType.h"

class HistoryController : public Controller {
private:
    std::shared_ptr<HistoryRepository> m_historyRepository;
    Group m_adminGroup;
    Group m_userGroup;

    ADD_HANDLER_METHOD(R"(/history/([^\/]+)/([^\/]+)/([^\/]+))", Get,
                       historyOf);

public:
    explicit HistoryController(const std::shared_ptr<IAuthService> &authService,
                               std::shared_ptr<GroupRepository> groupRepository,
                               std::shared_ptr<HistoryRepository> historyRepository);
};

#endif //CPP_BASE_LIBRARY_HISTORYCONTROLLER_H
