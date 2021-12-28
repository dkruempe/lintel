#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYREPSOITORIESDTO_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYREPSOITORIESDTO_H

#include <vector>

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/controller/SharedMemoryRepositoryDto.h"

class SharedMemoryRepositoriesDto : public JsonSerializable {
 private:
  std::vector<SharedMemoryRepositoryDto> m_repositories;

 public:
  explicit SharedMemoryRepositoriesDto(
      std::vector<SharedMemoryRepositoryDto> repositories);
  SharedMemoryRepositoriesDto() = default;

  [[nodiscard]] const std::vector<SharedMemoryRepositoryDto> &getRepositories() const;

  void serialize(
      rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;
  void deserialize(const std::string &json) override;
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYREPSOITORIESDTO_H
