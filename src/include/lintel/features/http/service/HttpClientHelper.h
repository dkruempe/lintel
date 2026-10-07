#ifndef LINTEL_HTTPCLIENTHELPER_H
#define LINTEL_HTTPCLIENTHELPER_H

namespace httplib {
class Result;
}// namespace httplib

/** Shared HTTP client helper functions. */
namespace HttpClientHelper {

/** @return true if a valid response was received from the server */
bool hasResponse(const httplib::Result &result);

}// namespace HttpClientHelper

#endif// LINTEL_HTTPCLIENTHELPER_H