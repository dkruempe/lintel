#include "base_library/features/base/controller/ProcessGroupDto.h"

#include "base_library/core/services/LoggerService.h"
ProcessGroupDto::Shapes ProcessGroupDto::m_shape{};
ProcessGroupDto::ProcessGroupDto(
    const std::shared_ptr<ProcessGroup> &processGroup,
    const std::vector<ProcessInfo> &processInfos)
    : m_id(processGroup->getId()),
      m_name(processGroup->getName()),
      m_processInfosDto(processInfos) {}
void ProcessGroupDto::serialize(
    rapidjson::Writer<rapidjson::StringBuffer> *writer) const {
  writer->StartObject();
  // ID
  writer->String(m_shape.ID.c_str());
  writer->String(m_id.c_str());
  // NAME
  writer->String(m_shape.NAME.c_str());
  writer->String(m_name.c_str());
  // PROCESSES
  writer->String(m_shape.PROCESSES.c_str());
  m_processInfosDto.serialize(writer);
  writer->EndObject();
}
bool ProcessGroupDto::deserialize(const rapidjson::Value &obj) {
  bool success = true;
  // ID
  if (obj.HasMember(m_shape.ID.c_str())) {
    m_id = obj[m_shape.ID.c_str()].GetString();
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.ID.c_str());
  }
  // NAME
  if (obj.HasMember(m_shape.NAME.c_str())) {
    m_name = obj[m_shape.NAME.c_str()].GetString();
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.NAME.c_str());
  }
  // PROCESSES
  if (obj.HasMember(m_shape.PROCESSES.c_str())) {
    ProcessInfosDto processInfosDto;
    processInfosDto.deserialize(obj[m_shape.PROCESSES.c_str()]);
    m_processInfosDto = processInfosDto;
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization",
              m_shape.PROCESSES.c_str());
  }
  return success;
}
const std::string &ProcessGroupDto::getId() const { return m_id; }
const std::string &ProcessGroupDto::getName() const { return m_name; }
const ProcessInfosDto &ProcessGroupDto::getProcessInfosDto() const {
  return m_processInfosDto;
}
