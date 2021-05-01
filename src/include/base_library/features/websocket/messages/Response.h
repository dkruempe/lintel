#ifndef CPP_BASE_LIBRARY_RESPONSE_H
#define CPP_BASE_LIBRARY_RESPONSE_H

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <vector>

#include "base_library/features/websocket/messages/Message.h"

class Response : public Message {
 private:
  std::string m_jsonRPC;
  std::string result;
  std::string error;
  std::string id;

  static void toJson(const Response &response,
                     rapidjson::Writer<rapidjson::StringBuffer> &writer);

 public:
  bool operator==(const Response &rhs) const;
  bool operator!=(const Response &rhs) const;
  [[nodiscard]] std::string toJson() const;
  static std::string toJson(const std::vector<Response> &responses);
  static std::vector<Response> fromJson(const std::string &serialized);
  Response(std::string jsonRPC, std::string result, std::string error, std::string id);
  [[nodiscard]] const std::string &getResult() const;
  [[nodiscard]] const std::string &getError() const;
  [[nodiscard]] const std::string &getId() const;
  [[nodiscard]] static std::string getJsonRpc();
  std::string serialize() override;
};

#endif  // CPP_BASE_LIBRARY_RESPONSE_H
