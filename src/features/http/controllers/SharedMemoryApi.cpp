#include <httplib.h>

#include <cstdint>
#include <optional>

#include <rapidjson/document.h>

#include "lintel/core/services/LoggerService.h"
#include "lintel/core/utils/JsonUtils.h"
#include "lintel/features/base/controller/SharedMemoryRepositoriesDto.h"
#include "lintel/features/base/controller/SharedMemorySegmentsDto.h"
#include "lintel/features/base/models/SharedMemorySegmentStats.h"
#include "lintel/features/http/controllers/SharedMemoryApi.h"
#include "lintel/features/http/service/HttpClientHelper.h"
#include "lintel/features/http/service/HttpStatusCodes.h"
#include "lintel/features/http/service/HttpUnauthorizedException.h"

namespace lintel {
SharedMemoryApi::SharedMemoryApi(const std::shared_ptr<ClientProvider> &clientProvider)
  : m_client(clientProvider->provide())
{}

std::vector<SharedMemorySegmentDto> SharedMemoryApi::allSegmentsOf(const std::string &segmentName)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const httplib::Result &result = m_client->get("/shm/segments/" + segmentName, headers);
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
  SharedMemorySegmentsDto sharedMemorySegmentsDto;
  try {
    sharedMemorySegmentsDto.deserialize(result->body);
  } catch (const std::exception &exception) {
    return {};
  }
  return sharedMemorySegmentsDto.getSegments();
}

void SharedMemoryApi::shrinkOf(const std::string &segmentName)
{
  auto result = m_client->put("/shm/segments/shrink/" + segmentName, "", "application/json");
  if (!HttpClientHelper::hasResponse(result)) { return; }
  HttpStatusCodes status(result->status);
  switch (status) {
  case HttpStatusCodes::Unauthorized:
    throw HttpUnauthorizedException();
  case HttpStatusCodes::OK:
    break;
  default:
    LOG_ERROR("error {}", status.getCode());
    return;
  }
  LOG_TRACE("successfully shrinkg segment {}", segmentName);
}

std::vector<SharedMemoryRepositoryDto> SharedMemoryApi::allRepositoriesOf(const std::string &repositoryName,
  const std::string &segmentName)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const httplib::Result &result = m_client->get("/shm/repositories/" + repositoryName + "/" + segmentName, headers);
  if (!HttpClientHelper::hasResponse(result)) { return {}; }
  HttpStatusCodes status(result->status);
  switch (status) {
  case HttpStatusCodes::Unauthorized:
    throw HttpUnauthorizedException();
  case HttpStatusCodes::OK:
    break;
  default:
    LOG_ERROR("error {}", status.getCode());
    return {};
  }
  LOG_TRACE("{}", result->body);
  SharedMemoryRepositoriesDto sharedMemoryRepositoriesDto;
  try {
    sharedMemoryRepositoriesDto.deserialize(result->body);
  } catch (const std::exception &exception) {
    return {};
  }
  return sharedMemoryRepositoriesDto.getRepositories();
}

void SharedMemoryApi::growOf(const std::string &segmentName, const std::string &sizeStr)
{
  auto result = m_client->put("/shm/segments/grow/" + segmentName + "/" + sizeStr, "", "application/json");
  if (!HttpClientHelper::hasResponse(result)) { return; }
  HttpStatusCodes status(result->status);
  switch (status) {
  case HttpStatusCodes::Unauthorized:
    throw HttpUnauthorizedException();
  case HttpStatusCodes::OK:
    break;
  default:
    LOG_ERROR("error {}", status.getCode());
    return;
  }
  LOG_TRACE("successfully shrinking segment {}", segmentName);
}

std::optional<SharedMemorySegmentStats> SharedMemoryApi::statsOf(const std::string &segmentName)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const httplib::Result &result = m_client->get("/shm/stats/" + segmentName, headers);
  if (!HttpClientHelper::hasResponse(result)) { return std::nullopt; }
  HttpStatusCodes status(result->status);
  switch (status) {
  case HttpStatusCodes::Unauthorized:
    throw HttpUnauthorizedException();
  case HttpStatusCodes::OK:
    break;
  default:
    LOG_ERROR("error {}", status.getCode());
    return std::nullopt;
  }
  rapidjson::Document document;
  document.Parse(result->body.c_str());
  if (document.HasParseError() || !document.IsObject()) { return std::nullopt; }
  SharedMemorySegmentStats stats;
  if (!json::getString(document, "NAME", stats.name)) { return std::nullopt; }
  const auto readCount = [&document](const char *key, std::size_t &out) {
    std::uint64_t value = 0;
    if (!json::getUint64(document, key, value)) { return false; }
    out = static_cast<std::size_t>(value);
    return true;
  };
  if (!readCount("ALLOCATED_BYTES", stats.allocatedBytes) || !readCount("CAPACITY_BYTES", stats.capacityBytes)
      || !readCount("FREE_BYTES", stats.freeBytes) || !readCount("GROW_COUNT", stats.growCount)
      || !readCount("SHRINK_COUNT", stats.shrinkCount) || !readCount("RECOVERY_COUNT", stats.recoveryCount)
      || !readCount("GENERATION", stats.generation)) {
    return std::nullopt;
  }
  return stats;
}

std::optional<SharedMemoryCleanupSummary> SharedMemoryApi::cleanupOf()
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const httplib::Result &result = m_client->post("/shm/cleanup", headers, "", "application/json");
  if (!HttpClientHelper::hasResponse(result)) { return std::nullopt; }
  HttpStatusCodes status(result->status);
  switch (status) {
  case HttpStatusCodes::Unauthorized:
    throw HttpUnauthorizedException();
  case HttpStatusCodes::OK:
    break;
  default:
    LOG_ERROR("error {}", status.getCode());
    return std::nullopt;
  }
  rapidjson::Document document;
  document.Parse(result->body.c_str());
  if (document.HasParseError() || !document.IsObject()) { return std::nullopt; }
  SharedMemoryCleanupSummary summary;
  if (!json::getBool(document, "DRY_RUN", summary.dryRun)) { return std::nullopt; }
  const auto readCount = [&document](const char *key, std::size_t &out) {
    std::uint64_t value = 0;
    if (!json::getUint64(document, key, value)) { return false; }
    out = static_cast<std::size_t>(value);
    return true;
  };
  if (!readCount("SCANNED", summary.scanned) || !readCount("ORPHANS", summary.orphans)
      || !readCount("REMOVED", summary.removed) || !readCount("FAILED", summary.failed)) {
    return std::nullopt;
  }
  return summary;
}

std::string SharedMemoryApi::repositoryOf(const std::string &uuid)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const httplib::Result &result = m_client->get("/shm/repository/" + uuid, headers);
  if (!HttpClientHelper::hasResponse(result)) { return ""; }
  HttpStatusCodes status(result->status);
  switch (status) {
  case HttpStatusCodes::Unauthorized:
    throw HttpUnauthorizedException();
  case HttpStatusCodes::OK:
    break;
  default:
    LOG_ERROR("error {}", status.getCode());
    return "";
  }
  LOG_TRACE("successfully export repository {} -> {}", uuid, result->body);
  return result->body;
}
}// namespace lintel
