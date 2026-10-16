#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <catch2/catch_all.hpp>

#include "lintel/features/base/controller/UserDto.h"
#include "lintel/features/base/models/Group.h"
#include "lintel/features/base/models/Page.h"
#include "lintel/features/base/models/User.h"

namespace lintel {
namespace {

  User makeUser(std::vector<Group> groups = {})
  {
    return User("Ada", "Lovelace", User::Sex::Female, "ada@example.com", "ada", "secret-hash", std::move(groups));
  }

  rapidjson::Document parse(const std::string &json)
  {
    rapidjson::Document document;
    document.Parse(json.c_str());
    return document;
  }

  std::string serialize(const JsonSerializable &serializable)
  {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    serializable.serialize(&writer);
    return std::string(buffer.GetString());
  }

}// namespace

TEST_CASE("UserDto serializes all fields including password when set")
{
  UserDto dto(makeUser({ Group("admin", {}, false) }), "id-1");
  dto.setPassword("plain-secret");

  REQUIRE(dto.getPassword().value() == "plain-secret");
  REQUIRE(dto.getId() == "id-1");
  REQUIRE(dto.getSex() == User::Sex::Female);
  REQUIRE(dto.getFirstName() == "Ada");
  REQUIRE(dto.getLastName() == "Lovelace");
  REQUIRE(dto.getEMail() == "ada@example.com");
  REQUIRE(dto.getUserName() == "ada");
  REQUIRE_FALSE(dto.getGroups().empty());

  const std::string json = serialize(dto);
  REQUIRE(json.find("\"password\":\"plain-secret\"") != std::string::npos);
  REQUIRE(json.find("\"id\":\"id-1\"") != std::string::npos);
  REQUIRE(json.find("\"sex\":\"female\"") != std::string::npos);
  REQUIRE(json.find("\"groups\"") != std::string::npos);
}

TEST_CASE("UserDto omits the id and password when not set")
{
  UserDto dto(makeUser());
  REQUIRE(dto.getId().empty());
  REQUIRE_FALSE(dto.getPassword().has_value());
  REQUIRE(dto.getGroups().empty());// default constructed, groups is null

  const std::string json = serialize(dto);
  REQUIRE(json.find("\"password\"") == std::string::npos);
  REQUIRE(json.find("\"id\"") == std::string::npos);
}

TEST_CASE("UserDto deserializes a full object")
{
  UserDto dto;
  const auto document = parse(R"({
    "first_name":"Grace","last_name":"Hopper","email":"grace@example.com",
    "sex":"female","user_name":"grace","created_timestamp":"2024-01-02T03:04:05.000000",
    "id":"id-9","password":"pw","groups":[{"group_name":"admin","virtual":false}]
  })");

  REQUIRE(dto.deserialize(document));
  REQUIRE(dto.getFirstName() == "Grace");
  REQUIRE(dto.getLastName() == "Hopper");
  REQUIRE(dto.getEMail() == "grace@example.com");
  REQUIRE(dto.getUserName() == "grace");
  REQUIRE(dto.getSex() == User::Sex::Female);
  REQUIRE(dto.getId() == "id-9");
  REQUIRE(dto.getPassword().value() == "pw");
  REQUIRE_FALSE(dto.getGroups().empty());
}

