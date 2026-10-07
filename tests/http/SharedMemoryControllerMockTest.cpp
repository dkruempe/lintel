#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>

#include <memory>
#include <regex>

#include "../mocks/MockAuthService.h"
#include "../mocks/MockSharedMemorySegmentManager.h"
#include "../mocks/MockSharedMemoryService.h"
#include "lintel/core/models/SharedMemorySegment.h"
#include "lintel/features/base/models/Group.h"
#include "lintel/features/base/models/SharedMemorySegmentInfo.h"
#include "lintel/features/base/models/User.h"
#include "lintel/features/http/controllers/SharedMemoryController.h"

using namespace trompeloeil;

namespace {

// `k` prefix: in unity builds all test files share one anonymous namespace,
// where a generic `adminGroup` would shadow local variables of other test
// files (Clang -Wshadow).
Group kAdminGroup{"Admin-Shm", {}, true};
Group kUserGroup{"User-Shm", {}, true};

User testUser("Admin", "User", User::Male, "admin@test.com", "admin", "",
              {kAdminGroup, kUserGroup});

UserToken makeToken() {
    return UserToken{"127.0.0.1", "token-id",
                     std::chrono::time_point_cast<std::chrono::microseconds>(
                             std::chrono::system_clock::now()),
                     testUser};
}

struct TestSharedMemoryController : public SharedMemoryController {
    using SharedMemoryController::SharedMemoryController;
};

}  // namespace

TEST_CASE("SharedMemoryController can be constructed with mocks") {
    auto mockAuth = std::make_shared<MockAuthService>();
    auto mockShmService = std::make_shared<MockSharedMemoryService>();
    auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();
    REQUIRE_NOTHROW(TestSharedMemoryController(mockAuth, mockShmService,
                                               mockShmManager, {}));
}

TEST_CASE("SharedMemoryController::allSegmentsOfGet calls allOf and showStateOf") {
    auto mockAuth = std::make_shared<MockAuthService>();
    auto mockShmService = std::make_shared<MockSharedMemoryService>();
    auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

    auto segment = std::make_shared<SharedMemorySegment>("/tmp/test", "test", 1024);
    SharedMemorySegmentInfo info(segment, 512, 512, 1, 2, true);

    REQUIRE_CALL(*mockShmManager, allOf("test"))
        .TIMES(1)
        .LR_RETURN(std::vector<std::shared_ptr<SharedMemorySegment>>{segment});
    REQUIRE_CALL(*mockShmService, showStateOf(segment))
        .TIMES(1)
        .LR_RETURN(info);

    TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

    httplib::Request request;
    std::smatch m;
    std::string str{"test"};
    std::regex_match(str, m, std::regex("(.*)"));
    request.matches = m;
    httplib::Response response;
    ContentType contentType("application/json");
    auto userToken = makeToken();

    controller.allSegmentsOfGet(request, response, contentType, userToken);
}

TEST_CASE("SharedMemoryController::shrinkSegmentOfPut calls of and shrinkOf") {
    auto mockAuth = std::make_shared<MockAuthService>();
    auto mockShmService = std::make_shared<MockSharedMemoryService>();
    auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

    auto segment = std::make_shared<SharedMemorySegment>("/tmp/test", "test", 1024);

    REQUIRE_CALL(*mockShmManager, of("test"))
        .TIMES(1)
        .LR_RETURN(segment);
    REQUIRE_CALL(*mockShmService, shrinkOf(segment)).TIMES(1);

    TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

    httplib::Request request;
    std::smatch m;
    std::string str{"test"};
    std::regex_match(str, m, std::regex("(.*)"));
    request.matches = m;
    httplib::Response response;
    ContentType contentType("application/json");
    auto userToken = makeToken();

    controller.shrinkSegmentOfPut(request, response, contentType, userToken);
}

TEST_CASE("SharedMemoryController::growSegmentOfPut calls of and growOf") {
    auto mockAuth = std::make_shared<MockAuthService>();
    auto mockShmService = std::make_shared<MockSharedMemoryService>();
    auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

    auto segment = std::make_shared<SharedMemorySegment>("/tmp/test", "test", 1024);

    REQUIRE_CALL(*mockShmManager, of("test"))
        .TIMES(1)
        .LR_RETURN(segment);
    REQUIRE_CALL(*mockShmService, growOf(segment, 4096)).TIMES(1);

    TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

    httplib::Request request;
    std::smatch m;
    std::string str{"test/4096"};
    std::regex_match(str, m, std::regex("(.*)/(.*)"));
    request.matches = m;
    httplib::Response response;
    ContentType contentType("application/json");
    auto userToken = makeToken();

    controller.growSegmentOfPut(request, response, contentType, userToken);
}

TEST_CASE("SharedMemoryController returns 401 for unauthenticated requests") {
    auto mockAuth = std::make_shared<MockAuthService>();
    auto mockShmService = std::make_shared<MockSharedMemoryService>();
    auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

    TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

    httplib::Request request;
    httplib::Response response;
    ContentType contentType("application/json");

    controller.allSegmentsOfGet(request, response, contentType, std::nullopt);
    REQUIRE(response.status == 401);
}

TEST_CASE("SharedMemoryController::shrinkSegmentOfPut returns 403 for unknown segment") {
    auto mockAuth = std::make_shared<MockAuthService>();
    auto mockShmService = std::make_shared<MockSharedMemoryService>();
    auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

    REQUIRE_CALL(*mockShmManager, of("unknown"))
        .TIMES(1)
        .LR_RETURN(nullptr);

    TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

    httplib::Request request;
    std::smatch m;
    std::string str{"unknown"};
    std::regex_match(str, m, std::regex("(.*)"));
    request.matches = m;
    httplib::Response response;
    ContentType contentType("application/json");
    auto userToken = makeToken();

    FORBID_CALL(*mockShmService, shrinkOf(ANY(std::shared_ptr<SharedMemorySegment>)));
    controller.shrinkSegmentOfPut(request, response, contentType, userToken);
    REQUIRE(response.status == 403);
}
