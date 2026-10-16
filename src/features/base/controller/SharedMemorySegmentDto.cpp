#include "lintel/features/base/controller/SharedMemorySegmentDto.h"

#include "lintel/core/services/LoggerService.h"
#include "lintel/core/utils/JsonUtils.h"

namespace lintel {
SharedMemorySegmentDto::Shapes SharedMemorySegmentDto::m_shape{};

SharedMemorySegmentDto::SharedMemorySegmentDto(const SharedMemorySegmentInfo &sharedMemoryInfo)
  : m_path(sharedMemoryInfo.getSharedMemorySegment()->getPath()),
    m_name(sharedMemoryInfo.getSharedMemorySegment()->getName()),
    m_size(sharedMemoryInfo.getSharedMemorySegment()->getSize()),
    m_isAutoExtend(sharedMemoryInfo.getSharedMemorySegment()->isAutoExtend()),
    m_autoExtendSize(sharedMemoryInfo.getSharedMemorySegment()->getAutoExtendSize()),
    m_maxSize(sharedMemoryInfo.getSharedMemorySegment()->getMaxSize()), m_isSanity(sharedMemoryInfo.isSanity()),
    m_currentSize(sharedMemoryInfo.getCurrentSize()), m_freeSize(sharedMemoryInfo.getFreeSize()),
    m_amountNamedObjects(sharedMemoryInfo.getAmountNamedObjects()),
    m_amountUniqueObjects(sharedMemoryInfo.getAmountUniqueObjects())
{}

const std::filesystem::path &SharedMemorySegmentDto::getPath() const { return m_path; }

const std::string &SharedMemorySegmentDto::getName() const { return m_name; }

size_t SharedMemorySegmentDto::getSize() const { return m_size; }

bool SharedMemorySegmentDto::isAutoExtend() const { return m_isAutoExtend; }

size_t SharedMemorySegmentDto::getAutoExtendSize() const { return m_autoExtendSize; }

size_t SharedMemorySegmentDto::getMaxSize() const { return m_maxSize; }

bool SharedMemorySegmentDto::isSanity() const { return m_isSanity; }

size_t SharedMemorySegmentDto::getCurrentSize() const { return m_currentSize; }

size_t SharedMemorySegmentDto::getFreeSize() const { return m_freeSize; }

size_t SharedMemorySegmentDto::getAmountNamedObjects() const { return m_amountNamedObjects; }

size_t SharedMemorySegmentDto::getAmountUniqueObjects() const { return m_amountUniqueObjects; }

const SharedMemorySegmentDto::Shapes &SharedMemorySegmentDto::getShape() { return m_shape; }

void SharedMemorySegmentDto::serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartObject();
  // PATH
  writer->String(m_shape.PATH);
  writer->String(m_path.string().c_str());
  // NAME
  writer->String(m_shape.NAME);
  writer->String(m_name.c_str());
  // SIZE
  writer->String(m_shape.SIZE);
  writer->String(std::to_string(m_size).c_str());
  // AUTO_EXTEND
  writer->String(m_shape.AUTO_EXTEND);
  writer->Bool(m_isAutoExtend);
  // AUTO_EXTEND_SIZE
  writer->String(m_shape.AUTO_EXTEND_SIZE);
  writer->String(std::to_string(m_autoExtendSize).c_str());
  // MAX_SIZE
  writer->String(m_shape.MAX_SIZE);
  writer->String(std::to_string(m_maxSize).c_str());
  // SANITY
  writer->String(m_shape.SANITY);
  writer->Bool(m_isSanity);
  // CURRENT_SIZE
  writer->String(m_shape.CURRENT_SIZE);
  writer->String(std::to_string(m_currentSize).c_str());
  // FREE_SIZE
  writer->String(m_shape.FREE_SIZE);
  writer->String(std::to_string(m_freeSize).c_str());
  // NAMED_OBJECTS
  writer->String(m_shape.NAMED_OBJECTS);
  writer->String(std::to_string(m_amountNamedObjects).c_str());
  // UNIQUE_OBJECTS
  writer->String(m_shape.UNIQUE_OBJECTS);
  writer->String(std::to_string(m_amountUniqueObjects).c_str());
  writer->EndObject();
}

bool SharedMemorySegmentDto::deserialize(const rapidjson::Value &obj)
{
  bool success = true;
  // PATH
  std::string pathString;
  if (json::getString(obj, m_shape.PATH, pathString)) {
    m_path = std::filesystem::path(pathString);
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.PATH);
  }
  // NAME
  if (!json::getString(obj, m_shape.NAME, m_name)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.NAME);
  }
  // SIZE
  std::string sizeString;
  if (json::getString(obj, m_shape.SIZE, sizeString)) {
    m_size = std::stoul(sizeString);
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.SIZE);
  }
  // AUTO_EXTEND
  if (!json::getBool(obj, m_shape.AUTO_EXTEND, m_isAutoExtend)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.AUTO_EXTEND);
  }
  // AUTO_EXTEND_SIZE
  std::string autoExtendSizeString;
  if (json::getString(obj, m_shape.AUTO_EXTEND_SIZE, autoExtendSizeString)) {
    m_autoExtendSize = std::stoul(autoExtendSizeString);
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.AUTO_EXTEND_SIZE);
  }
  // MAX_SIZE
  std::string maxSizeString;
  if (json::getString(obj, m_shape.MAX_SIZE, maxSizeString)) {
    m_maxSize = std::stoul(maxSizeString);
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.MAX_SIZE);
  }
  // SANITY
  if (!json::getBool(obj, m_shape.SANITY, m_isSanity)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.SANITY);
  }
  // CURRENT_SIZE
  std::string currentSizeString;
  if (json::getString(obj, m_shape.CURRENT_SIZE, currentSizeString)) {
    m_currentSize = std::stoul(currentSizeString);
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.CURRENT_SIZE);
  }
  // FREE_SIZE
  std::string freeSizeString;
  if (json::getString(obj, m_shape.FREE_SIZE, freeSizeString)) {
    m_freeSize = std::stoul(freeSizeString);
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.FREE_SIZE);
  }
  // NAMED_OBJECTS
  std::string namedObjectsString;
  if (json::getString(obj, m_shape.NAMED_OBJECTS, namedObjectsString)) {
    m_amountNamedObjects = std::stoul(namedObjectsString);
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.NAMED_OBJECTS);
  }
  // UNIQUE_OBJECTS
  std::string uniqueObjectsString;
  if (json::getString(obj, m_shape.UNIQUE_OBJECTS, uniqueObjectsString)) {
    m_amountUniqueObjects = std::stoul(uniqueObjectsString);
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.UNIQUE_OBJECTS);
  }

  return success;
}
}// namespace lintel
