#ifndef CPP_BASE_LIBRARY_PROCESSINFOSDTO_H
#define CPP_BASE_LIBRARY_PROCESSINFOSDTO_H

#include <memory>
#include <vector>

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/controller/ProcessInfoDto.h"
#include "base_library/features/base/models/ProcessInfo.h"

/** DTO representing a collection of process info entries */
class ProcessInfosDto : public JsonSerializable {
private:
    /** The process info DTOs */
    std::vector<ProcessInfoDto> m_processInfos;

    /** Build ProcessInfoDto objects from ProcessInfo models
     * @param processInfos The source process infos
     * @return Vector of process info DTOs */
    static std::vector<ProcessInfoDto> build(
            const std::vector<ProcessInfo> &processInfos);

public:
    /** Construct from ProcessInfo models
     * @param processInfos The source process infos */
    explicit ProcessInfosDto(const std::vector<ProcessInfo> &processInfos);

    /** Default constructor */
    ProcessInfosDto() = default;

    /** Get the process info DTOs
     * @return Vector of process info DTOs */
    [[nodiscard]] const std::vector<ProcessInfoDto> &getProcessInfos() const;

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON
     * @param obj The JSON value
     * @return True on success */
    bool deserialize(const rapidjson::Value &obj) override;
};

#endif  // CPP_BASE_LIBRARY_PROCESSINFOSDTO_H
