#include "base_library/core/services/LoggerService.h"
#include "base_library/features/base/controller/SharedMemoryApi.h"
#include "base_library/features/base/controller/SharedMemoryRepsoitoriesDto.h"
#include "base_library/features/base/controller/SharedMemorySegmentsDto.h"
#include "base_library/features/http/service/HttpStatusCodes.h"
#include "base_library/features/http/service/HttpUnauthorizedException.h"

SharedMemoryApi::SharedMemoryApi(
    const std::shared_ptr<ClientProvider>& clientProvider)
    : m_client(clientProvider->provide()) {}
std::vector<SharedMemorySegmentDto> SharedMemoryApi::allSegmentsOf(
    const std::string& segmentName) {
  httplib::Headers headers{};
  headers.insert({"Content-Type", "application/json"});
  const httplib::Result& result =
      m_client->get("/shm/segments/" + segmentName, headers);
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
  } catch (std::exception& exception) {
    return {};
  }
  return sharedMemorySegmentsDto.getSegments();
}
void SharedMemoryApi::shrinkOf(const std::string& segmentName) {
  auto result =
      m_client->put("/shm/segments/" + segmentName, "", "application/json");
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
std::vector<SharedMemoryRepositoryDto> SharedMemoryApi::allRepositoriesOf(
    const std::string& repositoryName, const std::string& segmentName) {
  httplib::Headers headers{};
  headers.insert({"Content-Type", "application/json"});
  const httplib::Result& result = m_client->get(
      "/shm/repositories/" + repositoryName + "/" + segmentName, headers);
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
  } catch (std::exception& exception) {
    return {};
  }
  return sharedMemoryRepositoriesDto.getRepositories();
}
