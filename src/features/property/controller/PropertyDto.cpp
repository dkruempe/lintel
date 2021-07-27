#include "base_library/features/property/controller/PropertyDto.h"

#include "base_library/core/services/LoggerService.h"

PropertyDto::Shapes PropertyDto::shape{};
PropertyDto::PropertyDto(const std::shared_ptr<PropertyBase>& property)
    : m_name(property->getName()),
      m_instanceName(property->getInstanceName()),
      m_className(property->getClassName()),
      m_processName(property->getProcessName()),
      m_runtimeChange(property->isRuntimeChange()),
      m_description(property->getDescription()),
      m_repositoryType(property->getDataStorage().getType()),
      m_extraInformation(property->getDataStorage().getExtraInformation()),
      m_value(property->toString()),
      m_type(property->getType()) {}
void PropertyDto::serialize(
    rapidjson::Writer<rapidjson::StringBuffer>* writer) const {
  writer->StartObject();
  // NAME
  writer->String(shape.NAME.c_str());
  writer->String(m_name.c_str());
  // TYPE
  writer->String(shape.TYPE.c_str());
  writer->String(m_type.c_str());
  // VALUE
  writer->String(shape.VALUE.c_str());
  writer->String(m_value.c_str());
  // PROCESS
  writer->String(shape.PROCESS.c_str());
  writer->String(m_processName.c_str());
  // CLASS
  writer->String(shape.CLASS.c_str());
  writer->String(m_className.c_str());
  // INSTANCE
  writer->String(shape.INSTANCE.c_str());
  writer->String(m_instanceName.c_str());
  // DESCRIPTION
  writer->String(shape.DESCRIPTION.c_str());
  writer->String(m_description.c_str());
  // EXTRA_INFORMATION
  writer->String(shape.EXTRA_INFORMATION.c_str());
  writer->String(m_extraInformation.c_str());
  // REPOSITORY_TYPE
  writer->String(shape.REPOSITORY_TYPE.c_str());
  writer->String(m_repositoryType.toString().c_str());
  // RUNTIME_CHANGE
  writer->String(shape.RUNTIME_CHANGE.c_str());
  writer->Bool(m_runtimeChange);
  writer->EndObject();
}
bool PropertyDto::deserialize(const rapidjson::Value& obj) {
  bool success = true;
  // NAME
  if (obj.HasMember(shape.NAME.c_str())) {
    m_name = obj[shape.NAME.c_str()].GetString();
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", shape.NAME.c_str());
  }
  // TYPE
  if (obj.HasMember(shape.TYPE.c_str())) {
    m_type = obj[shape.TYPE.c_str()].GetString();
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", shape.TYPE.c_str());
  }
  // VALUE
  if (obj.HasMember(shape.VALUE.c_str())) {
    m_value = obj[shape.VALUE.c_str()].GetString();
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", shape.VALUE.c_str());
  }
  // PROCESS
  if (obj.HasMember(shape.PROCESS.c_str())) {
    m_processName = obj[shape.PROCESS.c_str()].GetString();
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", shape.PROCESS.c_str());
  }
  // CLASS
  if (obj.HasMember(shape.CLASS.c_str())) {
    m_className = obj[shape.CLASS.c_str()].GetString();
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", shape.CLASS.c_str());
  }
  // INSTANCE
  if (obj.HasMember(shape.INSTANCE.c_str())) {
    m_instanceName = obj[shape.INSTANCE.c_str()].GetString();
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", shape.INSTANCE.c_str());
  }
  // DESCRIPTION
  if (obj.HasMember(shape.DESCRIPTION.c_str())) {
    m_description = obj[shape.DESCRIPTION.c_str()].GetString();
  }
  // EXTRA INFORMATION
  if (obj.HasMember(shape.EXTRA_INFORMATION.c_str())) {
    m_extraInformation = obj[shape.EXTRA_INFORMATION.c_str()].GetString();
  }
  // TYPE
  if (obj.HasMember(shape.REPOSITORY_TYPE.c_str())) {
    m_repositoryType =
        PropertyRepositoryType(obj[shape.REPOSITORY_TYPE.c_str()].GetString());
  }
  // RUNTIME_CHANGE
  if (obj.HasMember(shape.RUNTIME_CHANGE.c_str())) {
    m_runtimeChange = obj[shape.RUNTIME_CHANGE.c_str()].GetBool();
  }
  return success;
}
const std::string& PropertyDto::getName() const { return m_name; }
const std::string& PropertyDto::getInstanceName() const {
  return m_instanceName;
}
const std::string& PropertyDto::getClassName() const { return m_className; }
const std::string& PropertyDto::getProcessName() const { return m_processName; }
bool PropertyDto::isRuntimeChange() const { return m_runtimeChange; }
const std::string& PropertyDto::getDescription() const { return m_description; }
const PropertyRepositoryType& PropertyDto::getRepositoryType() const {
  return m_repositoryType;
}
const std::string& PropertyDto::getExtraInformation() const {
  return m_extraInformation;
}
const std::string& PropertyDto::getValue() const { return m_value; }
const std::string& PropertyDto::getType() const { return m_type; }
