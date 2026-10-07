#include "lintel/features/base/controller/SharedMemorySegmentsDto.h"

SharedMemorySegmentsDto::SharedMemorySegmentsDto(
        std::vector<SharedMemorySegmentDto> segments)
        : m_segments(std::move(segments)) {}

const std::vector<SharedMemorySegmentDto> &
SharedMemorySegmentsDto::getSegments() const {
    return m_segments;
}

void SharedMemorySegmentsDto::serialize(
        rapidjson::Writer<rapidjson::StringBuffer> *writer) const {
    writer->StartArray();
    for (const auto &segment: m_segments) {
        segment.serialize(writer);
    }
    writer->EndArray();
}

void SharedMemorySegmentsDto::deserialize(const std::string &json) {
    rapidjson::Document document;
    document.Parse(json.c_str());
    if (!document.IsArray()) {
        return;
    }
    for (const auto &segment: document.GetArray()) {
        SharedMemorySegmentDto dto;
        dto.deserialize(segment);
        m_segments.push_back(dto);
    }
}
