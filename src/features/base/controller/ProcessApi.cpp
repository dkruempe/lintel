#include "base_library/features/base/controller/ProcessApi.h"

#include "base_library/core/services/LoggerService.h"
#include "base_library/features/base/controller/ProcessInfosDto.h"
#include "base_library/features/http/service/HttpStatusCodes.h"
#include "base_library/features/http/service/HttpUnauthorizedException.h"

ProcessApi::ProcessApi(const std::shared_ptr<ClientProvider>& clientProvicer)
    : m_client(clientProvicer->provide()) {}
std::vector<ProcessInfoDto> ProcessApi::allOf(const std::string& processName) {
  httplib::Headers headers{};
  headers.insert({"Content-Type", "application/json"});
  const httplib::Result& result =
      m_client->get("/process/" + processName, headers);
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
    processInfosDto.deserialize(result->body);
  } catch (std::exception& exception) {
    return {};
  }
  return processInfosDto.getProcessInfos();
}
