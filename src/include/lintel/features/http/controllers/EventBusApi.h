#ifndef LINTEL_EVENTBUSAPI_H
#define LINTEL_EVENTBUSAPI_H

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lintel/features/base/controller/EventBusDtos.h"
#include "lintel/features/http/provider/ClientProvider.h"

namespace lintel {
/** Description of an event that should be published */
struct EventPublishRequest
{
  /** Name of the bus */
  std::string bus;
  /** Topic of the event (at most 31 bytes) */
  std::string topic;
  /** Type of the event (at most 31 bytes) */
  std::string type;
  /** Payload of the event (at most 256 bytes) */
  std::string payload;
  /** Priority: Critical, High, Normal or Low */
  std::string priority = "Normal";
  /** True to broadcast to the subscribers, false for the point-to-point queue */
  bool toSubscribers = false;
};

/** A subscription of a named subscriber to a topic of one bus */
struct EventSubscription
{
  /** Name of the bus */
  std::string bus;
  /** Topic name or wildcard pattern */
  std::string topic;
  /** Name of the subscriber */
  std::string subscriber;
};

/** API client for the event bus endpoints of the HTTP service */
class EventBusApi
{
private:
  std::shared_ptr<Client> m_client;

public:
  /** @param clientProvider provider of the configured HTTP client */
  explicit EventBusApi(const std::shared_ptr<ClientProvider> &clientProvider);

  /** @return all configured event buses with their configuration and runtime counters */
  std::vector<EventBusDto> allOf();

  /**
   * Read one configured event bus.
   * @param name exact bus name
   * @return the bus, nullopt when it is unknown or the request was rejected
   */
  std::optional<EventBusDto> of(const std::string &name);

  /**
   * Publish an event on a bus.
   * @param request the event to publish
   * @return true if the bus accepted the event, nullopt when the request was
   *         rejected; false means every matching queue was full
   */
  std::optional<bool> publishOf(const EventPublishRequest &request);

  /**
   * Subscribe a named subscriber to a topic.
   * @param subscription the subscription to create
   * @return true if the subscription exists afterwards, nullopt when the
   *         request was rejected
   */
  std::optional<bool> subscribeOf(const EventSubscription &subscription);

  /**
   * Remove a subscription.
   * @param subscription the subscription to remove
   * @return true if a subscription was removed, nullopt when the request was
   *         rejected
   */
  std::optional<bool> unsubscribeOf(const EventSubscription &subscription);

  /**
   * Read the queue depth of one subscriber.
   * @param name exact bus name
   * @param subscriber subscriber name
   * @return the number of queued events, nullopt when the request was rejected
   */
  std::optional<std::size_t> depthOf(const std::string &name, const std::string &subscriber);

  /**
   * Drop every queued event of a bus and of its subscriber queues.
   * @param name exact bus name
   * @return true if the bus was reset, nullopt when the request was rejected
   */
  std::optional<bool> resetOf(const std::string &name);
};

}// namespace lintel
#endif// LINTEL_EVENTBUSAPI_H
