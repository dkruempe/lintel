#ifndef CPP_BASE_LIBRARY_PROPERTYVALUEDTO_H
#define CPP_BASE_LIBRARY_PROPERTYVALUEDTO_H

#include <string>

#include "base_library/core/models/JsonSerializable.h"

class PropertyValueDto : public JsonSerializable {
 private:
  std::string m_value;

  static struct Shapes { const std::string VALUE = "value"; } m_shapes;

 public:
  explicit PropertyValueDto(std::string value);

  PropertyValueDto() = default;

  [[nodiscard]] const std::string &getValue() const;

  void serialize(
      rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

  bool deserialize(const rapidjson::Value &obj) override;
};

#endif  // CPP_BASE_LIBRARY_PROPERTYVALUEDTO_H
