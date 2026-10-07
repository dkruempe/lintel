#include "lintel/features/base/msg/MessageQueueCore.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

struct MessageQueueCore::Impl
{
  boost::interprocess::message_queue m_messageQueue;

  Impl(const std::string &name, int32_t msgCount, std::size_t msgSize)
    : m_messageQueue(boost::interprocess::open_or_create,
        name.c_str(),
        static_cast<boost::interprocess::message_queue::size_type>(msgCount),
        static_cast<boost::interprocess::message_queue::size_type>(msgSize))
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
