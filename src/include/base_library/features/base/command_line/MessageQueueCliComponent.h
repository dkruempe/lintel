#ifndef MESSAGEQUEUECLICOMPONENT_H
#define MESSAGEQUEUECLICOMPONENT_H
#include "base_library/features/base/controller/MessageQueueApi.h"
#include "base_library/features/cli/models/CommandLineComponent.h"
#include "base_library/features/cli/models/CommandParser.h"
#include "base_library/features/http/service/Controller.h"

class MessageQueueCliComponent : public CommandLineComponent
{
private:
  static constexpr std::string_view m_name = "MessageQueue";
  static constexpr std::string_view m_alias = "Mq";
  static constexpr std::string_view m_description = "The component can be used to show the message queue statistics";
  std::shared_ptr<MesssageQueueApi> m_messageQueueApi;

  enum Commands
  {
    Undefined,
    ShowMessageQueues
  };

  // Flags
  std::optional<std::string> m_processName;
  std::optional<std::string> m_messageQueueName;

  CommandParser<Commands, Undefined> m_commandParser;

  void printMessageQueues(const std::vector<MessageQueueDto> &messageQueueDtos);

public:
  explicit MessageQueueCliComponent(std::shared_ptr<MesssageQueueApi> messsageQueueApi);

  void onHelp() override;

  void onCommand(const UserDto &userDto,
    const std::string &command,
    const std::vector<std::string> &parameters) override;

  void onShowMenu() override;

  bool onMenu(const std::string &component) override;

  bool onExit() override;

  void printCommandList(std::set<std::string> menuAlias) override;

  std::vector<std::string> allCommandsOf() override;
};

#endif //MESSAGEQUEUECLICOMPONENT_H