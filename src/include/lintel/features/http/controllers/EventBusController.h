#ifndef LINTEL_EVENTBUSCONTROLLER_H
#define LINTEL_EVENTBUSCONTROLLER_H

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lintel/features/base/controller/EventBusDtos.h"
#include "lintel/features/base/services/EventBusService.h"
#include "lintel/features/base/services/IAuthService.h"
#include "lintel/features/http/service/ContentType.h"
#include "lintel/features/http/service/Controller.h"

namespace lintel {
/**
 * HTTP controller that exposes the configured event buses.
 *
 * Reading (list, details, subscriber depth) and publishing are open to the
 * "User-EventBus" and "Admin-EventBus" groups. Subscribing, unsubscribing and
 * resetting mutate the bus registry or drop queued events, so they require
 * the "Admin-EventBus" group.
 */
class EventBusController : public Controller
{
private:
  std::shared_ptr<EventBusService> m_eventBusService;
  Group m_adminGroup;
  Group m_userGroup;

  /** Required permission of a route. */
  enum class Access : std::uint8_t {
    /** read bus information - user or admin */
    Read,
    /** publish an event - user or admin */
    Publish,
    /** mutate subscriptions or drop queued events - admin only */
    Administer
  };

  /** Check the token, the group membership and the content type of a request.
   * @param response receives 401 or 403 when the request is rejected
   * @param contentType negotiated content type
   * @param user authenticated user, if any
   * @param access required permission of the route
   * @return true if the request may proceed */
  [[nodiscard]] bool authorize(httplib::Response &response,
    const ContentType &contentType,
    const std::optional<UserToken> &user,
    Access access) const;

  /** Resolve a configured bus.
   * @param response receives 404 when the name is not configured
   * @param contentType negotiated content type
   * @param name bus name
   * @return the bus view, empty when the response was set to 404 */
  [[nodiscard]] std::optional<std::shared_ptr<IEventBus>>
    busOf(httplib::Response &response, const ContentType &contentType, const std::string &name);

  /** Build the DTO of one configured bus, including the best-effort runtime
   * counters while the bus can be opened.
   * @param name bus name
   * @return the bus DTO */
  [[nodiscard]] EventBusDto describeOf(const std::string &name) const;

public:
  ADD_HANDLER_METHOD(R"(/eventBus/buses)", Get, busesOf);
  ADD_HANDLER_METHOD(R"(/eventBus/bus/([^\/]+))", Get, busOf);
  ADD_HANDLER_METHOD(R"(/eventBus/publish/([^\/]+))", Post, publishOf);
  ADD_HANDLER_METHOD(R"(/eventBus/subscribe/([^\/]+))", Post, subscribeOf);
  ADD_HANDLER_METHOD(R"(/eventBus/subscribe/([^\/]+))", Delete, unsubscribeOf);
  ADD_HANDLER_METHOD(R"(/eventBus/depth/([^\/]+)/([^\/]+))", Get, depthOf);
  ADD_HANDLER_METHOD(R"(/eventBus/reset/([^\/]+))", Post, resetOf);

  /** Construct an EventBusController.
   * @param authService authentication service for access control
   * @param eventBusService service that owns the configured buses */
  EventBusController(const std::shared_ptr<IAuthService> &authService,
    std::shared_ptr<EventBusService> eventBusService);
};

}// namespace lintel
#endif// LINTEL_EVENTBUSCONTROLLER_H
