#include "base_library/features/http/controllers/HistoryController.h"

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
    if (contentType == ContentType::ApplicationJson) {
        const auto history = m_historyRepository->allOf(processName, serviceName, label);
        const HistoryDtos historyDtos(history);
        response.set_content(historyDtos.JsonSerializable::serialize(), contentType.getName());
    } else {
        response.status = HttpStatusCodes::Forbidden;
        response.set_content("", contentType.getName());
    }
}

// groupRepository wird nicht ausgewertet - die Gruppen werden in der
// Initialisierungsliste hartcodiert (m_adminGroup/m_userGroup). Die Signatur
// bleibt unveraendert, weil der Konstruktor vom DI-Container aufgeloest wird;
// das Entfernen des Parameters waere ein Wiring- und API-Thema fuer sich.
HistoryController::HistoryController(const std::shared_ptr<IAuthService> &authService,
                                     std::shared_ptr<GroupRepository> groupRepository,  // NOLINT(performance-unnecessary-value-param)
                                     std::shared_ptr<HistoryRepository> historyRepository)
        : Controller(authService),
          m_historyRepository(std::move(historyRepository)),
          m_adminGroup("Admin-History", {}, true),
          m_userGroup("User-History", {}, true) {
    add(m_adminGroup);
    add(m_userGroup);
}
