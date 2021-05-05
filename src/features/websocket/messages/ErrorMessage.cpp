#include "base_library/features/websocket/messages/ErrorMessage.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#define ERROR_CODE "code"
#define ERROR_MESSAGE "message"
ErrorMessage::ErrorMessage(ErrorCode errorCode, const std::string &message,
                           std::string id)
    : Response("", buildJson(errorCode, message), std::move(id)) {}
std::string ErrorMessage::buildJson(ErrorCode code,
                                    const std::string &message) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  writer.StartObject();
  writer.Key(ERROR_CODE);
  writer.Int(code);
  writer.Key(ERROR_MESSAGE);
  writer.String(message.c_str());
  writer.EndObject();
  return buffer.GetString();
}
