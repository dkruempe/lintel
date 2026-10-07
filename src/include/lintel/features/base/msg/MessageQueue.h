#ifndef LINTEL_MESSAGEQUEUE_H
#define LINTEL_MESSAGEQUEUE_H

#include "lintel/features/base/models/ProcessName.h"
#include "lintel/features/base/msg/MessageQueueCore.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include <utility>

/**
 * MessageQueue wrapper class for boost::interprocess::message
 * - wraps boost::interprocess::message_queue class
 * - send/trySend sends message in blocking (full) or non blocking way
 * - receive/tryReceive receives message in blocking or non blocking way
 * - removeOf removes total message and doesn't reset message queue
 * @tparam T object for receive/send
 */
template<typename T>
class MessageQueue {
private:
    // variables
    std::string m_processName;
    std::string m_name;
    static constexpr std::size_t m_msgSize = sizeof(T);
    int32_t m_msgCount;
    MessageQueueCore m_messageQueue;
    std::shared_ptr<ProcessName> m_realProcessName;
    std::atomic<std::uint64_t> m_droppedMessages{0};

public:
    /**
     * remove of message queue
     */
    static void removeOf(const std::string &messageQueueName) {
        MessageQueueCore::removeOf(messageQueueName);
    }

    /**
     * @return true if the current process owns this message queue
     */
    [[nodiscard]] bool isOwner() const {
        return m_processName == m_realProcessName->getProcessName();
    }

    /** @return process name associated with this queue */
    [[nodiscard]] const std::string &getProcessName() const {
        return m_processName;
    }

    /** @return queue name */
    [[nodiscard]] const std::string &getName() const {
        return m_name;
    }

    /** @return maximum number of messages the queue can hold */
    [[nodiscard]] int32_t getMsgCount() const {
        return m_msgCount;
    }

    /** @return current number of messages in the queue */
    [[nodiscard]] int32_t numberMessagesOf() const {
        return m_messageQueue.numberMessagesOf();
    }

    /**
     * @param processName name of the owning process
     * @param name of message queue. No filepath just name.
     * @param msgCount limit of queue itself
     * @param realProcessName shared handle used to resolve the process name
     */
    MessageQueue(std::string processName, std::string name, int32_t msgCount,
                 std::shared_ptr<ProcessName> realProcessName)
            : m_processName(std::move(processName)),
              m_name(std::move(name)),
              m_msgCount(msgCount),
              m_messageQueue(m_name, msgCount, m_msgSize),
              m_realProcessName(std::move(realProcessName)) {}

    /**
     * send of message. Message Queue full => blocks until message queue is free
     * @param message
     */
    void sendOf(T message) { m_messageQueue.sendOf(&message, m_msgSize); }

    /**
     * send non blocking. Message Queue full => return false and doesn't send
     * message.
     * @param message
     */
    bool trySendOf(T message) {
        return m_messageQueue.trySendOf(&message, m_msgSize);
    }

    /**
     * send with a bounded wait. If the message queue stays full for the whole
     * timeout the message is dropped and the drop counter is increased.
     * @param message the message to send
     * @param timeout maximum time to wait for queue space
     * @return true if the message was queued, false if it was dropped
     */
    bool sendOfWithTimeout(T message,
                           const std::chrono::milliseconds &timeout) {
        const bool success = m_messageQueue.sendOfWithTimeout(&message, m_msgSize, timeout);
        if (!success) {
            m_droppedMessages.fetch_add(1u, std::memory_order_relaxed);
        }
        return success;
    }

    /**
     * @return the number of messages dropped because the queue stayed full
     * beyond the configured timeout
     */
    [[nodiscard]] std::uint64_t droppedMessagesOf() const {
        return m_droppedMessages.load(std::memory_order_relaxed);
    }

    /**
     * receives message. No message => waits until message is available
     *
     * @return messages
     */
    T receiveOf() {
        T temp{};
        m_messageQueue.receiveOf(&temp, m_msgSize);
        return temp;
    }

    /**
     * try to receive message. No message => non blocking and return nullopt
     *
     * @return optional of received message
     */
    std::optional<T> tryReceiveOf() {
        T temp{};
        if (!m_messageQueue.tryReceiveOf(&temp, m_msgSize)) {
            return std::nullopt;
        }
        return std::make_optional(temp);
    }

    /** @return true if this queue is less than rhs */
    bool operator<(const MessageQueue &rhs) const {
        if (m_processName < rhs.m_processName) return true;
        if (rhs.m_processName < m_processName) return false;
        return m_name < rhs.m_name;
    }

    /** @return true if this queue is greater than rhs */
    bool operator>(const MessageQueue &rhs) const { return rhs < *this; }

    /** @return true if this queue is less than or equal to rhs */
    bool operator<=(const MessageQueue &rhs) const { return !(rhs < *this); }

    /** @return true if this queue is greater than or equal to rhs */
    bool operator>=(const MessageQueue &rhs) const { return !(*this < rhs); }

    /** @return true if queues are equal */
    bool operator==(const MessageQueue &rhs) const {
        return m_processName == rhs.m_processName && m_name == rhs.m_name;
    }

    /** @return true if queues are not equal */
    bool operator!=(const MessageQueue &rhs) const { return !(*this == rhs); }
};

#endif  // LINTEL_MESSAGEQUEUE_H
