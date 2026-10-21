#ifndef LINTEL_EVENTBUSCLICOMPONENT_H
#define LINTEL_EVENTBUSCLICOMPONENT_H

#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "lintel/features/base/controller/EventBusDtos.h"
#include "lintel/features/cli/models/CommandLineComponent.h"
#include "lintel/features/cli/models/CommandParser.h"
#include "lintel/features/http/controllers/EventBusApi.h"

namespace lintel {
/** CLI component that inspects and drives the configured event buses */
class EventBusCliComponent : public CommandLineComponent
{
private:
  static constexpr std::string_view m_name = "EventBus";
  static constexpr std::string_view m_alias = "Eb";
  static constexpr std::string_view m_description =
    "The component can be used to inspect the event buses and to publish events";

  std::shared_ptr<EventBusApi> m_eventBusApi;

  enum Commands : std::uint8_t {
    Undefined,
    ShowEventBuses,
    ShowEventBus,
    PublishEvent,
    SubscribeEvent,
    UnsubscribeEvent,
    EventQueueDepth,
    ResetEventBus
  };

  // Flags
  std::optional<std::string> m_bus;
  std::optional<std::string> m_topic;
  std::optional<std::string> m_type;
  std::optional<std::string> m_payload;
  std::optional<std::string> m_priority;
  std::optional<std::string> m_subscriber;
  bool m_toSubscribers = false;

  CommandParser<Commands, Undefined> m_commandParser;

  /** Print a table of event bus DTOs to the console
   * @param eventBusDtos the buses to print */
  static void printEventBuses(const std::vector<EventBusDto> &eventBusDtos);

  /** @return the value of the required flag or throw when it is missing
   * @param value the flag value
   * @param flag the flag name used in the error message */
  static const std::string &requiredOf(const std::optional<std::string> &value, const char *flag);

public:
  /** @param eventBusApi API for the event bus endpoints */
  explicit EventBusCliComponent(std::shared_ptr<EventBusApi> eventBusApi);

  void onHelp() override;

  void
    onCommand(const UserDto &userDto, const std::string &command, const std::vector<std::string> &parameters) override;

  void onShowMenu() override;

  bool onMenu(const std::string &component) override;

  bool onExit() override;

  void printCommandList(std::set<std::string> menuAlias) override;

  std::vector<std::string> allCommandsOf() override;
};

}// namespace lintel
#endif// LINTEL_EVENTBUSCLICOMPONENT_H
