#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>

#include <memory>
#include <sstream>

#include "../mocks/MockUserApi.h"
#include "lintel/features/cli/components/UserManagementCliComponent.h"

namespace lintel {
using namespace trompeloeil;

TEST_CASE("UserManagementCliComponent: show_groups calls allOf()")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, allOf()).TIMES(1).LR_RETURN(std::vector<GroupDto>{});

  UserDto userDto;
  std::ostringstream oss;
  auto oldBuf = std::cout.rdbuf(oss.rdbuf());

  component.onCommand(userDto, "show_groups", {});

  std::cout.rdbuf(oldBuf);
}

TEST_CASE("UserManagementCliComponent: show_groups with -g calls allOf(name)")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, allOf(std::string{ "mygroup" })).TIMES(1).LR_RETURN(std::vector<GroupDto>{});

  UserDto userDto;
  std::ostringstream oss;
  auto oldBuf = std::cout.rdbuf(oss.rdbuf());

  component.onCommand(userDto, "show_groups", { "-g", "mygroup" });

  std::cout.rdbuf(oldBuf);
}

TEST_CASE("UserManagementCliComponent: show_groups with -v calls allOf(bool)")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, allOf(true)).TIMES(1).LR_RETURN(std::vector<GroupDto>{});

  UserDto userDto;
  std::ostringstream oss;
  auto oldBuf = std::cout.rdbuf(oss.rdbuf());

  component.onCommand(userDto, "show_groups", { "-v" });

  std::cout.rdbuf(oldBuf);
}

TEST_CASE("UserManagementCliComponent: show_groups with -g and -v calls allOf(name, bool)")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, allOf(std::string{ "mygroup" }, true)).TIMES(1).LR_RETURN(std::vector<GroupDto>{});

  UserDto userDto;
  std::ostringstream oss;
  auto oldBuf = std::cout.rdbuf(oss.rdbuf());

  component.onCommand(userDto, "show_groups", { "-g", "mygroup", "-v" });

  std::cout.rdbuf(oldBuf);
}

TEST_CASE("UserManagementCliComponent: show_users calls allUsersOf()")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, allUsersOf()).TIMES(1).LR_RETURN(std::vector<UserDto>{});

  UserDto userDto;
  std::ostringstream oss;
  auto oldBuf = std::cout.rdbuf(oss.rdbuf());

  component.onCommand(userDto, "show_users", {});

  std::cout.rdbuf(oldBuf);
}

TEST_CASE("UserManagementCliComponent: show_users with -u calls allUsersOf(name)")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, allUsersOf(std::string{ "jdoe" })).TIMES(1).LR_RETURN(std::vector<UserDto>{});

  UserDto userDto;
  std::ostringstream oss;
  auto oldBuf = std::cout.rdbuf(oss.rdbuf());

  component.onCommand(userDto, "show_users", { "-u", "jdoe" });

  std::cout.rdbuf(oldBuf);
}

TEST_CASE("UserManagementCliComponent: user_delete calls deleteOf")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, deleteOf(std::vector<std::string>{ "jdoe" })).TIMES(1);

  UserDto userDto;
  component.onCommand(userDto, "user_delete", { "-u", "jdoe" });
}

TEST_CASE("UserManagementCliComponent: user_delete without -u prints error")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  FORBID_CALL(*mockUserApi, deleteOf(ANY(std::vector<std::string>)));

  UserDto userDto;
  std::ostringstream oss;
  auto oldBuf = std::cout.rdbuf(oss.rdbuf());

  component.onCommand(userDto, "user_delete", {});

  std::cout.rdbuf(oldBuf);
  REQUIRE(oss.str().find("ERROR") != std::string::npos);
}

TEST_CASE("UserManagementCliComponent: user_add with missing args prints error")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  FORBID_CALL(*mockUserApi, createOf(ANY(UserDto)));

  UserDto userDto;
  std::ostringstream oss;
  auto oldBuf = std::cerr.rdbuf(oss.rdbuf());

  component.onCommand(userDto, "user_add", { "-u", "jdoe" });

  std::cerr.rdbuf(oldBuf);
  REQUIRE(oss.str().find("ERROR") != std::string::npos);
}

