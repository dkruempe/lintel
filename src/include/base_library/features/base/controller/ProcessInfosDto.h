#ifndef CPP_BASE_LIBRARY_PROCESSINFOSDTO_H
#define CPP_BASE_LIBRARY_PROCESSINFOSDTO_H

#include <memory>
#include <vector>

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/controller/ProcessInfoDto.h"
#include "base_library/features/base/models/ProcessInfo.h"

class ProcessInfosDto : public JsonSerializable {
 private:
  std::vector<ProcessInfoDto> m_processInfos;

  static std::vector<ProcessInfoDto> build(
      const std::vector<ProcessInfo> &processInfos);

 public:
  explicit ProcessInfosDto(const std::vector<ProcessInfo> &processInfos);
  ProcessInfosDto() = default;

  [[nodiscard]] const std::vector<ProcessInfoDto> &getProcessInfos() const;

  void serialize(
      rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

  bool deserialize(const rapidjson::Value& obj) override;
};

#endif  // CPP_BASE_LIBRARY_PROCESSINFOSDTO_H
