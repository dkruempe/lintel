#ifndef CPP_BASE_LIBRARY_MESSAGEQUEUECOMPONENT_H
#define CPP_BASE_LIBRARY_MESSAGEQUEUECOMPONENT_H

#include <memory>

#include "base_library/features/base/configuration/Component.h"
#include "base_library/features/base/configuration/Entry.h"
#include "base_library/features/base/configuration/EnvironmentConfiguration.h"

class MessageQueueComponent : public Component {
private:
    static const struct Shapes {
        const char *const CONFIG_ROOT = "MessageQueues";
        const char *const MESSAGE_QUEUE_ROOT = "MessageQueue";
        const char *const MESSAGE_QUEUE_NAME = "name";
        const char *const MESSAGE_QUEUE_PROCESS_NAME = "process_name";
        const char *const MESSAGE_QUEUE_MAX_MESSAGES = "max_messages";
        const char *const MESSAGE_QUEUE_REMOVE_ON_SHUTDOWN = "remove_on_shutdown";
    } shape;

public:
    MessageQueueComponent();

    std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                              const std::string &fileName,
                                              int32_t lineOffset) override;
};

#endif  // CPP_BASE_LIBRARY_MESSAGEQUEUECOMPONENT_H