TEST_CASE("UserManagementCliComponent: user_update with missing -u prints error")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  FORBID_CALL(*mockUserApi, updateOf(ANY(std::string), ANY(std::set<std::string>), ANY(std::set<std::string>)));

  UserDto userDto;
  std::ostringstream oss;
  auto oldBuf = std::cerr.rdbuf(oss.rdbuf());

  component.onCommand(userDto, "user_update", {});

  std::cerr.rdbuf(oldBuf);
  REQUIRE(oss.str().find("ERROR") != std::string::npos);
}

TEST_CASE("UserManagementCliComponent: user_update with -u calls updateOf")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, updateOf(std::string{ "jdoe" }, std::set<std::string>{}, std::set<std::string>{}))
    .TIMES(1);

  UserDto userDto;
  component.onCommand(userDto, "user_update", { "-u", "jdoe" });
}

TEST_CASE("UserManagementCliComponent: user_update with -u -g calls updateOf with group")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, updateOf(std::string{ "jdoe" }, std::set<std::string>{ "admin" }, std::set<std::string>{}))
    .TIMES(1);

  UserDto userDto;
  component.onCommand(userDto, "user_update", { "-u", "jdoe", "-g", "admin" });
}

TEST_CASE("UserManagementCliComponent: user_change_password with -u -n calls changePasswordOf")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, changePasswordOf(ANY(UserPasswordChangeDto)))
    .WITH(_1.getUserName() == "jdoe" && _1.getNewPassword() == "bmV3cGFzcw==" && _1.getOldPassword().empty())
    .TIMES(1)
    .LR_RETURN(true);

  UserDto userDto;
  component.onCommand(userDto, "user_change_password", { "-u", "jdoe", "-n", "newpass" });
}

TEST_CASE("UserManagementCliComponent: user_change_password without -n prints error")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  FORBID_CALL(*mockUserApi, changePasswordOf(ANY(UserPasswordChangeDto)));

  UserDto userDto;
  std::ostringstream oss;
  auto oldBuf = std::cerr.rdbuf(oss.rdbuf());

  component.onCommand(userDto, "user_change_password", { "-u", "jdoe" });

  std::cerr.rdbuf(oldBuf);
  REQUIRE(oss.str().find("ERROR") != std::string::npos);
}

TEST_CASE("UserManagementCliComponent: user_sessions calls sessionsOf and prints")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  UserSessionDto session;
  REQUIRE_CALL(*mockUserApi, sessionsOf()).TIMES(1).LR_RETURN(std::vector<UserSessionDto>{ session });

  UserDto userDto;
  std::ostringstream oss;
  auto oldBuf = std::cout.rdbuf(oss.rdbuf());

  component.onCommand(userDto, "user_sessions", {});

  std::cout.rdbuf(oldBuf);
  REQUIRE(oss.str().find("Last Access") != std::string::npos);
}

TEST_CASE("UserManagementCliComponent: user_session_revoke calls revokeSessionOf")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, revokeSessionOf(std::string{ "abc123" })).TIMES(1).LR_RETURN(true);

  UserDto userDto;
  component.onCommand(userDto, "user_session_revoke", { "-s", "abc123" });
}

TEST_CASE("UserManagementCliComponent: user_session_revoke without -s prints error")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  FORBID_CALL(*mockUserApi, revokeSessionOf(ANY(std::string)));

  UserDto userDto;
  std::ostringstream oss;
  auto oldBuf = std::cerr.rdbuf(oss.rdbuf());

  component.onCommand(userDto, "user_session_revoke", {});

  std::cerr.rdbuf(oldBuf);
  REQUIRE(oss.str().find("ERROR") != std::string::npos);
}

TEST_CASE("UserManagementCliComponent: user_add with all args calls createOf")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, createOf(ANY(UserDto))).TIMES(1);

  UserDto userDto;
  component.onCommand(
    userDto, "user_add", { "-f", "John", "-l", "Doe", "-e", "john@doe.test", "-p", "secret", "-u", "jdoe" });
}

