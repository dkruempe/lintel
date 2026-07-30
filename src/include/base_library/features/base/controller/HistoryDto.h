#ifndef HISTORYDTO_H
#define HISTORYDTO_H

#include <string>
#include <date/date.h>

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/models/HistoryEntry.h"

/** DTO representing a single history entry */
class HistoryDto : public JsonSerializable {
    /** The process name */
    std::string m_processName;
    /** The service name */
    std::string m_serviceName;
    /** The label */
    std::string m_label;
    /** The text content */
    std::string m_text;
    /** The UUID */
    std::string m_uuid;
    /** The creation timestamp */
    date::sys_time<std::chrono::microseconds> m_createdTimestamp;

    /** JSON field name constants */
    static struct Shapes {
        const char *const PROCESS_NAME = "process_name";
        const char *const SERVICE_NAME = "service_name";
        const char *const LABEL = "label";
        const char *const TEXT = "text";
        const char *const UUID = "uuid";
        const char *const CREATED_TIMESTAMP = "created_timestamp";
    } m_shape;

public:
    /** Construct from a HistoryEntry model
     * @param historyEntry The source history entry */
    HistoryDto(const HistoryEntry &historyEntry);

    /** Default constructor */
    HistoryDto() = default;

    /** Get the process name
     * @return The process name */
    [[nodiscard]] std::string getProcessName() const {
        return m_processName;
    }

    /** Get the service name
     * @return The service name */
    [[nodiscard]] std::string getServiceName() const {
        return m_serviceName;
    }

    /** Get the label
     * @return The label */
    [[nodiscard]] std::string getLabel() const {
        return m_label;
    }

    /** Get the text content
     * @return The text */
    [[nodiscard]] std::string getText() const {
        return m_text;
    }

    /** Get the UUID
     * @return The UUID string */
    [[nodiscard]] std::string getUuid() const {
        return m_uuid;
    }

    /** Get the creation timestamp
     * @return The timestamp */
    [[nodiscard]] date::sys_time<std::chrono::microseconds> getCreatedTimestamp() const {
        return m_createdTimestamp;
    }

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON
     * @param obj The JSON value
     * @return True on success */
    bool deserialize(const rapidjson::Value &obj) override;
};

#endif //HISTORYDTO_H
