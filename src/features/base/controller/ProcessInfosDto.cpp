#include "base_library/features/base/controller/ProcessInfosDto.h"

#include "base_library/core/services/LoggerService.h"

const std::vector<ProcessInfoDto>& ProcessInfosDto::getProcessInfos() const {
  return m_processInfos;
}
ProcessInfosDto::ProcessInfosDto(const std::vector<ProcessInfo>& processInfos)
    : m_processInfos(build(processInfos)) {}
std::vector<ProcessInfoDto> ProcessInfosDto::build(
    const std::vector<ProcessInfo>& processInfos) {
  std::vector<ProcessInfoDto> temp;
  temp.reserve(processInfos.size());
  for (const auto& iter : processInfos) {
    temp.emplace_back(iter);
  }
  return temp;
}
void ProcessInfosDto::serialize(
    rapidjson::Writer<rapidjson::StringBuffer>* writer) const {
  writer->StartArray();
  for (const auto& iter : m_processInfos) {
    iter.serialize(writer);
  }
  writer->EndArray();
}
void ProcessInfosDto::deserialize(const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());
  if (!document.IsArray()) {
    LOG_WARN("document is not an array");
    return;
  }
  for (const auto& iter : document.GetArray()) {
    ProcessInfoDto processInfoDto;
    processInfoDto.deserialize(iter);
    m_processInfos.push_back(processInfoDto);
  }
}
