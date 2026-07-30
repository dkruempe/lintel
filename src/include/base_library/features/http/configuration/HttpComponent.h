#ifndef CPP_BASE_LIBRARY_HTTPCOMPONENT_H
#define CPP_BASE_LIBRARY_HTTPCOMPONENT_H

#include <tinyxml2.h>

#include "base_library/features/base/configuration/Component.h"

/** Parses HTTP host configuration (server/client) from XML */
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

    /**
     * Parse XML content into HTTP configuration entries
     * @param content XML content
     * @param fileName source file name
     * @param lineOffset line offset for error reporting
     * @return list of parsed entries
     */
    std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                               const std::string &fileName,
                                               int32_t lineOffset) override;
};

#endif  // CPP_BASE_LIBRARY_HTTPCOMPONENT_H
