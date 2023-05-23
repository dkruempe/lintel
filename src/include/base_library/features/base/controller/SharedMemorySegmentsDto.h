#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTSDTO_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTSDTO_H

#include <vector>

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/controller/SharedMemorySegmentDto.h"

class SharedMemorySegmentsDto : public JsonSerializable {
private:
    std::vector<SharedMemorySegmentDto> m_segments;

public:
    explicit SharedMemorySegmentsDto(
            std::vector<SharedMemorySegmentDto> segments);

    SharedMemorySegmentsDto() = default;

    [[nodiscard]] const std::vector<SharedMemorySegmentDto> &getSegments() const;

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    void deserialize(const std::string &json) override;
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTSDTO_H
