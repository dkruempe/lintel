#include <boost/interprocess/ipc/message_queue.hpp>
#include <chrono>
#include <cstring>
#include <exception>
#include <iostream>
#include <optional>
#include <ctime>

class Message {
private:
    char m_to[100];
    char m_from[100];
    char m_content[1000];

public:
    Message(const std::string &to, const std::string &from) {
        std::strncpy(m_to, to.c_str(), std::strlen(m_to));
        std::strncpy(m_from, from.c_str(), std::strlen(m_from));
        std::memset(m_content, 0, sizeof(char) * std::strlen(m_content));
    }

    template<typename T>
    void assign(T content) {
        static_assert(sizeof(T) <= sizeof(m_content),
                      "content is bigger than reserved size for content");
        const char *temp = static_cast<char *>(static_cast<void *>(&content));
        /*
         * We'll cast an object to a char array. We have to copy it manually
         * and not with std::strcpy or std::strncpy bc. those function will stop
         * copying by the first 0 terminator.
         */
        for (int i = 0; i < sizeof(T); i++) {
            m_content[i] = temp[i];
        }
    }

    [[nodiscard]] const char *getTo() const { return m_to; }

    [[nodiscard]] const char *getFrom() const { return m_from; }

    template<typename T>
    T as() {
        static_assert(sizeof(T) <= sizeof(m_content),
                      "content is bigger than reserved size for content");
        return *static_cast<T *>(static_cast<void *>(m_content));
    }
};

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
    std::string m_name;
    static constexpr int32_t m_msgSize = sizeof(T);
    int32_t m_msgCount;
    boost::interprocess::message_queue m_messageQueue;
    // properties
    bool removeMessageQueue = true;

public:
    /**
     * remove of message queue
     */
    static void removeOf(const std::string &messageQueueName) {
        boost::interprocess::message_queue::remove(messageQueueName.c_str());
    }

    /**
     * constructor of message queue
     *
     * @param name of message queue. No filepath just name.
     * @param msgCount limit of queue itself
     */
    MessageQueue(std::string name, int32_t msgCount)
            : m_name(std::move(name)),
              m_msgCount(msgCount),
              m_messageQueue(boost::interprocess::open_or_create, m_name.c_str(),
                             m_msgCount, m_msgSize) {}

    ~MessageQueue() {
        if (removeMessageQueue) {
            removeOf(m_name);
        }
    }

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
        T *temp;
        boost::interprocess::message_queue::size_type recvSize;
        unsigned int priority;
        bool success =
                m_messageQueue.try_receive(temp, m_msgSize, recvSize, priority);
        if (!success) {
            return std::nullopt;
        }
        return *temp;
    }
};

struct TestObject {
    char msg[100];
    char name[30];
    int32_t age;
    double rate;
    std::time_t created_timestamp;

    friend std::ostream &operator<<(std::ostream &os, const TestObject &object) {
        os << "msg: " << object.msg << " name: " << object.name
           << " age: " << object.age << " rate: " << object.rate
           << " created_timestamp: " << std::ctime(&object.created_timestamp);
        return os;
    }
};


int main(int /*argc*/, char ** /*argv[]*/) {
    try {
        Message msg("to", "to");
        TestObject test;
        std::strncpy(test.msg, "Hello World!", sizeof(test.msg));
        test.age = 32;
        std::strncpy(test.name, "Example User", sizeof(test.name));
        test.rate = 3.14159265359;
        test.created_timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        msg.assign(test);
        MessageQueue<Message> messageQueue("msg_test_queue", 10);
        messageQueue.trySendOf(msg);
        std::optional<Message> messageOpt = messageQueue.tryReceiveOf();
        std::cout << "Message >" << messageOpt.has_value() << "<\n";
        auto testObjectRecv = messageOpt->as<TestObject>();
        std::cout << test << "\n";
        std::cout << testObjectRecv << "\n";
    } catch (std::exception &ex) {
        std::cerr << ex.what() << "\n";
    }
    return 0;
}
