#include "lintel/features/cli/components/EventBusCliComponent.h"

#include "lintel/core/utils/TableBuilder.h"

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace lintel {
namespace {
  /** @param policy the drop policy
   * @return the name used on the wire */
  const char *dropPolicyNameOf(DropPolicy policy)
  {
    switch (policy) {
    case DropPolicy::Fail:
      return "Fail";
    case DropPolicy::DropOldest:
      return "DropOldest";
    case DropPolicy::DropNewest:
      return "DropNewest";
    case DropPolicy::Block:
      return "Block";
    }
    return "Fail";
  }
}// namespace

EventBusCliComponent::EventBusCliComponent(std::shared_ptr<EventBusApi> eventBusApi)
  : CommandLineComponent(m_name, m_alias), m_eventBusApi(std::move(eventBusApi))
{
  m_commandParser.addCommand(Command("show_event_buses", "Show all configured event buses"), ShowEventBuses);
  m_commandParser.addCommand(Command("show_event_bus", "Show one configured event bus")
                               .addArgument({ "--bus", "-b" }, &m_bus, "Name of the event bus"),
    ShowEventBus);
  m_commandParser.addCommand(
    Command("publish_event", "Publish an event on an event bus")
      .addArgument({ "--bus", "-b" }, &m_bus, "Name of the event bus")
      .addArgument({ "--topic", "-t" }, &m_topic, "Topic of the event")
      .addArgument({ "--type", "-ty" }, &m_type, "Type of the event")
      .addArgument({ "--payload", "-p" }, &m_payload, "Payload of the event (at most 256 bytes)", true)
      .addArgument({ "--priority", "-pr" }, &m_priority, "Priority: Critical, High, Normal or Low", true)
      .addArgument({ "--to-subscribers", "-ts" },
        &m_toSubscribers,
        "Broadcast to the subscribers instead of queueing on the bus",
        true),
    PublishEvent);
  m_commandParser.addCommand(Command("subscribe_event", "Subscribe a named subscriber to a topic")
                               .addArgument({ "--bus", "-b" }, &m_bus, "Name of the event bus")
                               .addArgument({ "--topic", "-t" }, &m_topic, "Topic or wildcard pattern")
                               .addArgument({ "--subscriber", "-s" }, &m_subscriber, "Name of the subscriber"),
    SubscribeEvent);
  m_commandParser.addCommand(Command("unsubscribe_event", "Remove a subscription from a topic")
                               .addArgument({ "--bus", "-b" }, &m_bus, "Name of the event bus")
                               .addArgument({ "--topic", "-t" }, &m_topic, "Topic of the subscription")
                               .addArgument({ "--subscriber", "-s" }, &m_subscriber, "Name of the subscriber"),
    UnsubscribeEvent);
  m_commandParser.addCommand(Command("event_queue_depth", "Show the queue depth of one subscriber")
                               .addArgument({ "--bus", "-b" }, &m_bus, "Name of the event bus")
                               .addArgument({ "--subscriber", "-s" }, &m_subscriber, "Name of the subscriber"),
    EventQueueDepth);
  m_commandParser.addCommand(Command("reset_event_bus", "Drop every queued event of an event bus")
                               .addArgument({ "--bus", "-b" }, &m_bus, "Name of the event bus"),
    ResetEventBus);
}

const std::string &EventBusCliComponent::requiredOf(const std::optional<std::string> &value, const char *flag)
{
  if (!value.has_value()) { throw std::runtime_error(std::string(flag) + " is required"); }
  return value.value();
}

void EventBusCliComponent::onCommand(const UserDto & /*userDto*/,
  const std::string &command,
  const std::vector<std::string> &parameters)
{
  try {
    const Commands cmd = m_commandParser.parse(command, parameters);
    if (cmd == ShowEventBuses) {
      printEventBuses(m_eventBusApi->allOf());
    } else if (cmd == ShowEventBus) {
      const std::string &bus = requiredOf(m_bus, "--bus");
      const std::optional<EventBusDto> eventBus = m_eventBusApi->of(bus);
      if (!eventBus.has_value()) {
        std::cout << "ERROR: event bus " << bus << " not found\n";
        return;
      }
      printEventBuses({ eventBus.value() });
    } else if (cmd == PublishEvent) {
      const std::string &bus = requiredOf(m_bus, "--bus");
      const std::string &topic = requiredOf(m_topic, "--topic");
      const std::string &type = requiredOf(m_type, "--type");
      const std::string payload = m_payload.has_value() ? m_payload.value() : std::string();
      const std::string priority = m_priority.has_value() ? m_priority.value() : std::string("Normal");
      EventPublishRequest request;
      request.bus = bus;
      request.topic = topic;
      request.type = type;
      request.payload = payload;
      request.priority = priority;
      request.toSubscribers = m_toSubscribers;
      const std::optional<bool> published = m_eventBusApi->publishOf(request);
      if (!published.has_value()) {
        std::cout << "ERROR: event " << topic << "/" << type << " could not be published on event bus " << bus << "\n";
        return;
      }
      if (!published.value()) {
        std::cout << "ERROR: event " << topic << "/" << type << " was dropped, every matching queue is full\n";
        return;
      }
      std::cout << "Published event " << topic << "/" << type << " on event bus " << bus << "\n";
    } else if (cmd == SubscribeEvent) {
      const std::string &bus = requiredOf(m_bus, "--bus");
      const std::string &topic = requiredOf(m_topic, "--topic");
      const std::string &subscriber = requiredOf(m_subscriber, "--subscriber");
      const std::optional<bool> subscribed = m_eventBusApi->subscribeOf(EventSubscription{ bus, topic, subscriber });
      if (!subscribed.has_value() || !subscribed.value()) {
        std::cout << "ERROR: subscriber " << subscriber << " could not subscribe to " << topic << " on event bus "
                  << bus << "\n";
        return;
      }
      std::cout << "Subscribed " << subscriber << " to " << topic << " on event bus " << bus << "\n";
    } else if (cmd == UnsubscribeEvent) {
      const std::string &bus = requiredOf(m_bus, "--bus");
      const std::string &topic = requiredOf(m_topic, "--topic");
      const std::string &subscriber = requiredOf(m_subscriber, "--subscriber");
      const std::optional<bool> unsubscribed =
        m_eventBusApi->unsubscribeOf(EventSubscription{ bus, topic, subscriber });
      if (!unsubscribed.has_value() || !unsubscribed.value()) {
        std::cout << "ERROR: subscriber " << subscriber << " is not subscribed to " << topic << " on event bus " << bus
                  << "\n";
        return;
      }
      std::cout << "Unsubscribed " << subscriber << " from " << topic << " on event bus " << bus << "\n";
    } else if (cmd == EventQueueDepth) {
      const std::string &bus = requiredOf(m_bus, "--bus");
      const std::string &subscriber = requiredOf(m_subscriber, "--subscriber");
      const std::optional<std::size_t> depth = m_eventBusApi->depthOf(bus, subscriber);
      if (!depth.has_value()) {
        std::cout << "ERROR: queue depth of subscriber " << subscriber << " on event bus " << bus
                  << " could not be read\n";
        return;
      }
      std::cout << "Depth of subscriber " << subscriber << " on event bus " << bus << ": " << depth.value() << "\n";
    } else if (cmd == ResetEventBus) {
      const std::string &bus = requiredOf(m_bus, "--bus");
      const std::optional<bool> reset = m_eventBusApi->resetOf(bus);
      if (!reset.has_value() || !reset.value()) {
        std::cout << "ERROR: event bus " << bus << " could not be reset\n";
        return;
      }
      std::cout << "Reset event bus " << bus << "\n";
    }
  } catch (const std::exception &exception) {
    std::cerr << "ERROR: " << exception.what() << "\n";
  }
}

void EventBusCliComponent::printEventBuses(const std::vector<EventBusDto> &eventBusDtos)
{
  const auto configNumberOrDash = [](const std::optional<EventBusConfig> &config, auto getter) {
    return config.has_value() ? std::to_string(getter(config.value())) : std::string("-");
  };
  const auto runtimeNumberOrDash = [](const EventBusDto &eventBusDto, auto getter) {
    return eventBusDto.isAvailable() ? std::to_string(getter(eventBusDto)) : std::string("-");
  };
  TableBuilder<14> builder;
  builder.add({ "No.",
    "Name",
    "Available",
    "BusCapacity",
    "SubscriberCapacity",
    "MaxTopics",
    "MaxSubscribers",
    "PubPolicy",
    "Sticky",
    "Topics",
    "Subscribers",
    "Dropped",
    "Sequence",
    "BusDepth" });
  std::size_t iter = 0;
  for (const auto &eventBusDto : eventBusDtos) {
    const std::optional<EventBusConfig> &config = eventBusDto.getConfig();
    builder.add({ std::to_string(++iter),
      eventBusDto.getName(),
      eventBusDto.isAvailable() ? "yes" : "no",
      configNumberOrDash(config, [](const EventBusConfig &entry) { return entry.busCapacity; }),
      configNumberOrDash(config, [](const EventBusConfig &entry) { return entry.subscriberCapacity; }),
      configNumberOrDash(config, [](const EventBusConfig &entry) { return entry.maxTopics; }),
      configNumberOrDash(config, [](const EventBusConfig &entry) { return entry.maxSubscribers; }),
      config.has_value() ? dropPolicyNameOf(config.value().pubPolicy) : std::string("-"),
      config.has_value() ? (config.value().sticky ? "yes" : "no") : std::string("-"),
      runtimeNumberOrDash(eventBusDto, [](const EventBusDto &entry) { return entry.getTopics(); }),
      runtimeNumberOrDash(eventBusDto, [](const EventBusDto &entry) { return entry.getSubscribers(); }),
      runtimeNumberOrDash(eventBusDto, [](const EventBusDto &entry) { return entry.getDropped(); }),
      runtimeNumberOrDash(eventBusDto, [](const EventBusDto &entry) { return entry.getSequence(); }),
      runtimeNumberOrDash(eventBusDto, [](const EventBusDto &entry) { return entry.getBusDepth(); }) });
  }
  std::cout << builder.build() << "\n";
}

void EventBusCliComponent::onHelp() { m_commandParser.printHelp(getName(), getAlias(), m_description); }

bool EventBusCliComponent::onMenu(const std::string & /*component*/) { return false; }

void EventBusCliComponent::onShowMenu() { std::cout << "No submenu available!\n"; }

bool EventBusCliComponent::onExit() { return true; }

void EventBusCliComponent::printCommandList(std::set<std::string> menuAlias)
{
  m_commandParser.printCommandList(menuAlias);
}

std::vector<std::string> EventBusCliComponent::allCommandsOf() { return m_commandParser.allCommandsOf(); }
}// namespace lintel
