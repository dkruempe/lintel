#ifndef CPP_BASE_LIBRARY_MESSAGE_H
#define CPP_BASE_LIBRARY_MESSAGE_H

#include <base_library/config.h>

#include <string>
#include <chrono>

/**
 * abstract Message class for interprocess communication via MessageQueue
 */
class Message {
private:
    char m_receiver[MSG_QUEUE_NAME_SIZE]{};
    char m_sender[MSG_QUEUE_NAME_SIZE]{};
    char m_content[MSG_QUUEUE_CONTENT_SIZE]{};
    time_t m_sendTime{};

public:
    /**
     * constructor
     * @param receiver message queue name
     * @param sender message queue name
     */
    Message(const std::string &receiver, const std::string &sender);

    /**
     * default constructor
     */
    Message() = default;

    /**
     * @return receiver message queue
     */
    [[nodiscard]] const char *receiverOf() const;

    /**
     * @return sender message queue
     */
    [[nodiscard]] const char *senderOf() const;

    /**
     * @return returns send time
     */
    [[nodiscard]] std::chrono::time_point<std::chrono::system_clock> sendTimeOf() const;

    /**
     * assign an class T to the reserved content space.
     * Warning: class T isn't allowed to have dynamic allocation storage which is
     * currently not checked during compiletime. Also note, that the maximum size
     * is the sizeof m_content array.
     *
     * @tparam T class
     * @param content object which gets castet to  char* and copy to m_content
     */
    template<typename T>
    void assign(T content) {
        static_assert(sizeof(T) <= sizeof(m_content),
                      "content is bigger than reserved size for content");
        char *temp = static_cast<char *>(static_cast<void *>(&content));
        /*
         * We'll cast an object to a char array. We have to copy it manually
         * and not with std::strcpy or std::strncpy bc. those function will stop
         * copying by the first 0 terminator.
         */
        for (int i = 0; i < sizeof(T); i++) {
            m_content[i] = temp[i];
        }
    }

    /**
     * convert m_content again the mentioned class
     *
     * @tparam T class
     * @return converted class
     */
    template<typename T>
    T as() {
        static_assert(sizeof(T) <= sizeof(m_content),
                      "content is bigger than reserved size for content");
        return *static_cast<T *>(static_cast<void *>(m_content));
    }
};

#endif  // CPP_BASE_LIBRARY_MESSAGE_H