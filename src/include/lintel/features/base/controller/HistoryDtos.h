#ifndef HISTORYDTOS_H
#define HISTORYDTOS_H

#include <vector>
#include "HistoryDto.h"
#include "lintel/core/models/JsonSerializable.h"
#include "lintel/features/base/models/HistoryEntry.h"

/** DTO representing a collection of history entries */
class HistoryDtos : public JsonSerializable {
    /** The history DTOs */
    std::vector<HistoryDto> m_histories;

    /** Build history DTOs from HistoryEntry models
     * @param entries The source history entries
     * @return Vector of history DTOs */
    static std::vector<HistoryDto> build(const std::vector<HistoryEntry> entries);

public:
    /** Construct from HistoryEntry models
     * @param entries The source history entries */
    explicit HistoryDtos(const std::vector<HistoryEntry> entries);

    /** Default constructor */
    HistoryDtos() = default;

    /** Get the history DTOs
     * @return Vector of history DTOs */
    [[nodiscard]] const std::vector<HistoryDto> &getHistories() const;

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON string
     * @param json The JSON string to parse */
    void deserialize(const std::string &json) override;
};

#endif //HISTORYDTOS_H
