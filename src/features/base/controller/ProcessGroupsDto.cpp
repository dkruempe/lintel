#include "base_library/features/base/controller/ProcessGroupsDto.h"

#include <utility>

ProcessGroupsDto::ProcessGroupsDto(std::vector<ProcessGroupDto> processGroups)
    : m_processGroups(std::move(processGroups)) {}

void ProcessGroupsDto::serialize(
    rapidjson::Writer<rapidjson::StringBuffer>* writer) const {
  writer->StartArray();
  for (const auto &iter : m_processGroups) {
    iter.serialize(writer);
  }
  writer->EndArray();
}
void ProcessGroupsDto::deserialize(const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());
  if (!document.IsArray()) {
    return;
  }
  for (const auto& iter : document.GetArray()) {
    ProcessGroupDto processGroupDto;
    processGroupDto.deserialize(iter);
    m_processGroups.push_back(processGroupDto);
  }
}
