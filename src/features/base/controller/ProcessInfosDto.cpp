#include "base_library/features/base/controller/ProcessInfosDto.h"

#include "base_library/core/services/LoggerService.h"

const std::vector<ProcessInfoDto> &ProcessInfosDto::getProcessInfos() const { return m_processInfos; }

ProcessInfosDto::ProcessInfosDto(const std::vector<ProcessInfo> &processInfos)
  : m_processInfos(build(processInfos)) {}

std::vector<ProcessInfoDto> ProcessInfosDto::build(
  const std::vector<ProcessInfo> &processInfos)
{
  std::vector<ProcessInfoDto> temp;
  temp.reserve(processInfos.size());
  std::transform(processInfos.begin(),
    processInfos.end(),
    std::back_inserter(temp),
    [](const ProcessInfo &p) { return ProcessInfoDto(p); });
  return temp;
}

void ProcessInfosDto::serialize(
  rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartArray();
  for (const auto &iter : m_processInfos) { iter.serialize(writer); }
  writer->EndArray();
}

bool ProcessInfosDto::deserialize(const rapidjson::Value &obj)
{
  for (const auto &iter : obj.GetArray()) {
    ProcessInfoDto processInfoDto;
    processInfoDto.deserialize(iter);
    m_processInfos.push_back(processInfoDto);
  }
  return true;
}