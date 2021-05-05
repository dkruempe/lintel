#ifndef CPP_BASE_LIBRARY_ECHORESPONSE_H
#define CPP_BASE_LIBRARY_ECHORESPONSE_H

#include "base_library/features/websocket/messages/Response.h"

class EchoResponse : public Response {
 public:
  explicit EchoResponse(const std::string &id);
};

#endif  // CPP_BASE_LIBRARY_ECHORESPONSE_H
