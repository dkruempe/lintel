#include "base_library/features/base/configuration/EventBusComponent.h"

#include <base_library/core/utils/TypeName.h>
#include <base_library/features/base/configuration/ConfigurationException.h>
#include <base_library/features/base/configuration/EventBusEntry.h>
#include <base_library/features/base/events/EventBus.h>
#include <cstring>
#include <tinyxml2.h>

const EventBusComponent::Shapes EventBusComponent::shape{};

EventBusComponent::EventBusComponent() : Component(shape.ROOT) {}

namespace {
std::size_t eventBusAttributeSize(tinyxml2::XMLElement *element,
                                  const char *attribute,
                                  std::size_t defaultValue) {
    const char *value = element->Attribute(attribute);
    if (value == nullptr) {
        return defaultValue;
    }
    return static_cast<std::size_t>(std::stoull(value));
}
}  // namespace

std::vector<std::shared_ptr<Entry>> EventBusComponent::parse(
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

    for (tinyxml2::XMLElement *busElement = rootNode->FirstChildElement();
         busElement != nullptr; busElement = busElement->NextSiblingElement()) {
        if (std::strcmp(busElement->Name(), shape.EVENT_BUS_ROOT) != 0) {
            continue;
        }
        const char *name = busElement->Attribute(shape.EVENT_BUS_NAME);
        const char *segment = busElement->Attribute(shape.EVENT_BUS_SEGMENT);

        int32_t const lineNumber = busElement->GetLineNum() + lineOffset - 1;

        if (name == nullptr) {
            throw ConfigurationException(this->getConfigRoot(),
                                         "please set name for event bus",
                                         lineNumber);
        }
        if (segment == nullptr) {
            throw ConfigurationException(this->getConfigRoot(),
                                         "please set segment for event bus",
                                         lineNumber);
        }

        const std::size_t busCapacity = eventBusAttributeSize(
                busElement, shape.EVENT_BUS_CAPACITY, 128);
        const std::size_t subscriberCapacity = eventBusAttributeSize(
                busElement, shape.EVENT_BUS_SUBSCRIBER_CAPACITY, 128);
        const std::size_t maxTopics = eventBusAttributeSize(
                busElement, shape.EVENT_BUS_MAX_TOPICS,
                EventBusLimits::MAX_TOPICS);
        const std::size_t maxSubscribers = eventBusAttributeSize(
                busElement, shape.EVENT_BUS_MAX_SUBSCRIBERS,
                EventBusLimits::MAX_SUBSCRIBERS);

        entries.push_back(std::make_shared<EventBusEntry>(
                type_name<EventBusComponent>(), std::string(name),
                std::string(segment), busCapacity, subscriberCapacity,
                maxTopics, maxSubscribers));
    }
    return entries;
}
