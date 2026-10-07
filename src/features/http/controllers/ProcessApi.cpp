#include <httplib.h>

#include "lintel/features/http/controllers/ProcessApi.h"

#include "lintel/core/services/LoggerService.h"
#include "lintel/features/base/controller/ProcessGroupsDto.h"
#include "lintel/features/base/controller/ProcessInfosDto.h"
#include "lintel/features/http/service/HttpClientHelper.h"
#include "lintel/features/http/service/HttpStatusCodes.h"
#include "lintel/features/http/service/HttpUnauthorizedException.h"

ProcessApi::ProcessApi(const std::shared_ptr<ClientProvider> &clientProvicer) : m_client(clientProvicer->provide()) {}

std::vector<ProcessInfoDto> ProcessApi::allOf(const std::string &processName)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const httplib::Result &result = m_client->get("/process/processes/" + processName, headers);
  if (!HttpClientHelper::hasResponse(result)) { return {}; }
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

std::vector<ProcessGroupDto> ProcessApi::allGroupsOf(const std::string &groupName)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const httplib::Result &result = m_client->get("/process/groups/" + groupName, headers);
  if (!HttpClientHelper::hasResponse(result)) { return {}; }
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

void ProcessApi::startOf(const std::shared_ptr<Process> &process)
{
  ProcessInfo processInfo(process, -1, false, -1, "", "");
  ProcessInfoDto processInfoDto(processInfo);
  std::string body = processInfoDto.JsonSerializable::serialize();
  const httplib::Result &result = m_client->post("/process/start", body, "application/json");
  if (!HttpClientHelper::hasResponse(result)) {
    throw std::runtime_error("HTTP request failed while starting process");
  }
  HttpStatusCodes status(result->status);
  switch (status) {
  case HttpStatusCodes::Unauthorized:
    throw HttpUnauthorizedException();
  case HttpStatusCodes::OK:
    break;
  default:
    throw std::runtime_error("failed to start process; server responded with status " + std::to_string(result->status));
  }
}

void ProcessApi::stopOf(const std::string &id)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const httplib::Result &result = m_client->deletes("/process/stop/" + id, headers);
  if (!HttpClientHelper::hasResponse(result)) {
    throw std::runtime_error("HTTP request failed while stopping process");
  }
  HttpStatusCodes status(result->status);
  switch (status) {
  case HttpStatusCodes::Unauthorized:
    throw HttpUnauthorizedException();
  case HttpStatusCodes::OK:
    break;
  default:
    throw std::runtime_error("failed to stop process; server responded with status " + std::to_string(result->status));
  }
}

void ProcessApi::terminateOf(const std::string &id)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const httplib::Result &result = m_client->deletes("/process/terminate/" + id, headers);
  if (!HttpClientHelper::hasResponse(result)) {
    throw std::runtime_error("HTTP request failed while terminating process");
  }
  HttpStatusCodes status(result->status);
  switch (status) {
  case HttpStatusCodes::Unauthorized:
    throw HttpUnauthorizedException();
  case HttpStatusCodes::OK:
    break;
  default:
    throw std::runtime_error(
      "failed to terminate process; server responded with status " + std::to_string(result->status));
  }
}

void ProcessApi::restartOf(const std::string &id)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const httplib::Result &result = m_client->put("/process/restart/" + id, headers, std::string{}, "application/json");
  if (!HttpClientHelper::hasResponse(result)) {
    throw std::runtime_error("HTTP request failed while restarting process");
  }
  HttpStatusCodes status(result->status);
  switch (status) {
  case HttpStatusCodes::Unauthorized:
    throw HttpUnauthorizedException();
  case HttpStatusCodes::OK:
    break;
  default:
    throw std::runtime_error(
      "failed to restart process; server responded with status " + std::to_string(result->status));
  }
}

void ProcessApi::resetOf(const std::string &id)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const httplib::Result &result = m_client->post("/process/reset/" + id, "", "application/json");
  if (!HttpClientHelper::hasResponse(result)) {
    throw std::runtime_error("HTTP request failed while resetting process");
  }
  HttpStatusCodes status(result->status);
  switch (status) {
  case HttpStatusCodes::Unauthorized:
    throw HttpUnauthorizedException();
  case HttpStatusCodes::OK:
    break;
  default:
    throw std::runtime_error("failed to reset process; server responded with status " + std::to_string(result->status));
  }
}

std::optional<ProcessInfoDto> ProcessApi::healthOf(const std::string &id)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const httplib::Result &result = m_client->get("/process/health/" + id, headers);
  if (!HttpClientHelper::hasResponse(result)) { return std::nullopt; }
  HttpStatusCodes status(result->status);
  switch (status) {
  case HttpStatusCodes::Unauthorized:
    throw HttpUnauthorizedException();
  case HttpStatusCodes::OK:
    break;
  default:
    return std::nullopt;
  }
  LOG_TRACE("{}", result->body);
  ProcessInfoDto processInfoDto;
  try {
    processInfoDto.JsonSerializable::deserialize(result->body);
    return processInfoDto;
  } catch (const std::exception &exception) {
    static_cast<void>(exception);
    return std::nullopt;
  }
}
