#include "base_library/features/http/controllers/ProcessApi.h"

#include "base_library/core/services/LoggerService.h"
#include "base_library/features/base/controller/ProcessGroupsDto.h"
#include "base_library/features/base/controller/ProcessInfosDto.h"
#include "base_library/features/http/service/HttpClientHelper.h"
#include "base_library/features/http/service/HttpStatusCodes.h"
#include "base_library/features/http/service/HttpUnauthorizedException.h"

ProcessApi::ProcessApi(const std::shared_ptr<ClientProvider> &clientProvicer)
        : m_client(clientProvicer->provide()) {}

std::vector<ProcessInfoDto> ProcessApi::allOf(const std::string &processName) {
    httplib::Headers headers{};
    headers.insert({"Content-Type", "application/json"});
    const httplib::Result &result =
            m_client->get("/process/processes/" + processName, headers);
    if (!HttpClientHelper::hasResponse(result)) {
        return {};
    }
    HttpStatusCodes status(result->status);
    switch (status) {
        case HttpStatusCodes::Unauthorized:
            throw HttpUnauthorizedException();
        case HttpStatusCodes::OK:
            // all fine => no handling needed
            break;
        default:
            // currently no extra handling
            return {};
    }
    LOG_TRACE("{}", result->body);
    ProcessInfosDto processInfosDto;
    try {
        processInfosDto.JsonSerializable::deserialize(result->body);
    } catch (const std::exception &exception) {
        return {};
    }
    return processInfosDto.getProcessInfos();
}

std::vector<ProcessGroupDto> ProcessApi::allGroupsOf(
        const std::string &groupName) {
    httplib::Headers headers{};
    headers.insert({"Content-Type", "application/json"});
    const httplib::Result &result =
            m_client->get("/process/groups/" + groupName, headers);
    if (!HttpClientHelper::hasResponse(result)) {
        return {};
    }
    HttpStatusCodes status(result->status);
    switch (status) {
        case HttpStatusCodes::Unauthorized:
            throw HttpUnauthorizedException();
        case HttpStatusCodes::OK:
            // all fine => no handling needed
            break;
        default:
            // currently no extra handling
            return {};
    }
    LOG_TRACE("{}", result->body);
    ProcessGroupsDto processGroupsDto;
    try {
        processGroupsDto.deserialize(result->body);
    } catch (const std::exception &exception) {
        return {};
    }
    return processGroupsDto.getProcessGroups();
}

void ProcessApi::startOf(const std::shared_ptr<Process> &process) {
    ProcessInfo processInfo(process, -1, false, -1, "", "");
    ProcessInfoDto processInfoDto(processInfo);
    std::string body = processInfoDto.JsonSerializable::serialize();
    m_client->post("/process/start", body, "application/json");
}

void ProcessApi::stopOf(const std::string &id) {
    httplib::Headers headers{};
    headers.insert({"Content-Type", "application/json"});
    m_client->deletes("/process/stop/" + id, headers);
}

void ProcessApi::terminateOf(const std::string &id) {
    httplib::Headers headers{};
    headers.insert({"Content-Type", "application/json"});
    m_client->deletes("/process/terminate/" + id, headers);
}
