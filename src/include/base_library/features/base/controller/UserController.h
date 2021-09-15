#ifndef CPP_BASE_LIBRARY_USERCONTROLLER_H
#define CPP_BASE_LIBRARY_USERCONTROLLER_H

#include "base_library/features/base/repositories/UserRepository.h"
#include "base_library/features/base/services/AuthService.h"
#include "base_library/features/http/service/Controller.h"

class UserController : public Controller {
 private:
  std::shared_ptr<AuthService> m_authService;
  std::shared_ptr<GroupRepository> m_groupRepository;
  std::shared_ptr<UserRepository> m_userRepository;
  Group m_adminUser;
  Group m_userUser;

  ADD_HANDLER_METHOD("/user/login", Post, loginOf);
  ADD_HANDLER_METHOD("/user/logout", Delete, logoutOf);
  ADD_HANDLER_METHOD(R"(/user/groups)", Get, allGroupsOf);
  ADD_HANDLER_METHOD(R"(/user/groups/([^\/]+))", Get,
                     allGroupsOfGroupNameOrIsVirtualGroup);
  ADD_HANDLER_METHOD(R"(/user/groups/([^\/]+)/([^\/]+))", Get,
                     allGroupsOfGroupNameAndIsVirtualGroup);
  ADD_HANDLER_METHOD(R"(/user/users)", Get, allUsersOf);
  ADD_HANDLER_METHOD("/user/add", Post, addUser);
  ADD_HANDLER_METHOD("/user/update", Put, updateUser);

 public:
  explicit UserController(const std::shared_ptr<AuthService> &authService,
                          std::shared_ptr<GroupRepository> groupRepository,
                          std::shared_ptr<UserRepository> userRepository);
};

#endif  // CPP_BASE_LIBRARY_USERCONTROLLER_H
