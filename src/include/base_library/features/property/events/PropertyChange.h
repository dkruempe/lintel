#ifndef CPP_BASE_LIBRARY_PROPERTYCHANGE_H
#define CPP_BASE_LIBRARY_PROPERTYCHANGE_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

#include "base_library/features/base/events/Event.h"
#include "base_library/features/property/models/PropertyBase.h"

/**
 * Fixed-size change notification for a property, exchanged over the EventBus.
 *
 * Carries the property coordinates and the new value as a string. The struct
 * is trivially copyable and fits into the Event payload, so it can be stored
 * inside the lock-free queues of the bus.
 *
 * Values longer than the value buffer are marked as truncated
 * (m_valueTruncated). The receiving side must then resolve the full value from
 * a locked source (e.g. the shared memory repository) instead of using the
 * truncated string; a truncated change is never applied with a wrong value.
 */
struct PropertyChange {
    /** Buffer size for each identifier part. */
    static constexpr std::size_t PART_SIZE = 32;
    /** Buffer size for the serialized value string (including terminator). */
    static constexpr std::size_t VALUE_SIZE =
            Event::CONTENT_SIZE - 4 * PART_SIZE - 1;
    /** Maximum length of the value that fits into the buffer. */
    static constexpr std::size_t MAX_VALUE_LENGTH = VALUE_SIZE - 1;

    char processName[PART_SIZE]{};
    char className[PART_SIZE]{};
    char instanceName[PART_SIZE]{};
    char name[PART_SIZE]{};
    char value[VALUE_SIZE]{};
    /** True if the value did not fit into the buffer and was truncated. */
    bool m_valueTruncated = false;

    /** Create a change notification from a property.
     * @param property the property that was changed
     * @return the change notification carrying the new value */
    static PropertyChange of(PropertyBase &property) {
        PropertyChange change;
        copyInto(change.processName, property.getProcessName());
        copyInto(change.className, property.getClassName());
        copyInto(change.instanceName, property.getInstanceName());
        copyInto(change.name, property.getName());
        const std::string value = property.toString();
        change.m_valueTruncated = value.size() >= VALUE_SIZE;
        copyInto(change.value, value);
        return change;
    }

    /** @return the unique identifier name_instanceName_className_processName */
    [[nodiscard]] std::string identifier() const {
        return std::string(name) + "_" + instanceName + "_" + className + "_" +
               processName;
    }

private:
    static void copyInto(char (&destination)[PART_SIZE],
                         const std::string &value) {
        const std::size_t length =
                value.size() >= PART_SIZE ? PART_SIZE - 1 : value.size();
        std::memcpy(destination, value.data(), length);
        destination[length] = '\0';
    }

    static void copyInto(char (&destination)[VALUE_SIZE],
                         const std::string &value) {
        const std::size_t length =
                value.size() >= VALUE_SIZE ? VALUE_SIZE - 1 : value.size();
        std::memcpy(destination, value.data(), length);
        destination[length] = '\0';
    }
};

static_assert(std::is_trivially_copyable_v<PropertyChange>,
              "PropertyChange must be trivially copyable for the lock-free "
              "queues");
static_assert(sizeof(PropertyChange) == Event::CONTENT_SIZE,
              "PropertyChange must exactly fit into the Event payload");

#endif  // CPP_BASE_LIBRARY_PROPERTYCHANGE_H
