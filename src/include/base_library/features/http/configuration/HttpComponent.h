#ifndef CPP_BASE_LIBRARY_HTTPCOMPONENT_H
#define CPP_BASE_LIBRARY_HTTPCOMPONENT_H

#include <tinyxml2.h>

#include "base_library/features/base/configuration/Component.h"

class HttpComponent : public Component {
 private:
  static struct Shapes {
    const std::string CONFIG_ROOT = "HttpHost";
    const std::string SERVER_ROOT = "Server";
    const std::string CLIENT_ROOT = "Client";
    const std::string HOST = "host";
    const std::string PORT = "port";
    const std::string READ_TIMEOUT = "read_timeout";
    const std::string WRITE_TIMEOUT = "write_timeout";
    const std::string IDLE_TIMEOUT = "idle_timeout";
    const std::string CONNECTION_TIMEOUT = "connection_timeout";
    const std::string CERT_PATH = "cert_path";
    const std::string KEY_PATH = "key_path";
  } shape;

 public:
  HttpComponent();

  std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                            const std::string &fileName,
                                            int32_t lineOffset) override;
};
#endif  // CPP_BASE_LIBRARY_HTTPCOMPONENT_H
