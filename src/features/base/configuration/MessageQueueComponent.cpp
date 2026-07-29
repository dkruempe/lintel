#include "base_library/features/base/configuration/MessageQueueComponent.h"

#include <base_library/core/utils/TypeName.h>
#include <base_library/features/base/configuration/ConfigurationException.h>
#include <base_library/features/base/configuration/MessageQueueEntry.h>
#include <tinyxml2.h>

const MessageQueueComponent::Shapes MessageQueueComponent::shape{};

MessageQueueComponent::MessageQueueComponent() : Component(shape.CONFIG_ROOT) {}

std::vector<std::shared_ptr<Entry>> MessageQueueComponent::parse(
        const std::string &content, const std::string &fileName,
        int32_t lineOffset) {
    std::vector<std::shared_ptr<Entry>> entries;
    tinyxml2::XMLDocument document;
    document.Parse(content.c_str());

    tinyxml2::XMLElement *rootNode =
            document.FirstChildElement(getConfigRoot().c_str());
    if (rootNode == nullptr) {
        return entries;
    }

    for (tinyxml2::XMLElement *propertyElement = rootNode->FirstChildElement();
         propertyElement != nullptr;
         propertyElement = propertyElement->NextSiblingElement()) {
        if (std::strcmp(propertyElement->Name(),
                        shape.MESSAGE_QUEUE_ROOT) != 0) {
            continue;
        }
        const char *name =
                propertyElement->Attribute(shape.MESSAGE_QUEUE_NAME);
        const char *processName =
                propertyElement->Attribute(shape.MESSAGE_QUEUE_PROCESS_NAME);
        const char *maxMessagesStr =
                propertyElement->Attribute(shape.MESSAGE_QUEUE_MAX_MESSAGES);

        int32_t const lineNumber = propertyElement->GetLineNum() + lineOffset - 1;

        if (name == nullptr) {
            throw ConfigurationException(this->getConfigRoot(),
                                         "please set name for message queue",
                                         lineNumber);
        }
        if (processName == nullptr) {
            throw ConfigurationException(getConfigRoot(),
                                         "please set process_name for message queue",
                                         lineNumber);
        }
        if (maxMessagesStr == nullptr) {
            throw ConfigurationException(getConfigRoot(),
                                         "please set max_messages for message queue",
                                         lineNumber);
        }

        const int32_t maxMessages = std::stoi(maxMessagesStr);
        entries.push_back(std::make_shared<MessageQueueEntry>(
                type_name<MessageQueueComponent>(), std::string(processName),
                std::string(name), maxMessages));
    }
    return entries;
}
