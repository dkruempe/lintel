#include "lintel/features/base/controller/ProcessGroupDto.h"

#include "lintel/core/services/LoggerService.h"
#include "lintel/core/utils/JsonUtils.h"

namespace lintel {
ProcessGroupDto::Shapes ProcessGroupDto::m_shape{};

ProcessGroupDto::ProcessGroupDto(const std::shared_ptr<ProcessGroup> &processGroup,
  const std::vector<ProcessInfo> &processInfos)
  : m_id(processGroup->getId()), m_name(processGroup->getName()), m_processInfosDto(processInfos)
{}

void ProcessGroupDto::serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartObject();
  // ID
  writer->String(m_shape.ID);
  writer->String(m_id.c_str());
  // NAME
  writer->String(m_shape.NAME);
  writer->String(m_name.c_str());
  // PROCESSES
  writer->String(m_shape.PROCESSES);
  m_processInfosDto.serialize(writer);
  writer->EndObject();
}

bool ProcessGroupDto::deserialize(const rapidjson::Value &obj)
{
  bool success = true;
  // ID
  if (!json::getString(obj, m_shape.ID, m_id)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.ID);
  }
  // NAME
  if (!json::getString(obj, m_shape.NAME, m_name)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.NAME);
  }
  // PROCESSES
  if (const auto *processes = json::member(obj, m_shape.PROCESSES); processes != nullptr && processes->IsArray()) {
    ProcessInfosDto processInfosDto;
    processInfosDto.deserialize(*processes);
    m_processInfosDto = processInfosDto;
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.PROCESSES);
  }
  return success;
}

const std::string &ProcessGroupDto::getId() const { return m_id; }

const std::string &ProcessGroupDto::getName() const { return m_name; }

const ProcessInfosDto &ProcessGroupDto::getProcessInfosDto() const { return m_processInfosDto; }
}// namespace lintel
