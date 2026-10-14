#include "lintel/features/base/controller/UserTokenDto.h"

#include "lintel/core/services/LoggerService.h"
#include "lintel/core/utils/JsonUtils.h"

namespace lintel {
UserTokenDto::Shapes UserTokenDto::shape{};

UserTokenDto::UserTokenDto(std::string id) : m_id(std::move(id)) {}

void UserTokenDto::serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartObject();
  // ID
  writer->String(shape.ID);
  writer->String(m_id.c_str());
  writer->EndObject();
}

bool UserTokenDto::deserialize(const rapidjson::Value &obj)
{
  bool success = true;
  if (!json::getString(obj, shape.ID, m_id)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", shape.ID);
  }
  return success;
}

const std::string &UserTokenDto::getId() const { return m_id; }
}// namespace lintel
