#include "base_library/features/http/service/HttpClientHelper.h"

#include <httplib.h>

#include "base_library/core/services/LoggerService.h"

namespace HttpClientHelper {

bool hasResponse(const httplib::Result &result)
{
  if (result == nullptr) {
    LOG_ERROR("HTTP request failed - no response received");
    return false;
  }
  return true;
}

}// namespace HttpClientHelper