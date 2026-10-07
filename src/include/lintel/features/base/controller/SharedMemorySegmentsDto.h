#ifndef LINTEL_SHAREDMEMORYSEGMENTSDTO_H
#define LINTEL_SHAREDMEMORYSEGMENTSDTO_H

#include <vector>

#include "lintel/core/models/JsonSerializable.h"
#include "lintel/features/base/controller/SharedMemorySegmentDto.h"

/** DTO representing a collection of shared memory segments */
class SharedMemorySegmentsDto : public JsonSerializable {
private:
    /** The segment DTOs */
    std::vector<SharedMemorySegmentDto> m_segments;

public:
    /** Construct from existing SharedMemorySegmentDto objects
     * @param segments The segment DTOs */
    explicit SharedMemorySegmentsDto(
            std::vector<SharedMemorySegmentDto> segments);

    /** Default constructor */
    SharedMemorySegmentsDto() = default;

    /** Get the segment DTOs
     * @return Vector of segment DTOs */
    [[nodiscard]] const std::vector<SharedMemorySegmentDto> &getSegments() const;

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON string
     * @param json The JSON string to parse */
    void deserialize(const std::string &json) override;
};

#endif  // LINTEL_SHAREDMEMORYSEGMENTSDTO_H
