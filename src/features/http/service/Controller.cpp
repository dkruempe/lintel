#include "base_library/features/http/service/Controller.h"

#include <cctype>
#include <algorithm>

#include "base_library/core/utils/StringUtils.h"

std::string Controller::clientIpOf(const httplib::Request &request) {
    const std::string forwarded = request.get_header_value("X-Forwarded-For");
    if (!forwarded.empty()) {
        // take the leftmost address (original client)
        const std::size_t comma = forwarded.find(',');
        std::string ip = forwarded.substr(0, comma);
        ip.erase(ip.begin(),
                 std::find_if(ip.begin(), ip.end(), [](unsigned char c) {
                     return !std::isspace(c);
                 }));
        ip.erase(std::find_if(ip.rbegin(), ip.rend(), [](unsigned char c) {
                     return !std::isspace(c);
                 }).base(),
                 ip.end());
        if (!ip.empty()) {
            return ip;
        }
    }
    return request.remote_addr;
}

void Controller::registerMethods(std::shared_ptr<httplib::Server> &server) {
    for (const auto &[type, methods]: m_methods) {
        for (auto &method: methods) {
            switch (type) {
                case Get:
                    if (method.m_handler != nullptr) {
                        server->Get(method.m_pattern.c_str(), *method.m_handler);
                    }
                    break;
                case Put:
                    if (method.m_handler != nullptr) {
                        server->Put(method.m_pattern.c_str(), *method.m_handler);
                    } else if (method.m_handlerWithContentReader != nullptr) {
                        server->Put(method.m_pattern.c_str(),
                                    *method.m_handlerWithContentReader);
                    }
                    break;
                case Post:
                    if (method.m_handler != nullptr) {
                        server->Post(method.m_pattern.c_str(), *method.m_handler);
                    } else if (method.m_handlerWithContentReader != nullptr) {
                        server->Post(method.m_pattern.c_str(),
                                     *method.m_handlerWithContentReader);
                    }
                    break;
                case Delete:
                    if (method.m_handler != nullptr) {
                        server->Delete(method.m_pattern.c_str(), *method.m_handler);
                    } else if (method.m_handlerWithContentReader != nullptr) {
                        server->Delete(method.m_pattern.c_str(),
                                       *method.m_handlerWithContentReader);
                    }
                    break;
            }
        }
    }
}