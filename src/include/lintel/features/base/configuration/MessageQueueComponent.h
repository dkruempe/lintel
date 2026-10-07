#ifndef LINTEL_MESSAGEQUEUECOMPONENT_H
#define LINTEL_MESSAGEQUEUECOMPONENT_H

#include <memory>

#include "lintel/features/base/configuration/Component.h"
#include "lintel/features/base/configuration/Entry.h"
#include "lintel/features/base/configuration/EnvironmentConfiguration.h"

/** Component for parsing message queue configurations from XML */
class MessageQueueComponent : public Component {
private:
    /** XML element name constants for message queue parsing */
    static const struct Shapes {
        const char *const CONFIG_ROOT = "MessageQueues";
        const char *const MESSAGE_QUEUE_ROOT = "MessageQueue";
        const char *const MESSAGE_QUEUE_NAME = "name";
        const char *const MESSAGE_QUEUE_PROCESS_NAME = "process_name";
        const char *const MESSAGE_QUEUE_MAX_MESSAGES = "max_messages";
        const char *const MESSAGE_QUEUE_REMOVE_ON_SHUTDOWN = "remove_on_shutdown";
    } shape;

public:
    /** Construct a MessageQueueComponent */
    MessageQueueComponent();

    /** Parse message queue configuration XML
     * @param content XML content to parse
     * @param fileName Source file name for error reporting
     * @param lineOffset Line offset for error reporting
     * @return Vector of parsed message queue entries */
    std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                              const std::string &fileName,
                                              int32_t lineOffset) override;
};

#endif  // LINTEL_MESSAGEQUEUECOMPONENT_H
