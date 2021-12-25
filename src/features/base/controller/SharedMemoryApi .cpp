#include "base_library/core/services/LoggerService.h"
#include "base_library/features/base/controller/SharedMemoryApi.h"
#include "base_library/features/base/controller/SharedMemorySegmentsDto.h"
#include "base_library/features/http/service/HttpStatusCodes.h"
#include "base_library/features/http/service/HttpUnauthorizedException.h"

SharedMemoryApi::SharedMemoryApi(
    const std::shared_ptr<ClientProvider>& clientProvider)
    : m_client(clientProvider->provide()) {}
std::vector<SharedMemorySegmentDto> SharedMemoryApi::allOf() {
  httplib::Headers headers{};
  headers.insert({"Content-Type", "application/json"});
  // TODO replace .* with parameter
  const httplib::Result& result = m_client->get("/shm/segments/.*", headers);
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
