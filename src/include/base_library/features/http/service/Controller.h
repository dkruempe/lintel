#ifndef HTTP_LIBRARY_CONTROLLER_H
#define HTTP_LIBRARY_CONTROLLER_H

#include <httplib.h>

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <exception>
#include <optional>
#include <type_traits>
#include <vector>

#include "base_library/core/services/LoggerService.h"
#include "base_library/core/utils/StringUtils.h"
#include "base_library/features/base/provider/GroupProvider.h"
#include "base_library/features/base/services/IAuthService.h"
#include "base_library/features/http/service/ClientIpResolver.h"
#include "base_library/features/http/service/ContentType.h"
#include "base_library/features/http/service/HttpBadRequestException.h"
#include "base_library/features/http/service/HttpStatusCodes.h"


#define ADD_HANDLER_METHOD(pattern, httpType, name)                           \
  void name##httpType(                                                        \
      const httplib::Request &request, httplib::Response &response,           \
      const ContentType &contentType, const std::optional<UserToken> &user);  \
  Handler name##httpType##Function =                                          \
      addMethod<std::function<HandlerArgs>, httpType>(                        \
          pattern,                                                            \
          [&](const httplib::Request &request, httplib::Response &response) { \
            std::string auth = request.get_header_value("Authorization");     \
            std::string contentTypeString =                                   \
                request.get_header_value("Content-Type");                     \
            if (contentTypeString.empty() && httpType == Get) {               \
              contentTypeString = "application/json";                         \
            }                                                                 \
            ContentType contentType(contentTypeString);                       \
            std::optional<UserToken> user = std::nullopt;                     \
            try {                                                             \
              if (StringUtils::startsWith(auth, "Bearer ") &&                   \
                  auth.size() > 7) {                                            \
                std::string id = auth.substr(7);                                \
                UserTokenLogin userTokenLogin{clientIpOf(request), id};         \
                user = m_authService->onAccessOf(userTokenLogin);               \
              }                                                                 \
              name##httpType(request, response, contentType, user);           \
            } catch (const HttpBadRequestException &e) {                      \
              LOG_ERROR("{}: {}", #name, e.what());                           \
              response.status = HttpStatusCodes::BadRequest;                  \
              response.set_content("bad request", "text/plain");              \
            } catch (const std::exception &e) {                               \
              LOG_ERROR("{}: {}", #name, e.what());                           \
              response.status = HttpStatusCodes::InternalServerError;         \
              response.set_content("internal server error", "text/plain");    \
            }                                                                 \
          })

#define ADD_HANDLER_CONTENT_READER_METHOD(pattern, httpType, name)          \
  HandlerWithContentReader name##httpType##Function =                       \
      addMethod<std::function<HandlerWithContentReaderArgs>, httpType>(     \
          pattern,                                                          \
          [&](const httplib::Request &request, httplib::Response &response, \
              const httplib::ContentReader &contentReader) {                \
            const std::string contentTypeString =                           \
                request.get_header_value("Content-Type");                   \
            ContentType contentType(contentTypeString);                     \
            try {                                                             \
              name##httpType(request, response, contentReader, contentType);  \
            } catch (const std::exception &e) {                               \
              LOG_ERROR("{}: {}", #name, e.what());                           \
              response.status = HttpStatusCodes::InternalServerError;         \
              response.set_content("internal server error", "text/plain");    \
            }                                                                 \
          });                                                               \
  void name##httpType(const httplib::Request &request,                      \
                      httplib::Response &response,                          \
                      const httplib::ContentReader &contentReader,          \
                      const ContentType &contentType)

/** Parsed paging query parameters of a request */
struct PagingParams {
    /** Default maximum number of items per page */
    static constexpr std::size_t kDefaultLimit = 100;
    /** True if paging was requested via 'limit' and/or 'after' */
    bool m_enabled = false;
    /** Exclusive lower bound for the sort key (continuation token) */
    std::optional<std::string> m_after;
    /** Maximum number of items per page */
    std::size_t m_limit = kDefaultLimit;
};

/** Base class for HTTP controllers with automatic method registration and auth */
class Controller : public GroupProvider {
protected:
    std::shared_ptr<IAuthService> m_authService;

    /**
     * Determine the client IP address of a request, honoring the
     * X-Forwarded-For header (leftmost entry) only when the direct peer
     * is a configured trusted proxy.
     * @param request the HTTP request
     * @return the client IP address
     */
    std::string clientIpOf(const httplib::Request &request);

    /**
     * Parses keyset-paging query parameters ('after', 'limit').
     * @param request the HTTP request
     * @return the parsed parameters; enabled=false when neither parameter
     *         is present
     * @throws HttpBadRequestException on an invalid 'limit' value
     */
    static PagingParams pagingParamsOf(const httplib::Request &request);

    ClientIpResolver m_ipResolver;

    using HandlerArgs = void(const httplib::Request &, httplib::Response &);
    using HandlerWithContentReaderArgs = void(const httplib::Request &,
                                              httplib::Response &,
                                              const httplib::ContentReader &);
    using Handler = std::function<HandlerArgs>;
    using HandlerWithContentReader = std::function<HandlerWithContentReaderArgs>;

    enum HttpType {
        Get,
        Put,
        Post,
        Delete
    };

    struct Method {
        const std::string m_pattern;
        std::shared_ptr<Handler> m_handler;
        std::shared_ptr<HandlerWithContentReader> m_handlerWithContentReader;
    };

    std::map<HttpType, std::vector<Method> > m_methods;

    /** Register a handler for a given HTTP method and URL pattern */
    template<class Type, HttpType httpType>
    Type addMethod(const std::string &pattern, Type handler) {
        const auto found = m_methods.find(httpType);
        static_assert(
            httpType != Get || !std::is_same_v<Type, HandlerWithContentReader>,
            "Get and HandlerWithContentReader is not supported");
        static_assert(std::is_same_v<Type, Handler> ||
                      std::is_same_v<Type, HandlerWithContentReader>,
                      "Invalid method type! Pleas use Handler or "
                      "HandlerWithContentReader");
        if constexpr (std::is_same_v<Type, Handler>) {
            const Method method{pattern, std::make_shared<Type>(handler), nullptr};
            if (found != m_methods.end()) {
                found->second.push_back(method);
            } else {
                m_methods.insert({httpType, std::vector<Method>{method}});
            }
        } else if constexpr (std::is_same_v<Type, HandlerWithContentReader>) {
            const Method method{pattern, nullptr, std::make_shared<Type>(handler)};
            if (found != m_methods.end()) {
                found->second.push_back(method);
            } else {
                m_methods.insert({httpType, std::vector<Method>{method}});
            }
        }
        return handler;
    }

public:
    /** @param authService authentication service for access control */
    explicit Controller(std::shared_ptr<IAuthService> authService)
        : GroupProvider(), m_authService(std::move(authService)) {
    }

    ~Controller() override = default;

    /** Configure the trusted proxies allowed to forward X-Forwarded-For.
     *  @param trustedProxies IPs or CIDR ranges (e.g. "10.0.0.0/8") */
    void setTrustedProxies(std::vector<std::string> trustedProxies);

    /** Register all collected handler methods with the HTTP server */
    void registerMethods(std::shared_ptr<httplib::Server> &server);
};

#endif  // HTTP_LIBRARY_CONTROLLER_H
