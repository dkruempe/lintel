#include "lintel/features/property/controller/PropertyDto.h"

#include "lintel/core/services/LoggerService.h"
#include "lintel/core/utils/JsonUtils.h"

namespace lintel {
PropertyDto::Shapes PropertyDto::m_shape{};

PropertyDto::PropertyDto(const std::shared_ptr<PropertyBase> &property)
  : m_name(property->getName()), m_instanceName(property->getInstanceName()), m_className(property->getClassName()),
    m_processName(property->getProcessName()), m_runtimeChange(property->isRuntimeChange()),
    m_description(property->getDescription()), m_repositoryType(property->getDataStorage().getType()),
    m_extraInformation(property->getDataStorage().getExtraInformation()), m_value(property->toString()),
    m_type(property->getType())
{}

void PropertyDto::serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartObject();
  // NAME
  writer->String(m_shape.NAME.c_str());
  writer->String(m_name.c_str());
  // TYPE
  writer->String(m_shape.TYPE.c_str());
  writer->String(m_type.c_str());
  // VALUE
  writer->String(m_shape.VALUE.c_str());
  writer->String(m_value.c_str());
  // PROCESS
  writer->String(m_shape.PROCESS.c_str());
  writer->String(m_processName.c_str());
  // CLASS
  writer->String(m_shape.CLASS.c_str());
  writer->String(m_className.c_str());
  // INSTANCE
  writer->String(m_shape.INSTANCE.c_str());
  writer->String(m_instanceName.c_str());
  // DESCRIPTION
  writer->String(m_shape.DESCRIPTION.c_str());
  writer->String(m_description.c_str());
  // EXTRA_INFORMATION
  writer->String(m_shape.EXTRA_INFORMATION.c_str());
  writer->String(m_extraInformation.c_str());
  // REPOSITORY_TYPE
  writer->String(m_shape.REPOSITORY_TYPE.c_str());
  writer->String(m_repositoryType.toString().c_str());
  // RUNTIME_CHANGE
  writer->String(m_shape.RUNTIME_CHANGE.c_str());
  writer->Bool(m_runtimeChange);
  writer->EndObject();
}

bool PropertyDto::deserialize(const rapidjson::Value &obj)
{
  bool success = true;
  // NAME
  if (!json::getString(obj, m_shape.NAME.c_str(), m_name)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.NAME.c_str());
  }
  // TYPE
  if (!json::getString(obj, m_shape.TYPE.c_str(), m_type)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.TYPE.c_str());
  }
  // VALUE
  if (!json::getString(obj, m_shape.VALUE.c_str(), m_value)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.VALUE.c_str());
  }
  // PROCESS
  if (!json::getString(obj, m_shape.PROCESS.c_str(), m_processName)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.PROCESS.c_str());
  }
  // CLASS
  if (!json::getString(obj, m_shape.CLASS.c_str(), m_className)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.CLASS.c_str());
  }
  // INSTANCE
  if (!json::getString(obj, m_shape.INSTANCE.c_str(), m_instanceName)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.INSTANCE.c_str());
  }
  // DESCRIPTION
  json::getString(obj, m_shape.DESCRIPTION.c_str(), m_description);
  // EXTRA INFORMATION
  json::getString(obj, m_shape.EXTRA_INFORMATION.c_str(), m_extraInformation);
  // TYPE
  std::string repositoryType;
  if (json::getString(obj, m_shape.REPOSITORY_TYPE.c_str(), repositoryType)) {
    m_repositoryType = PropertyRepositoryType(repositoryType);
  }
  // RUNTIME_CHANGE
  json::getBool(obj, m_shape.RUNTIME_CHANGE.c_str(), m_runtimeChange);
  return success;
}

const std::string &PropertyDto::getName() const { return m_name; }

const std::string &PropertyDto::getInstanceName() const { return m_instanceName; }

const std::string &PropertyDto::getClassName() const { return m_className; }

const std::string &PropertyDto::getProcessName() const { return m_processName; }

bool PropertyDto::isRuntimeChange() const { return m_runtimeChange; }

const std::string &PropertyDto::getDescription() const { return m_description; }

const PropertyRepositoryType &PropertyDto::getRepositoryType() const { return m_repositoryType; }

const std::string &PropertyDto::getExtraInformation() const { return m_extraInformation; }

const std::string &PropertyDto::getValue() const { return m_value; }

const std::string &PropertyDto::getType() const { return m_type; }
}// namespace lintel
