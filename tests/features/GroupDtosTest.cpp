#include <catch2/catch_all.hpp>

#include <string>
#include <vector>

#include <rapidjson/document.h>

#include "lintel/features/base/controller/ProcessGroupDto.h"
#include "lintel/features/base/controller/UserGroupDto.h"

namespace lintel {

TEST_CASE("ProcessGroupDto: deserializes id, name and processes")
{
  rapidjson::Document document;
  document.Parse(R"({"id":"group-1","name":"Group One","processes":[]})");
  REQUIRE_FALSE(document.HasParseError());

  ProcessGroupDto dto;
  REQUIRE(dto.deserialize(document));
  REQUIRE(dto.getId() == "group-1");
  REQUIRE(dto.getName() == "Group One");
}

TEST_CASE("ProcessGroupDto: deserialization reports every missing field")
{
  SECTION("missing id")
  {
    rapidjson::Document document;
    document.Parse(R"({"name":"Group One","processes":[]})");
    ProcessGroupDto dto;
    REQUIRE_FALSE(dto.deserialize(document));
  }

  SECTION("missing name")
  {
    rapidjson::Document document;
    document.Parse(R"({"id":"group-1","processes":[]})");
    ProcessGroupDto dto;
    REQUIRE_FALSE(dto.deserialize(document));
  }

  SECTION("missing processes")
  {
    rapidjson::Document document;
    document.Parse(R"({"id":"group-1","name":"Group One"})");
    ProcessGroupDto dto;
    REQUIRE_FALSE(dto.deserialize(document));
  }

  SECTION("processes is not an array")
  {
    rapidjson::Document document;
    document.Parse(R"({"id":"group-1","name":"Group One","processes":"nope"})");
    ProcessGroupDto dto;
    REQUIRE_FALSE(dto.deserialize(document));
  }
}

TEST_CASE("UserGroupDto: deserializes user name and group changes")
{
  rapidjson::Document document;
  document.Parse(R"({"user_name":"alice","groups_add":["a","b"],"groups_remove":["c"]})");
  REQUIRE_FALSE(document.HasParseError());

  UserGroupDto dto;
  REQUIRE(dto.deserialize(document));
  REQUIRE(dto.getUserName() == "alice");
  REQUIRE(dto.getGroupAdds() == std::set<std::string>{ "a", "b" });
  REQUIRE(dto.getGroupRemoves() == std::set<std::string>{ "c" });
}

TEST_CASE("UserGroupDto: deserialization reports every missing field")
{
  SECTION("missing user name")
  {
    rapidjson::Document document;
    document.Parse(R"({"groups_add":[],"groups_remove":[]})");
    UserGroupDto dto;
    REQUIRE_FALSE(dto.deserialize(document));
  }

  SECTION("missing groups_add")
  {
    rapidjson::Document document;
    document.Parse(R"({"user_name":"alice","groups_remove":[]})");
    UserGroupDto dto;
    REQUIRE_FALSE(dto.deserialize(document));
  }

  SECTION("missing groups_remove")
  {
    rapidjson::Document document;
    document.Parse(R"({"user_name":"alice","groups_add":[]})");
    UserGroupDto dto;
    REQUIRE_FALSE(dto.deserialize(document));
  }
}

}// namespace lintel
