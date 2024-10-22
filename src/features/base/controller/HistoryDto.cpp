#include "base_library/features/base/controller/HistoryDto.h"

#include "base_library/core/services/LoggerService.h"
#include "base_library/core/services/StringifyService.h"
#include "base_library/features/base/models/HistoryEntry.h"

HistoryDto::HistoryDto(const HistoryEntry &historyEntry) : m_processName(historyEntry.getProcessName()),
                                                           m_serviceName(historyEntry.getServiceName()),
                                                           m_label(historyEntry.getLabel()),
                                                           m_text(historyEntry.getText()),
                                                           m_uuid(historyEntry.getUuid()),
                                                           m_createdTimestamp(historyEntry.getCreatedTimestamp()) {
}

void HistoryDto::serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const {
    writer->StartObject();
    // PROCESS_NAME
    writer->String(m_shape.PROCESS_NAME.c_str());
    writer->String(m_processName.c_str());
    // SERVICE_NAME
    writer->String(m_shape.SERVICE_NAME.c_str());
    writer->String(m_serviceName.c_str());
    // LABEL
    writer->String(m_shape.LABEL.c_str());
    writer->String(m_label.c_str());
    // TEXT
    writer->String(m_shape.TEXT.c_str());
    writer->String(m_text.c_str());
    // UUID
    writer->String(m_shape.UUID.c_str());
    writer->String(m_uuid.c_str());
    // CREATED_TIMESTAMP
    writer->String(m_shape.CREATED_TIMESTAMP.c_str());
    writer->String(
        StringifyService<date::sys_time<std::chrono::microseconds> >::serializeToString(
            m_createdTimestamp).c_str());
    writer->EndObject();
}

bool HistoryDto::deserialize(const rapidjson::Value &obj) {
    bool success = true;
    if (obj.HasMember(m_shape.PROCESS_NAME.c_str())) {
        m_processName = obj[m_shape.PROCESS_NAME.c_str()].GetString();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization", m_shape.PROCESS_NAME);
    }
    if (obj.HasMember(m_shape.SERVICE_NAME.c_str())) {
        m_serviceName = obj[m_shape.SERVICE_NAME.c_str()].GetString();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization", m_shape.SERVICE_NAME);
    }
    if (obj.HasMember(m_shape.LABEL.c_str())) {
        m_label = obj[m_shape.LABEL.c_str()].GetString();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serilization", m_shape.LABEL);
    }
    if (obj.HasMember(m_shape.TEXT.c_str())) {
        m_text = obj[m_shape.TEXT.c_str()].GetString();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json seriliazation", m_shape.TEXT);
    }
    if (obj.HasMember(m_shape.UUID.c_str())) {
        m_uuid = obj[m_shape.UUID.c_str()].GetString();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization", m_shape.UUID);
    }
    if (obj.HasMember(m_shape.CREATED_TIMESTAMP.c_str())) {
        m_createdTimestamp = StringifyService<date::sys_time<std::chrono::microseconds> >::deserializeFromString(
            obj[m_shape.CREATED_TIMESTAMP.c_str()].GetString());
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization", m_shape.CREATED_TIMESTAMP);
    }
    return success;
}
