#ifndef CPP_BASE_LIBRARY_NOTIFICATION_H
#define CPP_BASE_LIBRARY_NOTIFICATION_H

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <vector>

#include "base_library/features/websocket/messages/Message.h"

class Notification : public Message {
 private:
  const std::string m_method;
  const std::string m_params;

  static void toJson(Notification &notification,
                     rapidjson::Writer<rapidjson::StringBuffer> &writer);

 public:
  bool operator==(const Notification &rhs) const;
  bool operator!=(const Notification &rhs) const;
  std::string toJson();
  static std::string toJson(std::vector<Notification> &requests);
  static std::shared_ptr<Notification> fromJson(jsonType iter);
  Notification(std::string method, std::string params);
  [[nodiscard]] const std::string &getMethod() const;
  [[nodiscard]] const std::string &getParams() const;
  [[nodiscard]] static std::string getJsonRpc();
  std::string serialize() override;
  void serialize(rapidjson::Writer<rapidjson::StringBuffer> &writer) override;
  create_t createFunctionOf() override;
};

#endif  // CPP_BASE_LIBRARY_NOTIFICATION_H
