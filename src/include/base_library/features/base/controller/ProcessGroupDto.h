#ifndef CPP_BASE_LIBRARY_PROCESSGROUPDTO_H
#define CPP_BASE_LIBRARY_PROCESSGROUPDTO_H

#include "ProcessInfosDto.h"
#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/models/ProcessGroup.h"

class ProcessGroupDto : public JsonSerializable {
private:
    std::string m_id;
    std::string m_name;
    ProcessInfosDto m_processInfosDto;

    static struct Shapes {
        const char *const ID = "id";
        const char *const NAME = "name";
        const char *const PROCESSES = "processes";
    } m_shape;

public:
    ProcessGroupDto(const std::shared_ptr<ProcessGroup> &processGroup,
                    const std::vector<ProcessInfo> &processInfos);

    ProcessGroupDto() = default;

    [[nodiscard]] const std::string &getId() const;

    [[nodiscard]] const std::string &getName() const;

    [[nodiscard]] const ProcessInfosDto &getProcessInfosDto() const;

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    bool deserialize(const rapidjson::Value &obj) override;
};

#endif  // CPP_BASE_LIBRARY_PROCESSGROUPDTO_H
