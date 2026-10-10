#include <lintel/core/models/JsonSerializable.h>
#include <lintel/features/base/controller/GroupDto.h>
#include <lintel/features/http/service/HttpBadRequestException.h>

#include <catch2/catch_all.hpp>

#include <string>
#include <vector>

// GroupsDto is used as the concrete DTO because it is the one production DTO that reads a JSON
// array instead of a JSON object: deserialize() returns early for a non-array value, so feeding it
// the documents below exercises JsonSerializable::deserialize(const std::string&) itself without
// the DTO running into member lookups on a scalar.

// JsonSerializable::deserialize(const std::string&) rejects everything that is neither an object
// nor an array. Those requests reach a controller with a body the DTO can never read, so the
// rejection has to happen before the value is dispatched - HttpBadRequestException is what the
// HTTP layer turns into a 400 instead of a 500.
TEST_CASE("JsonSerializable::deserialize rejects bodies that are neither object nor array")
{
  const std::vector<std::string> rejectedBodies = {
    "123", "\"str\"", "null", "true", "false", "{not json", "", "   ", "[1,", "{\"a\":}"
  };

  for (const std::string &body : rejectedBodies) {
    CAPTURE(body);
    GroupsDto groups;
    JsonSerializable &serializable = groups;
    REQUIRE_THROWS_AS(serializable.deserialize(body), HttpBadRequestException);
  }
}

// An array is a valid document and must be forwarded to the DTO. Treating it as a bad request
// would break every collection endpoint, whose bodies are arrays by construction.
TEST_CASE("JsonSerializable::deserialize accepts a JSON array")
{
  GroupsDto groups;
  JsonSerializable &serializable = groups;
  REQUIRE_NOTHROW(serializable.deserialize(std::string("[]")));
  REQUIRE(groups.getGroups().empty());

  // Counterpart of the empty array: a populated array really reaches the DTO.
  REQUIRE_NOTHROW(serializable.deserialize(std::string(R"([{"group_name":"admin","virtual":true}])")));
  REQUIRE(groups.getGroups().size() == 1);
  REQUIRE(groups.getGroups()[0].getGroupName() == "admin");
  REQUIRE(groups.getGroups()[0].isVirtual());
}

// An object is accepted as well and simply leaves the DTO without members to read here.
TEST_CASE("JsonSerializable::deserialize accepts a JSON object")
{
  GroupsDto groups;
  JsonSerializable &serializable = groups;
  REQUIRE_NOTHROW(serializable.deserialize(std::string("{}")));
  REQUIRE(groups.getGroups().empty());
}