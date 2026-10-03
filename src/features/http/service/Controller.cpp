#include "base_library/features/http/service/Controller.h"

#include <utility>

namespace {

/** Hard cap for the 'limit' query parameter */
constexpr std::size_t kMaxPageLimit = 1000;

}  // namespace

std::string Controller::clientIpOf(const httplib::Request &request) {
    return m_ipResolver.resolveClientIp(
            request.remote_addr, request.get_header_value("X-Forwarded-For"));
}

PagingParams Controller::pagingParamsOf(const httplib::Request &request) {
    PagingParams params;
    const bool hasLimit = request.has_param("limit");
    const bool hasAfter = request.has_param("after");
    if (!hasLimit && !hasAfter) {
        return params;
    }
    params.m_enabled = true;
    if (hasAfter) {
        std::string after = request.get_param_value("after");
        if (!after.empty()) {
            params.m_after = std::move(after);
        }
    }
    if (hasLimit) {
        const std::string &raw = request.get_param_value("limit");
        std::size_t consumed = 0;
        unsigned long long parsed = 0;
        try {
            parsed = std::stoull(raw, &consumed);
        } catch (const std::exception &) {
            throw HttpBadRequestException();
        }
        if (consumed != raw.size()) {
            throw HttpBadRequestException();
        }
        if (parsed == 0 || parsed > kMaxPageLimit) {
            throw HttpBadRequestException();
        }
        params.m_limit = static_cast<std::size_t>(parsed);
    }
    return params;
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
