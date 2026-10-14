#include "lintel/features/base/controller/HistoryDto.h"
#include <chrono>

#include "lintel/core/services/LoggerService.h"
#include "lintel/core/services/StringifyService.h"
#include "lintel/core/utils/JsonUtils.h"
#include "lintel/features/base/models/HistoryEntry.h"

namespace lintel {
HistoryDto::HistoryDto(const HistoryEntry &historyEntry)
  : m_processName(historyEntry.getProcessName()), m_serviceName(historyEntry.getServiceName()),
    m_label(historyEntry.getLabel()), m_text(historyEntry.getText()), m_uuid(historyEntry.getUuid()),
    m_createdTimestamp(historyEntry.getCreatedTimestamp())
{}

void HistoryDto::serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartObject();
  // PROCESS_NAME
  writer->String(m_shape.PROCESS_NAME);
  writer->String(m_processName.c_str());
  // SERVICE_NAME
  writer->String(m_shape.SERVICE_NAME);
  writer->String(m_serviceName.c_str());
  // LABEL
  writer->String(m_shape.LABEL);
  writer->String(m_label.c_str());
  // TEXT
  writer->String(m_shape.TEXT);
  writer->String(m_text.c_str());
  // UUID
  writer->String(m_shape.UUID);
  writer->String(m_uuid.c_str());
  // CREATED_TIMESTAMP
  writer->String(m_shape.CREATED_TIMESTAMP);
  writer->String(
    StringifyService<std::chrono::sys_time<std::chrono::microseconds>>::serializeToString(m_createdTimestamp).c_str());
  writer->EndObject();
}

bool HistoryDto::deserialize(const rapidjson::Value &obj)
{
  bool success = true;
  if (!json::getString(obj, m_shape.PROCESS_NAME, m_processName)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.PROCESS_NAME);
  }
  if (!json::getString(obj, m_shape.SERVICE_NAME, m_serviceName)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.SERVICE_NAME);
  }
  if (!json::getString(obj, m_shape.LABEL, m_label)) {
    success = false;
    LOG_ERROR("{} not defined in json serilization", m_shape.LABEL);
  }
  if (!json::getString(obj, m_shape.TEXT, m_text)) {
    success = false;
    LOG_ERROR("{} not defined in json seriliazation", m_shape.TEXT);
  }
  if (!json::getString(obj, m_shape.UUID, m_uuid)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.UUID);
  }
  std::string createdTimestamp;
  if (json::getString(obj, m_shape.CREATED_TIMESTAMP, createdTimestamp)) {
    m_createdTimestamp =
      StringifyService<std::chrono::sys_time<std::chrono::microseconds>>::deserializeFromString(createdTimestamp);
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.CREATED_TIMESTAMP);
  }
  return success;
}
}// namespace lintel
