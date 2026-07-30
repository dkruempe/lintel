#ifndef CPP_BASE_LIBRARY_PROCESSGROUPDTO_H
#define CPP_BASE_LIBRARY_PROCESSGROUPDTO_H

#include "ProcessInfosDto.h"
#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/models/ProcessGroup.h"

/** DTO representing a process group with its processes */
class ProcessGroupDto : public JsonSerializable {
private:
    /** The group ID */
    std::string m_id;
    /** The group name */
    std::string m_name;
    /** The process info DTOs belonging to this group */
    ProcessInfosDto m_processInfosDto;

    /** JSON field name constants */
    static struct Shapes {
        const char *const ID = "id";
        const char *const NAME = "name";
        const char *const PROCESSES = "processes";
    } m_shape;

public:
    /** Construct from a ProcessGroup model and its process infos
     * @param processGroup The process group model
     * @param processInfos The process infos belonging to the group */
    ProcessGroupDto(const std::shared_ptr<ProcessGroup> &processGroup,
                    const std::vector<ProcessInfo> &processInfos);

    /** Default constructor */
    ProcessGroupDto() = default;

    /** Get the group ID
     * @return The ID */
    [[nodiscard]] const std::string &getId() const;

    /** Get the group name
     * @return The name */
    [[nodiscard]] const std::string &getName() const;

    /** Get the process info DTOs
     * @return The process infos DTO */
    [[nodiscard]] const ProcessInfosDto &getProcessInfosDto() const;

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON
     * @param obj The JSON value
     * @return True on success */
    bool deserialize(const rapidjson::Value &obj) override;
};

#endif  // CPP_BASE_LIBRARY_PROCESSGROUPDTO_H
