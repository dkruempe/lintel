#ifndef HISTORYDTO_H
#define HISTORYDTO_H

#include <string>
#include <date/date.h>

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/models/HistoryEntry.h"

class HistoryDto : public JsonSerializable {
    std::string m_processName;
    std::string m_serviceName;
    std::string m_label;
    std::string m_text;
    std::string m_uuid;
    date::sys_time<std::chrono::microseconds> m_createdTimestamp;

    static struct Shapes {
        const std::string PROCESS_NAME = "process_name";
        const std::string SERVICE_NAME = "service_name";
        const std::string LABEL = "label";
        const std::string TEXT = "text";
        const std::string UUID = "uuid";
        const std::string CREATED_TIMESTAMP = "created_timestamp";
    } m_shape;

public:
    HistoryDto(const HistoryEntry &historyEntry);

    HistoryDto() = default;

    [[nodiscard]] std::string getProcessName() const {
        return m_processName;
    }

    [[nodiscard]] std::string getServiceName() const {
        return m_serviceName;
    }

    [[nodiscard]] std::string getLabel() const {
        return m_label;
    }

    [[nodiscard]] std::string getText() const {
        return m_text;
    }

    [[nodiscard]] std::string getUuid() const {
        return m_uuid;
    }

    [[nodiscard]] date::sys_time<std::chrono::microseconds> getCreatedTimestamp() const {
        return m_createdTimestamp;
    }


    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    bool deserialize(const rapidjson::Value &obj) override;
};

#endif //HISTORYDTO_H
