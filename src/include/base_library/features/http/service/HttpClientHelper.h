#ifndef CPP_BASE_LIBRARY_HTTPCLIENTHELPER_H
#define CPP_BASE_LIBRARY_HTTPCLIENTHELPER_H

#include <httplib.h>

#include "base_library/core/services/LoggerService.h"

/** Shared HTTP client helper functions. */
namespace HttpClientHelper {

/** @return true if a valid response was received from the server */
inline bool hasResponse(const httplib::Result &result) {
    if (result == nullptr) {
        LOG_ERROR("HTTP request failed - no response received");
        return false;
    }
    return true;
}

}  // namespace HttpClientHelper

#endif  // CPP_BASE_LIBRARY_HTTPCLIENTHELPER_H
