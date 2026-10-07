#include <httplib.h>

#include "lintel/core/services/LoggerService.h"
#include "lintel/features/http/controllers/HistoryApi.h"

#include "lintel/features/base/controller/HistoryDtos.h"
#include "lintel/features/http/service/HttpClientHelper.h"
#include "lintel/features/http/service/HttpStatusCodes.h"
#include "lintel/features/http/service/HttpUnauthorizedException.h"

HistoryApi::HistoryApi(const std::shared_ptr<ClientProvider> &clientProvicer)
        : m_client(clientProvicer->provide()) {
}

std::vector<HistoryDto> HistoryApi::allOf(const std::string &processName, const std::string &serviceName,
                                          const std::string &label) {
    httplib::Headers headers{};
    headers.insert({"Content-Type", "application/json"});
    const auto &result = m_client->get("/history/" + processName + "/" + serviceName + "/" + label, headers);
    if (!HttpClientHelper::hasResponse(result)) {
        return {};
    }
    HttpStatusCodes const status(result->status);
    switch (status) {
        case HttpStatusCodes::Unauthorized:
            throw HttpUnauthorizedException();
        case HttpStatusCodes::OK:
            break;
        default:
            return {};
    }
    HistoryDtos historyDtos;
    try {
        historyDtos.deserialize(result->body);
    } catch (std::exception &exception) {
        LOG_ERROR("{}", exception.what());
        return {};
    }
    return historyDtos.getHistories();
}
