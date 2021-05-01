#ifndef CPP_BASE_LIBRARY_ERRORMESSAGE_H
#define CPP_BASE_LIBRARY_ERRORMESSAGE_H

#include "base_library/features/websocket/messages/Response.h"
#include "base_library/features/websocket/models/ErrorCode.h"

class ErrorMessage : public Response {
 private:
  static std::string buildJson(ErrorCode code, const std::string& message);

 public:
  ErrorMessage(ErrorCode errorCode, const std::string& message, std::string id);
};

#endif  // CPP_BASE_LIBRARY_ERRORMESSAGE_H
