#include "lintel/features/base/controller/SharedMemoryRepositoryDto.h"

#include "lintel/core/utils/JsonUtils.h"

namespace lintel {
SharedMemoryRepositoryDto::Shapes SharedMemoryRepositoryDto::m_shape{};

const std::string &SharedMemoryRepositoryDto::getName() const { return m_name; }

const std::string &SharedMemoryRepositoryDto::getSegmentName() const { return m_segmentName; }

SharedMemoryType SharedMemoryRepositoryDto::getType() const { return m_type; }

size_t SharedMemoryRepositoryDto::getSize() const { return m_size; }

const std::string &SharedMemoryRepositoryDto::getUuid() const { return m_uuid; }

int32_t SharedMemoryRepositoryDto::getCurrentVersion() const { return m_currentVersion; }

SharedMemoryRepositoryDto::SharedMemoryRepositoryDto(const SharedMemoryRepository &repository)
  : m_uuid(repository.getUuid()), m_name(repository.getSharedMemoryRepository()),
    m_segmentName(repository.getSharedMemorySegment()->getName()), m_type(repository.getType()),
    m_size(repository.getSizeOfData()), m_currentVersion(repository.getCodeVersion())
{}

void SharedMemoryRepositoryDto::serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartObject();
  // UUID
  writer->String(m_shape.UUID);
  writer->String(m_uuid.c_str());
  // NAME
  writer->String(m_shape.NAME);
  writer->String(m_name.c_str());
  // SEGMENT_NAME
  writer->String(m_shape.SEGMENT_NAME);
  writer->String(m_segmentName.c_str());
  // TYPE
  writer->String(m_shape.TYPE);
  writer->String(std::string(magic_enum::enum_name<>(m_type)).c_str());
  // SIZE
  writer->String(m_shape.SIZE);
  writer->String(std::to_string(m_size).c_str());
  // VERSION
  writer->String(m_shape.VERSION);
  writer->Int(m_currentVersion);
  writer->EndObject();
}

bool SharedMemoryRepositoryDto::deserialize(const rapidjson::Value &obj)
{
  bool success = true;
  // UUID
  if (!json::getString(obj, m_shape.UUID, m_uuid)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.UUID);
  }
  // NAME
  if (!json::getString(obj, m_shape.NAME, m_name)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.NAME);
  }
  // SEGMENT_NAME
  if (!json::getString(obj, m_shape.SEGMENT_NAME, m_segmentName)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.SEGMENT_NAME);
  }
  // TYPE
  std::string enumName;
  if (json::getString(obj, m_shape.TYPE, enumName)) {
    auto enumValue = magic_enum::enum_cast<SharedMemoryType>(std::string_view(enumName));
    if (enumValue.has_value()) { m_type = enumValue.value(); }
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.TYPE);
  }
  // SIZE
  std::string sizeString;
  if (json::getString(obj, m_shape.SIZE, sizeString)) {
    m_size = std::stoul(sizeString);
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.SIZE);
  }
  // VERSION
  if (!json::getInt(obj, m_shape.VERSION, m_currentVersion)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.VERSION);
  }
  return success;
}
}// namespace lintel
