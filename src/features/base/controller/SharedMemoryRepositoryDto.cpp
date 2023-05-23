#include "base_library/features/base/controller/SharedMemoryRepositoryDto.h"

SharedMemoryRepositoryDto::Shapes SharedMemoryRepositoryDto::m_shape{};

const std::string &SharedMemoryRepositoryDto::getName() const { return m_name; }

const std::string &SharedMemoryRepositoryDto::getSegmentName() const {
    return m_segmentName;
}

SharedMemoryType SharedMemoryRepositoryDto::getType() const { return m_type; }

size_t SharedMemoryRepositoryDto::getSize() const { return m_size; }

int32_t SharedMemoryRepositoryDto::getCurrentVersion() const {
    return m_currentVersion;
}

SharedMemoryRepositoryDto::SharedMemoryRepositoryDto(
        const SharedMemoryRepository &repository)
        : m_name(repository.getSharedMemoryRepository()),
          m_segmentName(repository.getSharedMemorySegment()->getName()),
          m_type(repository.getType()),
          m_size(repository.getSizeOfData()),
          m_currentVersion(repository.getCodeVersion()) {}

void SharedMemoryRepositoryDto::serialize(
        rapidjson::Writer<rapidjson::StringBuffer> *writer) const {
    writer->StartObject();
    // NAME
    writer->String(m_shape.NAME.c_str());
    writer->String(m_name.c_str());
    // SEGMENT_NAME
    writer->String(m_shape.SEGMENT_NAME.c_str());
    writer->String(m_segmentName.c_str());
    // TYPE
    writer->String(m_shape.TYPE.c_str());
    writer->String(std::string(magic_enum::enum_name<>(m_type)).c_str());
    // SIZE
    writer->String(m_shape.SIZE.c_str());
    writer->String(std::to_string(m_size).c_str());
    // VERSION
    writer->String(m_shape.VERSION.c_str());
    writer->Int(m_currentVersion);
    writer->EndObject();
}

bool SharedMemoryRepositoryDto::deserialize(const rapidjson::Value &obj) {
    bool success = true;
    // NAME
    if (obj.HasMember(m_shape.NAME.c_str())) {
        m_name = obj[m_shape.NAME.c_str()].GetString();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization", m_shape.NAME);
    }
    // SEGMENT_NAME
    if (obj.HasMember(m_shape.SEGMENT_NAME.c_str())) {
        m_segmentName = obj[m_shape.SEGMENT_NAME.c_str()].GetString();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization", m_shape.SEGMENT_NAME);
    }
    // TYPE
    if (obj.HasMember(m_shape.TYPE.c_str())) {
        std::string_view enumName = obj[m_shape.TYPE.c_str()].GetString();

        m_type = magic_enum::enum_cast<SharedMemoryType>(enumName).value();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization", m_shape.TYPE);
    }
    // SIZE
    if (obj.HasMember(m_shape.SIZE.c_str())) {
        m_size = std::stoul(obj[m_shape.SIZE.c_str()].GetString());
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization", m_shape.SIZE);
    }
    // VERSION
    if (obj.HasMember(m_shape.VERSION.c_str())) {
        m_currentVersion = obj[m_shape.VERSION.c_str()].GetInt();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization", m_shape.VERSION);
    }
    return success;
}
