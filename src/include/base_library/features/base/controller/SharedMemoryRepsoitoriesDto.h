#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYREPSOITORIESDTO_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYREPSOITORIESDTO_H

#include <vector>

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/controller/SharedMemoryRepositoryDto.h"

/** DTO representing a collection of shared memory repositories */
class SharedMemoryRepositoriesDto : public JsonSerializable {
private:
    /** The repository DTOs */
    std::vector<SharedMemoryRepositoryDto> m_repositories;

public:
    /** Construct from existing SharedMemoryRepositoryDto objects
     * @param repositories The repository DTOs */
    explicit SharedMemoryRepositoriesDto(
            std::vector<SharedMemoryRepositoryDto> repositories);

    /** Default constructor */
    SharedMemoryRepositoriesDto() = default;

    /** Get the repository DTOs
     * @return Vector of repository DTOs */
    [[nodiscard]] const std::vector<SharedMemoryRepositoryDto> &getRepositories() const;

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON string
     * @param json The JSON string to parse */
    void deserialize(const std::string &json) override;
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYREPSOITORIESDTO_H
