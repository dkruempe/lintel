#include "lintel/features/cli/components/MessageQueueCliComponent.h"

#include "lintel/core/utils/TableBuilder.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace lintel {
namespace {
  /** @return the value as string, or "-" when it is not present */
  template<typename T> std::string valueOrDash(const std::optional<T> &value)
  {
    return value.has_value() ? std::to_string(value.value()) : "-";
  }

  std::string valueOrDash(const std::optional<std::string> &value) { return value.has_value() ? value.value() : "-"; }
}// namespace

MessageQueueCliComponent::MessageQueueCliComponent(std::shared_ptr<MesssageQueueApi> messsageQueueApi)
  : CommandLineComponent(m_name, m_alias), m_messageQueueApi(std::move(messsageQueueApi))
{
  m_commandParser.addCommand(
    Command("show_message_queues", "Show all message queues")
      .addArgument({ "--process-name", "-p" }, &m_processName, "Process Name of the message queue owner")
      .addArgument({ "--message-queue-name", "-mq" }, &m_messageQueueName, "Name of message queue itself"),
    ShowMessageQueues);
  m_commandParser.addCommand(
    Command("purge_message_queue", "Remove all messages currently queued in one message queue")
      .addArgument({ "--message-queue-name", "-mq" }, &m_purgeMessageQueueName, "Name of the message queue to purge"),
    PurgeMessageQueue);
}


void MessageQueueCliComponent::onCommand(const UserDto &userDto,
  const std::string &command,
  const std::vector<std::string> &parameters)
{
  try {
    Commands cmd = m_commandParser.parse(command, parameters);
    if (cmd == ShowMessageQueues) {
      const std::string processNameTemp = m_processName.has_value() ? m_processName.value() : ".*";
      const std::string messageQueueNameTemp = m_messageQueueName.has_value() ? m_messageQueueName.value() : ".*";
      auto messageQueues = m_messageQueueApi->allOf(MessageQueueFilter{ processNameTemp, messageQueueNameTemp });
      printMessageQueues(messageQueues);
    } else if (cmd == PurgeMessageQueue) {
      if (!m_purgeMessageQueueName.has_value()) {
        throw std::runtime_error("purge_message_queue requires --message-queue-name");
      }
      const std::string &messageQueueName = m_purgeMessageQueueName.value();
      const std::optional<std::int32_t> purged = m_messageQueueApi->purgeOf(messageQueueName);
      if (!purged.has_value()) {
        std::cout << "ERROR: message queue " << messageQueueName << " could not be purged\n";
        return;
      }
      std::cout << "Purged " << purged.value() << " messages from message queue " << messageQueueName << "\n";
    }
  } catch (const std::exception &exception) {
    std::cerr << "ERROR: " << exception.what() << "\n";
  }
}

void MessageQueueCliComponent::printMessageQueues(const std::vector<MessageQueueDto> &messageQueueDtos)
{
  TableBuilder<12> builder;
  builder.add({ "No.",
    "ProcessName",
    "MessageQueueName",
    "MaxMessages",
    "Messages",
    "Lag",
    "OwnerPid",
    "OwnerHost",
    "OwnerState",
    "HeartbeatAge[ms]",
    "Alive",
    "DeadLetters" });
  std::size_t iter = 0;
  for (const auto &messageQueue : messageQueueDtos) {
    builder.add({ std::to_string(++iter),
      messageQueue.getProcess(),
      messageQueue.getName(),
      std::to_string(messageQueue.getMaxMessages()),
      std::to_string(messageQueue.getMessages()),
      std::to_string(messageQueue.getConsumerLag()),
      valueOrDash(messageQueue.getOwnerPid()),
      valueOrDash(messageQueue.getOwnerHost()),
      valueOrDash(messageQueue.getOwnerState()),
      valueOrDash(messageQueue.getHeartbeatAgeMs()),
      messageQueue.isAlive() ? "alive" : "stale",
      valueOrDash(messageQueue.getDeadLetterQueueSize()) });
  }
  std::cout << builder.build() << "\n";
}


void MessageQueueCliComponent::onHelp() { m_commandParser.printHelp(getName(), getAlias(), m_description); }

bool MessageQueueCliComponent::onMenu(const std::string &component) { return false; }

void MessageQueueCliComponent::onShowMenu() { std::cout << "No submenu available!\n"; }

bool MessageQueueCliComponent::onExit() { return true; }

void MessageQueueCliComponent::printCommandList(std::set<std::string> menuAlias)
{
  m_commandParser.printCommandList(menuAlias);
}

std::vector<std::string> MessageQueueCliComponent::allCommandsOf() { return m_commandParser.allCommandsOf(); }
}// namespace lintel
