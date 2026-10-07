#ifndef LINTEL_EVENTBUSCOMPONENT_H
#define LINTEL_EVENTBUSCOMPONENT_H

#include <memory>

#include "lintel/features/base/configuration/Component.h"
#include "lintel/features/base/configuration/Entry.h"
#include "lintel/features/base/configuration/EnvironmentConfiguration.h"

/** Component for parsing event bus configurations from XML */
class EventBusComponent : public Component {
private:
    /** XML element name constants for event bus parsing */
    static const struct Shapes {
        const char *const ROOT = "EventBuses";
        const char *const EVENT_BUS_ROOT = "EventBus";
        const char *const EVENT_BUS_NAME = "name";
        const char *const EVENT_BUS_SEGMENT = "segment";
        const char *const EVENT_BUS_CAPACITY = "bus_capacity";
        const char *const EVENT_BUS_SUBSCRIBER_CAPACITY = "subscriber_capacity";
        const char *const EVENT_BUS_MAX_TOPICS = "max_topics";
        const char *const EVENT_BUS_MAX_SUBSCRIBERS = "max_subscribers";
    } shape;

public:
    /** Construct an EventBusComponent */
    EventBusComponent();

    /** Parse event bus configuration XML
     * @param content XML content to parse
     * @param fileName Source file name for error reporting
     * @param lineOffset Line offset for error reporting
     * @return Vector of parsed event bus entries */
    std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                              const std::string &fileName,
                                              int32_t lineOffset) override;
};

#endif  // LINTEL_EVENTBUSCOMPONENT_H
