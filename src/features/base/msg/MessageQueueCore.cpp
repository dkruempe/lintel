#include "lintel/features/base/msg/MessageQueueCore.h"
#include <chrono>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

namespace lintel {
struct MessageQueueCore::Impl
{
  boost::interprocess::message_queue m_messageQueue;
  std::string m_name;
  MessageQueueOverflowPolicy m_policy = MessageQueueOverflowPolicy::BlockWithTimeout;
  std::chrono::milliseconds m_timeout{ 100 };
  std::string m_dlqName;

  Impl(const std::string &name, int32_t msgCount, std::size_t msgSize)
    : m_messageQueue(boost::interprocess::open_or_create,
        name.c_str(),
        static_cast<boost::interprocess::message_queue::size_type>(msgCount),
        static_cast<boost::interprocess::message_queue::size_type>(msgSize)),
      m_name(name)
  {}
};

MessageQueueCore::MessageQueueCore(std::string name, int32_t msgCount, std::size_t msgSize)
  : m_impl(std::make_unique<Impl>(name, msgCount, msgSize))
{}

MessageQueueCore::~MessageQueueCore() = default;

void MessageQueueCore::removeOf(const std::string &messageQueueName)
{
  boost::interprocess::message_queue::remove(messageQueueName.c_str());
}

int32_t MessageQueueCore::numberMessagesOfIfExistsOf(const std::string &messageQueueName)
{
  try {
    boost::interprocess::message_queue queue(boost::interprocess::open_only, messageQueueName.c_str());
    return static_cast<int32_t>(queue.get_num_msg());
  } catch (const boost::interprocess::interprocess_exception &) {
    // queue does not exist yet => no messages
    return 0;
  }
}

int32_t MessageQueueCore::numberMessagesOf() const
{
  return static_cast<int32_t>(m_impl->m_messageQueue.get_num_msg());
}

void MessageQueueCore::sendOf(const void *message, std::size_t size) { m_impl->m_messageQueue.send(message, size, 0); }

bool MessageQueueCore::trySendOf(const void *message, std::size_t size)
{
  return m_impl->m_messageQueue.try_send(message, size, 0);
}

bool MessageQueueCore::sendOfWithTimeout(const void *message,
  std::size_t size,
  const std::chrono::milliseconds &timeout)
{
  return m_impl->m_messageQueue.timed_send(message,
    size,
    0,
    boost::posix_time::microsec_clock::universal_time() + boost::posix_time::milliseconds(timeout.count()));
}

void MessageQueueCore::receiveOf(void *message, std::size_t size)
{
  boost::interprocess::message_queue::size_type recvSize;
  unsigned int priority;
  m_impl->m_messageQueue.receive(message, size, recvSize, priority);
}

bool MessageQueueCore::tryReceiveOf(void *message, std::size_t size)
{
  boost::interprocess::message_queue::size_type recvSize;
  unsigned int priority;
  return m_impl->m_messageQueue.try_receive(message, size, recvSize, priority);
}

bool MessageQueueCore::receiveOfWithTimeout(void *message, std::size_t size, const std::chrono::milliseconds &timeout)
{
  boost::interprocess::message_queue::size_type recvSize;
  unsigned int priority;
  return m_impl->m_messageQueue.timed_receive(message,
    size,
    recvSize,
    priority,
    boost::posix_time::microsec_clock::universal_time() + boost::posix_time::milliseconds(timeout.count()));
}
}// namespace lintel

namespace lintel {
void MessageQueueCore::setOverflowPolicy(MessageQueueOverflowPolicy policy) { m_impl->m_policy = policy; }
void MessageQueueCore::setOverflowTimeout(const std::chrono::milliseconds &timeout) { m_impl->m_timeout = timeout; }
void MessageQueueCore::setDeadLetterQueueName(const std::string &name) { m_impl->m_dlqName = name; }
}// namespace lintel