TEST_CASE("UserManagementCliComponent: show_groups with data prints the groups")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  GroupDto group(Group("admin", { Group("sub", {}, false), Group("ops", {}, true) }, false));
  REQUIRE_CALL(*mockUserApi, allOf()).TIMES(1).LR_RETURN(std::vector<GroupDto>{ group });

  UserDto userDto;
  std::ostringstream oss;
  auto oldBuf = std::cout.rdbuf(oss.rdbuf());

  component.onCommand(userDto, "show_groups", {});

  std::cout.rdbuf(oldBuf);
  const std::string output = oss.str();
  REQUIRE(output.find("admin") != std::string::npos);
  REQUIRE(output.find("sub;ops") != std::string::npos);
}

TEST_CASE("UserManagementCliComponent: show_users with data prints the users")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  User user("John", "Doe", User::Sex::Male, "john@doe.test", "jdoe", "secret", { Group("admin", {}, false) });
  UserDto userDto(user);
  REQUIRE_CALL(*mockUserApi, allUsersOf()).TIMES(1).LR_RETURN(std::vector<UserDto>{ userDto });

  std::ostringstream oss;
  auto oldBuf = std::cout.rdbuf(oss.rdbuf());

  component.onCommand(UserDto{}, "show_users", {});

  std::cout.rdbuf(oldBuf);
  REQUIRE(oss.str().find("jdoe") != std::string::npos);
}

TEST_CASE("UserManagementCliComponent: lifecycle and metadata accessors")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  std::ostringstream oss;
  auto oldOut = std::cout.rdbuf(oss.rdbuf());

  component.onHelp();
  component.onShowMenu();
  component.printCommandList({ "Test" });
  const auto commands = component.allCommandsOf();

  std::cout.rdbuf(oldOut);

  REQUIRE_FALSE(component.onMenu("anything"));
  REQUIRE(component.onExit());
  REQUIRE_FALSE(commands.empty());
  REQUIRE(oss.str().find("No submenu available!") != std::string::npos);
}

TEST_CASE("UserManagementCliComponent: a failed password change prints an error")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, changePasswordOf(ANY(UserPasswordChangeDto))).TIMES(1).LR_RETURN(false);

  std::ostringstream oss;
  auto oldBuf = std::cerr.rdbuf(oss.rdbuf());

  component.onCommand(UserDto{}, "user_change_password", { "-u", "jdoe", "-n", "newpass" });

  std::cerr.rdbuf(oldBuf);
  REQUIRE(oss.str().find("ERROR") != std::string::npos);
}

TEST_CASE("UserManagementCliComponent: a failed session revoke prints an error")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, revokeSessionOf(std::string{ "abc123" })).TIMES(1).LR_RETURN(false);

  std::ostringstream oss;
  auto oldBuf = std::cerr.rdbuf(oss.rdbuf());

  component.onCommand(UserDto{}, "user_session_revoke", { "-s", "abc123" });

  std::cerr.rdbuf(oldBuf);
  REQUIRE(oss.str().find("ERROR") != std::string::npos);
}

TEST_CASE("UserManagementCliComponent: an exception from the api is caught and printed")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, allOf()).TIMES(1).LR_THROW(std::runtime_error("boom"));

  std::ostringstream oss;
  auto oldBuf = std::cerr.rdbuf(oss.rdbuf());

  component.onCommand(UserDto{}, "show_groups", {});

  std::cerr.rdbuf(oldBuf);
  REQUIRE(oss.str().find("boom") != std::string::npos);
}
TEST_CASE("UserManagementCliComponent: group_add with -g calls createGroupOf")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, createGroupOf(ANY(const GroupDto &)))
    .WITH(_1.getGroupName() == "ops" && !_1.isVirtual())
    .TIMES(1)
    .LR_RETURN(true);

  UserDto userDto;
  component.onCommand(userDto, "group_add", { "-g", "ops" });
}

