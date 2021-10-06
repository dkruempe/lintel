#ifndef HTTP_LIBRARY_CONTROLLER_H
#define HTTP_LIBRARY_CONTROLLER_H
#include <httplib.h>

#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <type_traits>

#include "base_library/core/services/LoggerService.h"
#include "base_library/core/utils/StringUtils.h"
#include "base_library/features/base/provider/GroupProvider.h"
#include "base_library/features/base/services/AuthService.h"
#include "base_library/features/http/service/ContentType.h"
#include "base_library/features/http/service/HttpStatusCodes.h"

#define ADD_HANDLER_METHOD(pattern, httpType, name)                           \
  Handler name##httpType##Function =                                          \
      addMethod<std::function<HandlerArgs>, httpType>(                        \
          pattern,                                                            \
          [&](const httplib::Request &request, httplib::Response &response) { \
            std::string auth = request.get_header_value("Authorization");     \
            const std::string contentTypeString =                             \
                request.get_header_value("Content-Type");                     \
            ContentType contentType(contentTypeString);                       \
            bool isBearerToken = StringUtils::startsWith(auth, "Bearer");     \
            std::optional<UserToken> user = std::nullopt;                     \
            if (isBearerToken) {                                              \
              std::string id = auth.substr(7);                                \
              UserTokenLogin userTokenLogin{request.remote_addr, id};         \
              user = m_authService->onAccessOf(userTokenLogin);               \
            }                                                                 \
            name##httpType(request, response, contentType, user);             \
          });                                                                 \
  void name##httpType(                                                        \
      const httplib::Request &request, httplib::Response &response,           \
      const ContentType &contentType, const std::optional<UserToken> &user)
#define ADD_HANDLER_CONTENT_READER_METHOD(pattern, httpType, name)          \
  HandlerWithContentReader name##httpType##Function =                       \
      addMethod<std::function<HandlerWithContentReaderArgs>, httpType>(     \
          pattern,                                                          \
          [&](const httplib::Request &request, httplib::Response &response, \
              const httplib::ContentReader &contentReader) {                \
            const std::string contentTypeString =                           \
                request.get_header_value("Content-Type");                   \
            ContentType contentType(contentTypeString);                     \
            name##httpType(request, response, contentReader, contentType);  \
          });                                                               \
  void name##httpType(const httplib::Request &request,                      \
                      httplib::Response &response,                          \
                      const httplib::ContentReader &contentReader,          \
                      const ContentType &contentType)

class Controller : public GroupProvider {
 protected:
  std::shared_ptr<AuthService> m_authService;
  using HandlerArgs = void(const httplib::Request &, httplib::Response &);
  using HandlerWithContentReaderArgs = void(const httplib::Request &,
                                            httplib::Response &,
                                            const httplib::ContentReader &);
  using Handler = std::function<HandlerArgs>;
  using HandlerWithContentReader = std::function<HandlerWithContentReaderArgs>;
  enum HttpType { Get, Put, Post, Delete };
  struct Method {
    const std::string m_pattern;
    std::shared_ptr<Handler> m_handler;
    std::shared_ptr<HandlerWithContentReader> m_handlerWithContentReader;
  };

  std::map<HttpType, std::vector<Method>> m_methods;
  template <class Type, HttpType httpType>
  Type addMethod(const std::string &pattern, Type handler) {
    auto found = m_methods.find(httpType);
    // I check if all limitations of library are checked
    // a) Get / HandlerWithContentReader not supported => abort
    // compilation
    static_assert(
        httpType != Get || !std::is_same<Type, HandlerWithContentReader>::value,
        "Get and HandlerWithContentReader is not supported");
    // b) type has to be Handler or HandlerWithContentReader
    static_assert(std::is_same<Type, Handler>::value ||
                      std::is_same<Type, HandlerWithContentReader>::value,
                  "Invalid method type! Pleas use Handler or "
                  "HandlerWithContentReader");
    // II create method
    if constexpr (std::is_same<Type, Handler>::value) {
      Method method{pattern, std::make_shared<Type>(handler), nullptr};
      if (found != m_methods.end()) {
        found->second.push_back(method);
      } else {
        m_methods.insert({httpType, std::vector<Method>{method}});
      }
    } else if constexpr (std::is_same<Type, HandlerWithContentReader>::value) {
      Method method{pattern, nullptr, std::make_shared<Type>(handler)};
      if (found != m_methods.end()) {
        found->second.push_back(method);
      } else {
        m_methods.insert({httpType, std::vector<Method>{method}});
      }
    }
    return handler;
  }

 public:
  // default constructor / destructor
  explicit Controller(std::shared_ptr<AuthService> authService)
      : GroupProvider(), m_authService(std::move(authService)) {}
  ~Controller() override = default;

  // register methods for http functions
  void registerMethods(std::shared_ptr<httplib::Server> &server);
};

#endif  // HTTP_LIBRARY_CONTROLLER_H
