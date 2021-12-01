#ifndef CPP_BASE_LIBRARY_PROCESSGROUPSDTO_H
#define CPP_BASE_LIBRARY_PROCESSGROUPSDTO_H

#include <vector>

#include "ProcessGroupDto.h"
#include "base_library/core/models/JsonSerializable.h"

class ProcessGroupsDto : public JsonSerializable {
 private:
  std::vector<ProcessGroupDto> m_processGroups;

 public:
  explicit ProcessGroupsDto(std::vector<ProcessGroupDto> processGroups);
  ProcessGroupsDto() = default;

  [[nodiscard]] const std::vector<ProcessGroupDto> &getProcessGroups() const {
    return m_processGroups;
  }

  void serialize(
      rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;
  void deserialize(const std::string &json) override;
};

#endif  // CPP_BASE_LIBRARY_PROCESSGROUPSDTO_H
