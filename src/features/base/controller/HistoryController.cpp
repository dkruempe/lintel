#include "base_library/features/base/controller/HistoryController.h"

#include "base_library/features/base/controller/HistoryDto.h"
#include "base_library/features/base/controller/HistoryDtos.h"
#include "base_library/features/http/service/HttpStatusCodes.h"

void HistoryController::historyOfGet(const httplib::Request &request, httplib::Response &response,
                                     const ContentType &contentType, const std::optional<UserToken> &user) {
    if (!user.has_value()) {
        response.status = HttpStatusCodes::Unauthorized;
        response.set_content("", contentType.getName());
        return;
    }
    if (!user->m_user.has(m_userGroup) && !user->m_user.has(m_adminGroup)) {
        response.status = HttpStatusCodes::Unauthorized;
        response.set_content("", contentType.getName());
        return;
    }
    const std::string processName = request.matches[1];
    const std::string serviceName = request.matches[2];
    const std::string label = request.matches[3];
    switch (contentType) {
        case ContentType::ApplicationJson: {
            const auto history = m_historyRepository->allOf(processName, serviceName, label);
            const HistoryDtos historyDtos(history);
            response.set_content(historyDtos.JsonSerializable::serialize(), contentType.getName());
            break;
        }
        default: {
            response.status = HttpStatusCodes::Forbidden;
            response.set_content("", contentType.getName());
            break;
        }
    }
}

HistoryController::HistoryController(const std::shared_ptr<AuthService> &authService,
                                     std::shared_ptr<GroupRepository> groupRepository,
                                     std::shared_ptr<HistoryRepository> historyRepository)
        : Controller(authService),
          m_historyRepository(std::move(historyRepository)),
          m_adminGroup("Admin-History", {}, true),
          m_userGroup("User-History", {}, true) {
    add(m_adminGroup);
    add(m_userGroup);
}
