#include "base_library/features/http/service/Controller.h"

#include <utility>

std::string Controller::clientIpOf(const httplib::Request &request) {
    return m_ipResolver.resolveClientIp(
            request.remote_addr, request.get_header_value("X-Forwarded-For"));
}

void Controller::setTrustedProxies(std::vector<std::string> trustedProxies) {
    m_ipResolver.setTrustedProxies(std::move(trustedProxies));
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
