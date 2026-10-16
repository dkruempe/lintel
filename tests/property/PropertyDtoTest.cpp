#include <catch2/catch_all.hpp>

#include <rapidjson/document.h>

#include <memory>
#include <string>
#include <vector>

#include "lintel/core/models/Property.h"
#include "lintel/features/property/controller/PropertiesDto.h"
#include "lintel/features/property/controller/PropertyDto.h"
#include "lintel/features/property/controller/PropertyValueDto.h"
#include "lintel/features/property/factories/PropertyFactory.h"

namespace lintel {

TEST_CASE("PropertyDto: reflects the property metadata and round-trips through JSON")
{
  auto property =
    PropertyFactory::Create("testProperty", "testInstance", "testClass", "testProcess", "int32_t", "4711", "extra", true);
  REQUIRE(property != nullptr);

  PropertyDto dto(property);
  REQUIRE(dto.getName() == "testProperty");
  REQUIRE(dto.getInstanceName() == "testInstance");
  REQUIRE(dto.getClassName() == "testClass");
  REQUIRE(dto.getProcessName() == "testProcess");
  REQUIRE(dto.getType() == "int32_t");
  REQUIRE(dto.getValue() == "4711");
  REQUIRE(dto.getDescription() == "extra");
  REQUIRE(dto.isRuntimeChange());

  const std::string serialized = dto.JsonSerializable::serialize();
  REQUIRE(serialized.find("testProperty") != std::string::npos);

  PropertyDto parsed;
  parsed.JsonSerializable::deserialize(serialized);
  REQUIRE(parsed.getName() == "testProperty");
  REQUIRE(parsed.getInstanceName() == "testInstance");
  REQUIRE(parsed.getClassName() == "testClass");
  REQUIRE(parsed.getProcessName() == "testProcess");
  REQUIRE(parsed.getType() == "int32_t");
  REQUIRE(parsed.getValue() == "4711");
  REQUIRE(parsed.getDescription() == "extra");
  REQUIRE(parsed.isRuntimeChange());
  REQUIRE(static_cast<PropertyRepositoryType::Value>(parsed.getRepositoryType())
          == static_cast<PropertyRepositoryType::Value>(dto.getRepositoryType()));
}

TEST_CASE("PropertyDto: deserialize reports failure when required fields are missing")
{
  rapidjson::Document document;
  document.Parse(R"({})");
  REQUIRE(document.IsObject());

  PropertyDto dto;
  REQUIRE_FALSE(dto.deserialize(document));
}

TEST_CASE("PropertyDto: deserialize accepts a full object")
{
  rapidjson::Document document;
  document.Parse(R"({
    "name":"n","type":"int32_t","value":"1","process":"p",
    "class":"c","instance":"i","description":"d",
    "extra_information":"e","repository_type":"UNDEFINED","runtime_change":true
  })");
  REQUIRE(document.IsObject());

  PropertyDto dto;
  REQUIRE(dto.deserialize(document));
  REQUIRE(dto.getName() == "n");
  REQUIRE(dto.getType() == "int32_t");
  REQUIRE(dto.isRuntimeChange());
}

TEST_CASE("PropertiesDto: builds, serializes and deserializes a list of properties")
{
  std::vector<std::shared_ptr<PropertyBase>> properties{
    PropertyFactory::Create("a", "i", "c", "p", "int32_t", "1", "", true),
    PropertyFactory::Create("b", "i", "c", "p", "std::string", "hello", "", false)
  };

  PropertiesDto dto(properties);
  REQUIRE(dto.getProperties().size() == 2);
  REQUIRE(dto.getProperties()[0].getName() == "a");
  REQUIRE(dto.getProperties()[1].getName() == "b");

  const std::string serialized = dto.JsonSerializable::serialize();
  REQUIRE(serialized.front() == '[');

  PropertiesDto parsed;
  parsed.deserialize(serialized);
  REQUIRE(parsed.getProperties().size() == 2);
  REQUIRE(parsed.getProperties()[0].getName() == "a");
  REQUIRE(parsed.getProperties()[1].getName() == "b");
}

TEST_CASE("PropertiesDto: deserialize ignores non-array and non-object entries")
{
  PropertiesDto dto;
  dto.deserialize(R"("not an array")");
  REQUIRE(dto.getProperties().empty());

  PropertiesDto mixed;
  mixed.deserialize(R"([[],{"name":"a","type":"t","value":"v","process":"p","class":"c","instance":"i"}])");
  REQUIRE(mixed.getProperties().size() == 1);
  REQUIRE(mixed.getProperties()[0].getName() == "a");
}

TEST_CASE("PropertyValueDto: round-trips a single value")
{
  PropertyValueDto dto("the value");
  REQUIRE(dto.getValue() == "the value");

  const std::string serialized = dto.JsonSerializable::serialize();
  REQUIRE(serialized.find("the value") != std::string::npos);

  PropertyValueDto parsed;
  parsed.JsonSerializable::deserialize(serialized);
  REQUIRE(parsed.getValue() == "the value");
}

TEST_CASE("PropertyValueDto: deserialize reports failure when the value field is missing")
{
  rapidjson::Document document;
  document.Parse(R"({})");
  REQUIRE(document.IsObject());

  PropertyValueDto dto;
  REQUIRE_FALSE(dto.deserialize(document));
}

}// namespace lintel
