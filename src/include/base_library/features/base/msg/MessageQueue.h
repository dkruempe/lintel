#ifndef CPP_BASE_LIBRARY_MESSAGEQUEUE_H
#define CPP_BASE_LIBRARY_MESSAGEQUEUE_H

#include <boost/interprocess/ipc/message_queue.hpp>
#include <optional>

/**
 * MessageQueue wrapper class for boost::interprocess::message
 * - wraps boost::interprocess::message_queue class
 * - send/trySend sends message in blocking (full) or non blocking way
 * - receive/tryReceivie receives message in blocking or non blocking way
 * - removeOf removes total message and doesn't reset message queue
 * @tparam T object for receive/send
 */
template<typename T>
class MessageQueue {
private:
    // variables
    std::string m_processName;
    std::string m_name;
    static constexpr int32_t m_msgSize = sizeof(T);
    int32_t m_msgCount;
    boost::interprocess::message_queue m_messageQueue;
    // properties
    bool m_removeMessageQueueAfterShutdown;

public:
    /**
     * remove of message queue
     */
    static void removeOf(const std::string &messageQueueName) {
        boost::interprocess::message_queue::remove(messageQueueName.c_str());
    }

    const std::string &getProcessName() {
        return m_processName;
    }

    const std::string &getName() {
        return m_name;
    }

    int32_t getMsgCount() {
        return m_msgCount;
    }

    bool isRemoveMessageQueueAfterShutdown() {
        return m_removeMessageQueueAfterShutdown;
    }

    /**
     * constructor of message queue
     *
     * @param name of message queue. No filepath just name.
     * @param msgCount limit of queue itself
     */
    MessageQueue(std::string processName, std::string name, int32_t msgCount,
                 bool removeMessageQueueAfterShutdown)
            : m_processName(std::move(processName)),
              m_name(std::move(name)),
              m_msgCount(msgCount),
              m_messageQueue(boost::interprocess::open_or_create, m_name.c_str(),
                             m_msgCount, m_msgSize),
              m_removeMessageQueueAfterShutdown(removeMessageQueueAfterShutdown) {}

    /**
     * send of message. Message Queue full => blocks until message queue is free
     * @param message
     */
    void sendOf(T message) { m_messageQueue.send(&message, m_msgSize, 0); }

    /**
     * send non blocking. Message Queue full => return false and doesn't send
     * message.
     * @param message
     */
    bool trySendOf(T message) {
        return m_messageQueue.try_send(&message, m_msgSize, 0);
    }

    /**
     * receives message. No message => waits until message is available
     *
     * @return messages
     */
    T receiveOf() {
        T temp{};
        boost::interprocess::message_queue::size_type recvSize;
        unsigned int priority;
        m_messageQueue.receive(&temp, m_msgSize, recvSize, priority);
        return temp;
    }

    /**
     * try to receive message. No message => non blocking and return nullopt
     *
     * @return optional of received message
     */
    std::optional<T> tryReceiveOf() {
        T temp;
        boost::interprocess::message_queue::size_type recvSize;
        unsigned int priority;
        bool success =
                m_messageQueue.try_receive(&temp, m_msgSize, recvSize, priority);
        if (!success) {
            return std::nullopt;
        }
        return temp;
    }

    bool operator<(const MessageQueue &rhs) const {
        if (m_processName < rhs.m_processName) return true;
        if (rhs.m_processName < m_processName) return false;
        return m_name < rhs.m_name;
    }

    bool operator>(const MessageQueue &rhs) const { return rhs < *this; }

    bool operator<=(const MessageQueue &rhs) const { return !(rhs < *this); }

    bool operator>=(const MessageQueue &rhs) const { return !(*this < rhs); }

    bool operator==(const MessageQueue &rhs) const {
        return m_processName == rhs.m_processName && m_name == rhs.m_name;
    }

    bool operator!=(const MessageQueue &rhs) const { return !(rhs == *this); }
};

#endif  // CPP_BASE_LIBRARY_MESSAGEQUEUE_H
