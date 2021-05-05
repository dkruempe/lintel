#ifndef CPP_BASE_LIBRARY_REQUEST_H
#define CPP_BASE_LIBRARY_REQUEST_H

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <vector>

#include "base_library/features/websocket/messages/Message.h"

class Request : public Message {
 private:
  const std::string m_method;
  const std::string m_params;
  const std::string m_id;

  static void toJson(Request &request,
                     rapidjson::Writer<rapidjson::StringBuffer> &writer);

 public:
  bool operator==(const Request &rhs) const;
  bool operator!=(const Request &rhs) const;
  std::string toJson();
  static std::string toJson(std::vector<Request> &requests);
  static std::vector<Request> fromJson(const std::string &serialized);
  Request(std::string method, std::string params, std::string id);
  [[nodiscard]] const std::string &getMethod() const;
  [[nodiscard]] const std::string &getParams() const;
  [[nodiscard]] const std::string &getId() const;
  [[nodiscard]] static std::string getJsonRpc();
  std::string serialize() override;
  void serialize(rapidjson::Writer<rapidjson::StringBuffer> &writer) override;
};

#endif  // CPP_BASE_LIBRARY_REQUEST_H
