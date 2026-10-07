#ifndef LINTEL_PROCESSGROUPSDTO_H
#define LINTEL_PROCESSGROUPSDTO_H

#include <vector>

#include "ProcessGroupDto.h"
#include "lintel/core/models/JsonSerializable.h"

/** DTO representing a collection of process groups */
class ProcessGroupsDto : public JsonSerializable {
private:
    /** The process group DTOs */
    std::vector<ProcessGroupDto> m_processGroups;

public:
    /** Construct from existing ProcessGroupDto objects
     * @param processGroups The process group DTOs */
    explicit ProcessGroupsDto(std::vector<ProcessGroupDto> processGroups);

    /** Default constructor */
    ProcessGroupsDto() = default;

    /** Get the process group DTOs
     * @return Vector of process group DTOs */
    [[nodiscard]] const std::vector<ProcessGroupDto> &getProcessGroups() const {
        return m_processGroups;
    }

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON string
     * @param json The JSON string to parse */
    void deserialize(const std::string &json) override;
};

#endif  // LINTEL_PROCESSGROUPSDTO_H
