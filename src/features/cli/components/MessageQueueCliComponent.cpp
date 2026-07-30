#include "base_library/features/cli/components/MessageQueueCliComponent.h"

#include "base_library/core/utils/TableBuilder.h"

MessageQueueCliComponent::MessageQueueCliComponent(std::shared_ptr<MesssageQueueApi> messsageQueueApi)
  : CommandLineComponent(m_name, m_alias),
    m_messageQueueApi(std::move(messsageQueueApi))
{
  m_commandParser.addCommand(
    Command("show_message_queues", "Show all message queues").addArgument({ "--process-name", "-p" },
      &m_processName,
      "Process Name of the message queue owner")
    .addArgument({ "--message-queue-name", "-mq" }, &m_messageQueueName, "Name of message queue itself"),
    ShowMessageQueues);
}


void MessageQueueCliComponent::onCommand(const UserDto &userDto,
  const std::string &command,
  const std::vector<std::string> &parameters)
{
  try {
    Commands cmd = m_commandParser.parse(command, parameters);
    switch (cmd) {
    case ShowMessageQueues: {
      const std::string processNameTemp = m_processName.has_value() ? m_processName.value() : ".*";
      const std::string messageQueueNameTemp = m_messageQueueName.has_value() ? m_messageQueueName.value() : ".*";
      auto messageQueues = m_messageQueueApi->allOf(processNameTemp, messageQueueNameTemp);
      printMessageQueues(messageQueues);
      break;
    }
    default: { break; }
    }
  } catch (const std::exception &exception) { std::cerr << "ERROR: " << exception.what() << "\n"; }
}

void MessageQueueCliComponent::printMessageQueues(const std::vector<MessageQueueDto> &messageQueueDtos)
{
  TableBuilder<5> builder;
  builder.add({ "No.", "ProcessName", "MessageQueueName", "MaxMessages", "Messages" });
  std::size_t iter = 0;
  for (const auto &messageQueue : messageQueueDtos) {
    builder.add({
      std::to_string(++iter), messageQueue.getProcess(), messageQueue.getName(),
      std::to_string(messageQueue.getMaxMessages()), std::to_string(messageQueue.getMessages()) });
  }
  std::cout << builder.build() << "\n";
}


void MessageQueueCliComponent::onHelp() { m_commandParser.printHelp(getName(), getAlias(), m_description); }

bool MessageQueueCliComponent::onMenu(const std::string &component) { return true; }

void MessageQueueCliComponent::onShowMenu() { std::cout << "No submenu available!\n"; }

bool MessageQueueCliComponent::onExit() { return true; }

void MessageQueueCliComponent::printCommandList(std::set<std::string> menuAlias)
{
  m_commandParser.printCommandList(menuAlias);
}

std::vector<std::string> MessageQueueCliComponent::allCommandsOf() { return m_commandParser.allCommandsOf(); }