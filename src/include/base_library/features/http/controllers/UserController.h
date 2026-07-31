#ifndef CPP_BASE_LIBRARY_USERCONTROLLER_H
#define CPP_BASE_LIBRARY_USERCONTROLLER_H

#include "base_library/features/base/repositories/UserRepository.h"
#include "base_library/features/base/repositories/GroupRepository.h"
#include "base_library/features/base/services/IAuthService.h"
#include "base_library/features/http/service/Controller.h"
#include "base_library/features/http/service/ContentType.h"

/** HTTP controller for user authentication and management */
class UserController : public Controller {
private:
    std::shared_ptr<GroupRepository> m_groupRepository;
    std::shared_ptr<UserRepository> m_userRepository;
    Group m_adminUser;
    Group m_userUser;

    ADD_HANDLER_METHOD("/user/login", Post, loginOf);

    ADD_HANDLER_METHOD("/user/logout", Delete, logoutOf);

    ADD_HANDLER_METHOD("/user/state", Get, loginStateOf);

    ADD_HANDLER_METHOD(R"(/user/groups)", Get, allGroupsOf);

    ADD_HANDLER_METHOD(R"(/user/groups/([^\/]+))", Get,
                       allGroupsOfGroupNameOrIsVirtualGroup);

    ADD_HANDLER_METHOD(R"(/user/groups/([^\/]+)/([^\/]+))", Get,
                       allGroupsOfGroupNameAndIsVirtualGroup);

    ADD_HANDLER_METHOD(R"(/user/users)", Get, allUsersOf);

    ADD_HANDLER_METHOD(R"(/user/users/([^\/]+))", Get, allUsersOfUserName);

    ADD_HANDLER_METHOD("/user/add", Post, addUser);

    ADD_HANDLER_METHOD("/user/update", Put, updateUser);

    ADD_HANDLER_METHOD("/user/password", Put, changePasswordOf);

    ADD_HANDLER_METHOD(R"(/user/sessions)", Get, allSessionsOf);

    ADD_HANDLER_METHOD(R"(/user/sessions/([^\/]+))", Delete, revokeSessionOf);

    ADD_HANDLER_METHOD("/user/delete", Delete, deleteUser);

public:
    explicit UserController(const std::shared_ptr<IAuthService> &authService,
                            std::shared_ptr<GroupRepository> groupRepository,
                            std::shared_ptr<UserRepository> userRepository);
};

#endif  // CPP_BASE_LIBRARY_USERCONTROLLER_H
