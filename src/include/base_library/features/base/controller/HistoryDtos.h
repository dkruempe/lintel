#ifndef HISTORYDTOS_H
#define HISTORYDTOS_H

#include <vector>
#include "HistoryDto.h"
#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/models/HistoryEntry.h"

class HistoryDtos : public JsonSerializable {
    std::vector<HistoryDto> m_histories;

    static std::vector<HistoryDto> build(const std::vector<HistoryEntry> entries);

public:
    explicit HistoryDtos(const std::vector<HistoryEntry> entries);

    HistoryDtos() = default;

    [[nodiscard]] const std::vector<HistoryDto> &getHistories() const;


    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    void deserialize(const std::string &json) override;
};

#endif //HISTORYDTOS_H
