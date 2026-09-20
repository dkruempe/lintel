#ifndef CPP_BASE_LIBRARY_HTTPCLIENTHELPER_H
#define CPP_BASE_LIBRARY_HTTPCLIENTHELPER_H

namespace httplib {
class Result;
}// namespace httplib

/** Shared HTTP client helper functions. */
namespace HttpClientHelper {

/** @return true if a valid response was received from the server */
bool hasResponse(const httplib::Result &result);

}// namespace HttpClientHelper

#endif// CPP_BASE_LIBRARY_HTTPCLIENTHELPER_H