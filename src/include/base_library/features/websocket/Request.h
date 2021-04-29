#ifndef CPP_BASE_LIBRARY_REQUEST_H
#define CPP_BASE_LIBRARY_REQUEST_H

#include <optional>
#include <ostream>

#include "base_library/features/websocket/Message.h"

class Request : public Message {
 private:
  std::string m_id;
  std::string m_params;
  std::string m_method;
  std::string m_jsonRPC;

 public:
  std::string serialize() override;
  Request(std::string jsonRPC, std::string method, std::string params,
          std::string id);
  [[nodiscard]] const std::string& getId() const;
  [[nodiscard]] std::optional<std::string> getParams() const;
  [[nodiscard]] const std::string& getMethod() const;
  [[nodiscard]] const std::string& getJsonRpc() const;
  ~Request() override = default;
  friend std::ostream& operator<<(std::ostream& os, const Request& request);
};

#endif  // CPP_BASE_LIBRARY_REQUEST_H
