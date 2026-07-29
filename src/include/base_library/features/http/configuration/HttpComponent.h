#ifndef CPP_BASE_LIBRARY_HTTPCOMPONENT_H
#define CPP_BASE_LIBRARY_HTTPCOMPONENT_H

#include <tinyxml2.h>

#include "base_library/features/base/configuration/Component.h"

class HttpComponent : public Component {
private:
    static struct Shapes {
        const char* const CONFIG_ROOT = "HttpHost";
        const char* const SERVER_ROOT = "Server";
        const char* const CLIENT_ROOT = "Client";
        const char* const HOST = "host";
        const char* const PORT = "port";
        const char* const READ_TIMEOUT = "read_timeout";
        const char* const WRITE_TIMEOUT = "write_timeout";
        const char* const IDLE_TIMEOUT = "idle_timeout";
        const char* const CONNECTION_TIMEOUT = "connection_timeout";
        const char* const CERT_PATH = "cert_path";
        const char* const KEY_PATH = "key_path";
    } shape;

public:
    HttpComponent();

    std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                              const std::string &fileName,
                                              int32_t lineOffset) override;
};

#endif  // CPP_BASE_LIBRARY_HTTPCOMPONENT_H
