#include <string>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <catch2/catch_all.hpp>

#include "lintel/features/base/controller/UserLoginDto.h"
#include "lintel/features/base/services/IAuthService.h"

namespace lintel {
namespace {

  rapidjson::Document loginParse(const std::string &json)
  {
    rapidjson::Document document;
    document.Parse(json.c_str());
    return document;
  }

  std::string loginSerialize(const JsonSerializable &serializable)
  {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    serializable.serialize(&writer);
    return std::string(buffer.GetString());
  }

}// namespace

TEST_CASE("UserLoginDto: constructs from a UserLogin model and exposes its fields")
{
  UserLogin login;
  login.m_userName = "ada";
  login.m_password = "secret";

  const UserLoginDto dto(login);
  REQUIRE(dto.getUserName() == "ada");
  REQUIRE(dto.getPassword() == "secret");
}

TEST_CASE("UserLoginDto: constructs from explicit credentials")
{
  const UserLoginDto dto("grace", "hopper");
  REQUIRE(dto.getUserName() == "grace");
  REQUIRE(dto.getPassword() == "hopper");
}

TEST_CASE("UserLoginDto: serializes both fields")
{
  const UserLoginDto dto("ada", "secret");
  const std::string json = loginSerialize(dto);
  REQUIRE(json.find("\"user_name\":\"ada\"") != std::string::npos);
  REQUIRE(json.find("\"password\":\"secret\"") != std::string::npos);
}

TEST_CASE("UserLoginDto: deserializes a full object")
{
  UserLoginDto dto;
  REQUIRE(dto.deserialize(loginParse(R"({"user_name":"ada","password":"secret"})")));
  REQUIRE(dto.getUserName() == "ada");
  REQUIRE(dto.getPassword() == "secret");
}

TEST_CASE("UserLoginDto: reports missing fields")
{
  SECTION("user name missing")
  {
    UserLoginDto dto;
    REQUIRE_FALSE(dto.deserialize(loginParse(R"({"password":"secret"})")));
    REQUIRE(dto.getPassword() == "secret");
  }
  SECTION("password missing")
  {
    UserLoginDto dto;
    REQUIRE_FALSE(dto.deserialize(loginParse(R"({"user_name":"ada"})")));
    REQUIRE(dto.getUserName() == "ada");
  }
  SECTION("both missing")
  {
    UserLoginDto dto;
    REQUIRE_FALSE(dto.deserialize(loginParse(R"({})")));
  }
}

}// namespace lintel
