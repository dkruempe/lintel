#include <sstream>
#include <string>
#include <vector>

#include <catch2/catch_all.hpp>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "lintel/features/base/controller/GroupDto.h"
#include "lintel/features/base/models/Group.h"

namespace lintel {

TEST_CASE("Group: exposes its construction arguments")
{
  const Group group("admin", { Group("member", {}, true) }, true);
  REQUIRE(group.getGroupName() == "admin");
  REQUIRE(group.isVirtual());
  REQUIRE(group.getGroups().size() == 1);
  REQUIRE(group.getGroups()[0].getGroupName() == "member");
}

TEST_CASE("Group: streams a flat group")
{
  const Group group("admin", {}, false);
  std::ostringstream stream;
  stream << group;
  REQUIRE(stream.str() == "Group{group_name: admin, isVirtual:0}");
}

TEST_CASE("Group: streams nested groups")
{
  const Group group("admin", { Group("member", {}, true), Group("guest", {}, false) }, false);
  std::ostringstream stream;
  stream << group;
  const std::string text = stream.str();
  REQUIRE(text.find("m_groups: {") != std::string::npos);
  REQUIRE(text.find("group_name: member") != std::string::npos);
  REQUIRE(text.find("group_name: guest") != std::string::npos);
}

TEST_CASE("Group: compares by name, members and virtual flag")
{
  const Group admin("admin", {}, false);
  const Group sameAdmin("admin", {}, false);
  const Group virtualAdmin("admin", {}, true);
  const Group member("member", {}, false);
  const Group aaa("aaa", {}, false);

  REQUIRE(admin == sameAdmin);
  REQUIRE(admin != virtualAdmin);
  REQUIRE(admin != member);

  REQUIRE(admin <= sameAdmin);
  REQUIRE(admin >= sameAdmin);
  REQUIRE_FALSE(admin < sameAdmin);
  REQUIRE_FALSE(admin > sameAdmin);

  // ordering by name
  REQUIRE(aaa < admin);
  REQUIRE(admin > aaa);
  REQUIRE(aaa <= admin);
  REQUIRE(admin >= aaa);
  REQUIRE_FALSE(admin < aaa);
  REQUIRE_FALSE(aaa > admin);

  // same name, ordered by the member groups
  const Group withMembers("admin", { member }, false);
  REQUIRE(admin < withMembers);
  REQUIRE_FALSE(withMembers < admin);

  // same name and members, ordered by the virtual flag
  REQUIRE(admin < virtualAdmin);
  REQUIRE_FALSE(virtualAdmin < admin);
}

TEST_CASE("GroupDto: serializes a group including its sub-groups")
{
  const Group group("parent", { Group("child", {}, true) }, false);
  const GroupDto dto(group);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  dto.serialize(&writer);

  const std::string json = buffer.GetString();
  REQUIRE(json.find("\"group_name\":\"parent\"") != std::string::npos);
  REQUIRE(json.find("\"groups\"") != std::string::npos);
  REQUIRE(json.find("\"group_name\":\"child\"") != std::string::npos);
  REQUIRE(json.find("\"virtual\":false") != std::string::npos);
}

TEST_CASE("GroupDto: deserializes name, virtual flag and sub-groups")
{
  rapidjson::Document document;
  document.Parse(R"({"group_name":"g","virtual":true,"groups":[{"group_name":"c","virtual":false}]})");
  REQUIRE_FALSE(document.HasParseError());

  GroupDto dto;
  REQUIRE(dto.deserialize(document));
  REQUIRE(dto.getGroupName() == "g");
  REQUIRE(dto.isVirtual());
  REQUIRE(dto.getSubGroups().size() == 1);
  REQUIRE(dto.getSubGroups()[0].getGroupName() == "c");
  REQUIRE_FALSE(dto.getSubGroups()[0].isVirtual());
}

TEST_CASE("GroupDto: deserialization fails without the group name")
{
  rapidjson::Document document;
  document.Parse(R"({"virtual":true})");
  REQUIRE_FALSE(document.HasParseError());

  GroupDto dto;
  REQUIRE_FALSE(dto.deserialize(document));
}

TEST_CASE("GroupDto: deserialization fails without the virtual flag")
{
  rapidjson::Document document;
  document.Parse(R"({"group_name":"g"})");
  REQUIRE_FALSE(document.HasParseError());

  GroupDto dto;
  REQUIRE_FALSE(dto.deserialize(document));
}

TEST_CASE("GroupsDto: serializes and deserializes a list of group dtos")
{
  const GroupsDto groups({ Group("a", {}, false), Group("b", {}, true) });

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  groups.serialize(&writer);
  REQUIRE(std::string(buffer.GetString()).find("\"group_name\":\"a\"") != std::string::npos);

  rapidjson::Document document;
  document.Parse(R"([{"group_name":"a","virtual":false},{"group_name":"b","virtual":true}])");
  REQUIRE_FALSE(document.HasParseError());

  GroupsDto parsed(std::vector<GroupDto>{});
  REQUIRE(parsed.deserialize(document));
  REQUIRE(parsed.getGroups().size() == 2);
  REQUIRE(parsed.getGroups()[0].getGroupName() == "a");
  REQUIRE(parsed.getGroups()[1].isVirtual());

  // a non-array value is rejected
  rapidjson::Document object;
  object.Parse(R"({"group_name":"a"})");
  GroupsDto rejected(std::vector<GroupDto>{});
  REQUIRE_FALSE(rejected.deserialize(object));
}

TEST_CASE("GroupsDto: constructs from existing group dtos")
{
  const GroupsDto groups(std::vector<GroupDto>{ GroupDto(Group("only", {}, false)) });
  REQUIRE(groups.getGroups().size() == 1);
  REQUIRE(groups.getGroups()[0].getGroupName() == "only");
}

}// namespace lintel