TEST_CASE("UserManagementCliComponent: group_add with -g -v calls createGroupOf for a virtual group")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, createGroupOf(ANY(const GroupDto &)))
    .WITH(_1.getGroupName() == "ops" && _1.isVirtual())
    .TIMES(1)
    .LR_RETURN(true);

  UserDto userDto;
  component.onCommand(userDto, "group_add", { "-g", "ops", "-v" });
}

TEST_CASE("UserManagementCliComponent: group_add without -g prints error")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  FORBID_CALL(*mockUserApi, createGroupOf(ANY(const GroupDto &)));

  UserDto userDto;
  std::ostringstream oss;
  auto oldBuf = std::cerr.rdbuf(oss.rdbuf());

  component.onCommand(userDto, "group_add", {});

  std::cerr.rdbuf(oldBuf);
  REQUIRE(oss.str().find("ERROR") != std::string::npos);
}

TEST_CASE("UserManagementCliComponent: a failed group_add prints an error")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, createGroupOf(ANY(const GroupDto &))).TIMES(1).LR_RETURN(false);

  std::ostringstream oss;
  auto oldBuf = std::cerr.rdbuf(oss.rdbuf());

  component.onCommand(UserDto{}, "group_add", { "-g", "ops" });

  std::cerr.rdbuf(oldBuf);
  REQUIRE(oss.str().find("ERROR") != std::string::npos);
}

TEST_CASE("UserManagementCliComponent: group_remove calls deleteGroupOf")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, deleteGroupOf(std::string{ "ops" })).TIMES(1).LR_RETURN(true);

  UserDto userDto;
  component.onCommand(userDto, "group_remove", { "-g", "ops" });
}

TEST_CASE("UserManagementCliComponent: group_remove without -g prints error")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  FORBID_CALL(*mockUserApi, deleteGroupOf(ANY(std::string)));

  UserDto userDto;
  std::ostringstream oss;
  auto oldBuf = std::cerr.rdbuf(oss.rdbuf());

  component.onCommand(userDto, "group_remove", {});

  std::cerr.rdbuf(oldBuf);
  REQUIRE(oss.str().find("ERROR") != std::string::npos);
}

TEST_CASE("UserManagementCliComponent: group_add_subgroup calls addSubGroupOf")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, addSubGroupOf(std::string{ "ops" }, std::string{ "admin" })).TIMES(1).LR_RETURN(true);

  UserDto userDto;
  component.onCommand(userDto, "group_add_subgroup", { "-g", "ops", "-s", "admin" });
}

TEST_CASE("UserManagementCliComponent: group_add_subgroup without -s prints error")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  FORBID_CALL(*mockUserApi, addSubGroupOf(ANY(std::string), ANY(std::string)));

  UserDto userDto;
  std::ostringstream oss;
  auto oldBuf = std::cerr.rdbuf(oss.rdbuf());

  component.onCommand(userDto, "group_add_subgroup", { "-g", "ops" });

  std::cerr.rdbuf(oldBuf);
  REQUIRE(oss.str().find("ERROR") != std::string::npos);
}

TEST_CASE("UserManagementCliComponent: group_remove_subgroup calls removeSubGroupOf")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  REQUIRE_CALL(*mockUserApi, removeSubGroupOf(std::string{ "ops" }, std::string{ "admin" })).TIMES(1).LR_RETURN(true);

  UserDto userDto;
  component.onCommand(userDto, "group_remove_subgroup", { "-g", "ops", "-s", "admin" });
}

TEST_CASE("UserManagementCliComponent: group_remove_subgroup without -g prints error")
{
  auto mockUserApi = std::make_shared<MockUserApi>();
  UserManagementCliComponent component(mockUserApi);

  FORBID_CALL(*mockUserApi, removeSubGroupOf(ANY(std::string), ANY(std::string)));

  UserDto userDto;
  std::ostringstream oss;
  auto oldBuf = std::cerr.rdbuf(oss.rdbuf());

  component.onCommand(userDto, "group_remove_subgroup", { "-s", "admin" });

  std::cerr.rdbuf(oldBuf);
  REQUIRE(oss.str().find("ERROR") != std::string::npos);
}
}// namespace lintel