TEST_CASE("UserDto reports incomplete objects")
{
  SECTION("first name missing")
  {
    UserDto dto;
    REQUIRE_FALSE(dto.deserialize(parse(
      R"({"last_name":"x","email":"e","user_name":"u","sex":"male","created_timestamp":"2024-01-02T03:04:05.000000"})")));
  }
  SECTION("last name missing")
  {
    UserDto dto;
    REQUIRE_FALSE(dto.deserialize(parse(
      R"({"first_name":"x","email":"e","user_name":"u","sex":"male","created_timestamp":"2024-01-02T03:04:05.000000"})")));
  }
  SECTION("email missing")
  {
    UserDto dto;
    REQUIRE_FALSE(dto.deserialize(parse(
      R"({"first_name":"x","last_name":"y","user_name":"u","sex":"male","created_timestamp":"2024-01-02T03:04:05.000000"})")));
  }
  SECTION("user name missing")
  {
    UserDto dto;
    REQUIRE_FALSE(dto.deserialize(parse(
      R"({"first_name":"x","last_name":"y","email":"e","sex":"male","created_timestamp":"2024-01-02T03:04:05.000000"})")));
  }
  SECTION("timestamp missing")
  {
    UserDto dto;
    REQUIRE_FALSE(
      dto.deserialize(parse(R"({"first_name":"x","last_name":"y","email":"e","user_name":"u","sex":"male"})")));
  }
}

TEST_CASE("UserDto rejects an unknown sex value")
{
  UserDto dto;
  const auto document = parse(R"({
    "first_name":"x","last_name":"y","email":"e","user_name":"u","sex":"robot",
    "created_timestamp":"2024-01-02T03:04:05.000000"
  })");
  REQUIRE_FALSE(dto.deserialize(document));
}

TEST_CASE("UserDto without groups yields an empty group list")
{
  UserDto dto;
  const auto document = parse(R"({
    "first_name":"x","last_name":"y","email":"e","user_name":"u","sex":"male",
    "created_timestamp":"2024-01-02T03:04:05.000000"
  })");
  REQUIRE(dto.deserialize(document));
  REQUIRE(dto.getGroups().empty());
}

TEST_CASE("UsersDto serializes and deserializes a collection")
{
  UsersDto users({ makeUser(), makeUser() });
  REQUIRE(users.getUsers().size() == 2);

  const std::string json = serialize(users);
  REQUIRE(json.front() == '[');

  UsersDto restored;
  REQUIRE(restored.deserialize(parse(json)));
  REQUIRE(restored.getUsers().size() == 2);

  // non-array and non-object entries are tolerated
  UsersDto wrong;
  REQUIRE_FALSE(wrong.deserialize(parse(R"({"not":"an array"})")));
}

TEST_CASE("UsersPageDto serializes and deserializes a page")
{
  const Page<User> page({ makeUser(), makeUser() }, true, std::string("cursor-1"));
  UsersPageDto dto(page);

  REQUIRE(dto.hasMore());
  REQUIRE(dto.getNextAfter().value() == "cursor-1");
  REQUIRE(dto.getUsers().size() == 2);

  const std::string json = serialize(dto);
  REQUIRE(json.find("\"has_more\":true") != std::string::npos);
  REQUIRE(json.find("\"next_after\":\"cursor-1\"") != std::string::npos);

  UsersPageDto restored;
  REQUIRE(restored.deserialize(parse(json)));
  REQUIRE(restored.hasMore());
  REQUIRE(restored.getNextAfter().value() == "cursor-1");
  REQUIRE(restored.getUsers().size() == 2);
}

TEST_CASE("UsersPageDto reports missing fields")
{
  SECTION("items missing")
  {
    UsersPageDto dto;
    REQUIRE_FALSE(dto.deserialize(parse(R"({"has_more":false})")));
  }
  SECTION("has_more missing")
  {
    UsersPageDto dto;
    REQUIRE_FALSE(dto.deserialize(parse(R"({"items":[]})")));
  }
  SECTION("not an object")
  {
    UsersPageDto dto;
    REQUIRE_FALSE(dto.deserialize(parse(R"([1,2,3])")));
  }
}

TEST_CASE("UsersPageDto without next_after leaves it empty")
{
  const Page<User> page({}, false, std::nullopt);
  UsersPageDto dto(page);
  REQUIRE_FALSE(dto.getNextAfter().has_value());

  UsersPageDto restored;
  REQUIRE(restored.deserialize(parse(serialize(dto))));
  REQUIRE_FALSE(restored.getNextAfter().has_value());
}

}// namespace lintel
