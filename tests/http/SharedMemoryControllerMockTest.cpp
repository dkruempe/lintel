#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>

#include <memory>
#include <regex>
#include <string_view>
#include <vector>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "../mocks/MockAuthService.h"
#include "../mocks/MockSharedMemorySegmentManager.h"
#include "../mocks/MockSharedMemoryService.h"
#include "lintel/core/models/SharedMemorySegment.h"
#include "lintel/core/models/SharedMemorySegmentInfo.h"
#include "lintel/features/base/models/Group.h"
#include "lintel/features/base/models/User.h"
#include "lintel/features/http/controllers/SharedMemoryController.h"

namespace lintel {
using namespace trompeloeil;

namespace {

  // `k` prefix: in unity builds all test files share one anonymous namespace,
  // where a generic `adminGroup` would shadow local variables of other test
  // files (Clang -Wshadow).
  Group kAdminGroup{ "Admin-Shm", {}, true };
  Group kUserGroup{ "User-Shm", {}, true };

  User testUser("Admin", "User", User::Male, "admin@test.com", "admin", "", { kAdminGroup, kUserGroup });

  UserToken makeToken()
  {
    return UserToken{ "127.0.0.1",
      "token-id",
      std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now()),
      testUser };
  }

  struct TestSharedMemoryController : public SharedMemoryController
  {
    using SharedMemoryController::SharedMemoryController;
  };

  class ShmCtlFakeRepository : public SharedMemoryRepository
  {
  public:
    ShmCtlFakeRepository(std::shared_ptr<SharedMemorySegment> segment, std::string_view name, std::string uuid)
      : SharedMemoryRepository(std::move(segment), 8, name, 1, std::move(uuid))
    {
    }

    SharedMemoryType getType() const override { return SharedMemoryType::Map; }

    void onMigrate(int32_t) override {}

    void serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const override
    {
      writer->StartObject();
      writer->EndObject();
    }
  };

  UserToken tokenForGroups(const std::vector<Group> &groups)
  {
    User user("Admin", "User", User::Male, "admin@test.com", "admin", "", groups);
    return UserToken{ "127.0.0.1",
      "token-id",
      std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now()),
      user };
  }

  httplib::Request requestWithMatches(const std::vector<std::string> &parts)
  {
    // The smatch stored in the request holds iterators into the subject string,
    // so the subject must outlive the returned request.
    static std::string subject;
    subject.clear();
    std::string pattern;
    for (std::size_t i = 0; i < parts.size(); ++i) {
      if (i != 0) {
        subject += "/";
        pattern += "/";
      }
      subject += parts[i];
      pattern += "(.*)";
    }
    httplib::Request request;
    std::smatch m;
    std::regex_match(subject, m, std::regex(pattern));
    request.matches = m;
    return request;
  }

}// namespace

TEST_CASE("SharedMemoryController can be constructed with mocks")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();
  REQUIRE_NOTHROW(TestSharedMemoryController(mockAuth, mockShmService, mockShmManager, {}));
}

TEST_CASE("SharedMemoryController::allSegmentsOfGet calls allOf and showStateOf")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  auto segment = std::make_shared<SharedMemorySegment>("/tmp/test", "test", 1024);
  SharedMemorySegmentInfo info(segment, 512, 512, 1, 2, true);

  REQUIRE_CALL(*mockShmManager, allOf("test"))
    .TIMES(1)
    .LR_RETURN(std::vector<std::shared_ptr<SharedMemorySegment>>{ segment });
  REQUIRE_CALL(*mockShmService, showStateOf(segment)).TIMES(1).LR_RETURN(info);

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  httplib::Request request;
  std::smatch m;
  std::string str{ "test" };
  std::regex_match(str, m, std::regex("(.*)"));
  request.matches = m;
  httplib::Response response;
  ContentType contentType("application/json");
  auto userToken = makeToken();

  controller.allSegmentsOfGet(request, response, contentType, userToken);
}

TEST_CASE("SharedMemoryController::shrinkSegmentOfPut calls of and shrinkOf")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  auto segment = std::make_shared<SharedMemorySegment>("/tmp/test", "test", 1024);

  REQUIRE_CALL(*mockShmManager, of("test")).TIMES(1).LR_RETURN(segment);
  REQUIRE_CALL(*mockShmService, shrinkOf(segment)).TIMES(1);

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  httplib::Request request;
  std::smatch m;
  std::string str{ "test" };
  std::regex_match(str, m, std::regex("(.*)"));
  request.matches = m;
  httplib::Response response;
  ContentType contentType("application/json");
  auto userToken = makeToken();

  controller.shrinkSegmentOfPut(request, response, contentType, userToken);
}

TEST_CASE("SharedMemoryController::growSegmentOfPut calls of and growOf")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  auto segment = std::make_shared<SharedMemorySegment>("/tmp/test", "test", 1024);

  REQUIRE_CALL(*mockShmManager, of("test")).TIMES(1).LR_RETURN(segment);
  REQUIRE_CALL(*mockShmService, growOf(segment, 4096)).TIMES(1);

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  httplib::Request request;
  std::smatch m;
  std::string str{ "test/4096" };
  std::regex_match(str, m, std::regex("(.*)/(.*)"));
  request.matches = m;
  httplib::Response response;
  ContentType contentType("application/json");
  auto userToken = makeToken();

  controller.growSegmentOfPut(request, response, contentType, userToken);
}

TEST_CASE("SharedMemoryController returns 401 for unauthenticated requests")
{
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

TEST_CASE("SharedMemoryController::shrinkSegmentOfPut returns 403 for unknown segment")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  REQUIRE_CALL(*mockShmManager, of("unknown")).TIMES(1).LR_RETURN(nullptr);

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  httplib::Request request;
  std::smatch m;
  std::string str{ "unknown" };
  std::regex_match(str, m, std::regex("(.*)"));
  request.matches = m;
  httplib::Response response;
  ContentType contentType("application/json");
  auto userToken = makeToken();

  FORBID_CALL(*mockShmService, shrinkOf(ANY(std::shared_ptr<SharedMemorySegment>)));
  controller.shrinkSegmentOfPut(request, response, contentType, userToken);
  REQUIRE(response.status == 403);
}

TEST_CASE("SharedMemoryController::allSegmentsOfGet returns 403 for non-json")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  auto request = requestWithMatches({ "test" });
  httplib::Response response;
  ContentType contentType("text/plain");

  controller.allSegmentsOfGet(request, response, contentType, makeToken());
  REQUIRE(response.status == 403);
}

TEST_CASE("SharedMemoryController::allSegmentsOfGet returns 401 for user without groups")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  auto request = requestWithMatches({ "test" });
  httplib::Response response;
  ContentType contentType("application/json");

  controller.allSegmentsOfGet(request, response, contentType, tokenForGroups({ Group{ "Other", {}, true } }));
  REQUIRE(response.status == 401);
}

TEST_CASE("SharedMemoryController::allRepositoriesOfGet lists matching repositories")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  auto segment = std::make_shared<SharedMemorySegment>("/tmp/test", "mySeg", 1024);
  auto repo = std::make_shared<ShmCtlFakeRepository>(segment, "myRepo", "uuid-1");

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, { repo });

  auto request = requestWithMatches({ "myRepo.*", "mySeg.*" });
  httplib::Response response;
  ContentType contentType("application/json");

  controller.allRepositoriesOfGet(request, response, contentType, makeToken());
  REQUIRE(response.body.find("myRepo") != std::string::npos);
}

TEST_CASE("SharedMemoryController::allRepositoriesOfGet skips non-matching repositories")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  auto segment = std::make_shared<SharedMemorySegment>("/tmp/test", "mySeg", 1024);
  auto repo = std::make_shared<ShmCtlFakeRepository>(segment, "myRepo", "uuid-1");

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, { repo });

  auto request = requestWithMatches({ "nope.*", "nope.*" });
  httplib::Response response;
  ContentType contentType("application/json");

  controller.allRepositoriesOfGet(request, response, contentType, makeToken());
  REQUIRE(response.body.find("myRepo") == std::string::npos);
}

TEST_CASE("SharedMemoryController::allRepositoriesOfGet returns 400 for invalid regex")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  auto request = requestWithMatches({ "[", ".*" });
  httplib::Response response;
  ContentType contentType("application/json");

  controller.allRepositoriesOfGet(request, response, contentType, makeToken());
  REQUIRE(response.status == 400);
}

TEST_CASE("SharedMemoryController::allRepositoriesOfGet returns 403 for non-json")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  auto request = requestWithMatches({ ".*", ".*" });
  httplib::Response response;
  ContentType contentType("text/plain");

  controller.allRepositoriesOfGet(request, response, contentType, makeToken());
  REQUIRE(response.status == 403);
}

TEST_CASE("SharedMemoryController::allRepositoriesOfGet returns 401 without user")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  auto request = requestWithMatches({ ".*", ".*" });
  httplib::Response response;
  ContentType contentType("application/json");

  controller.allRepositoriesOfGet(request, response, contentType, std::nullopt);
  REQUIRE(response.status == 401);
}

TEST_CASE("SharedMemoryController::exportRepositoryOfGet serializes a known repository")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  auto segment = std::make_shared<SharedMemorySegment>("/tmp/test", "mySeg", 1024);
  auto repo = std::make_shared<ShmCtlFakeRepository>(segment, "myRepo", "uuid-1");

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, { repo });

  auto request = requestWithMatches({ "uuid-1" });
  httplib::Response response;
  ContentType contentType("application/json");

  controller.exportRepositoryOfGet(request, response, contentType, makeToken());
  REQUIRE(response.body == "{}");
}

TEST_CASE("SharedMemoryController::exportRepositoryOfGet returns 404 for unknown repository")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  auto request = requestWithMatches({ "missing" });
  httplib::Response response;
  ContentType contentType("application/json");

  controller.exportRepositoryOfGet(request, response, contentType, makeToken());
  REQUIRE(response.status == 404);
}

TEST_CASE("SharedMemoryController::exportRepositoryOfGet returns 403 for non-json")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  auto request = requestWithMatches({ "uuid-1" });
  httplib::Response response;
  ContentType contentType("text/plain");

  controller.exportRepositoryOfGet(request, response, contentType, makeToken());
  REQUIRE(response.status == 403);
}

TEST_CASE("SharedMemoryController::exportRepositoryOfGet returns 401 without user")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  auto request = requestWithMatches({ "uuid-1" });
  httplib::Response response;
  ContentType contentType("application/json");

  controller.exportRepositoryOfGet(request, response, contentType, std::nullopt);
  REQUIRE(response.status == 401);
}

TEST_CASE("SharedMemoryController::growSegmentOfPut returns 400 for invalid size")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  auto request = requestWithMatches({ "test", "notasize" });
  httplib::Response response;
  ContentType contentType("application/json");

  controller.growSegmentOfPut(request, response, contentType, makeToken());
  REQUIRE(response.status == 400);
}

TEST_CASE("SharedMemoryController::growSegmentOfPut returns 403 for unknown segment")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  REQUIRE_CALL(*mockShmManager, of("unknown")).TIMES(1).LR_RETURN(nullptr);

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  auto request = requestWithMatches({ "unknown", "4096" });
  httplib::Response response;
  ContentType contentType("application/json");

  controller.growSegmentOfPut(request, response, contentType, makeToken());
  REQUIRE(response.status == 403);
}

TEST_CASE("SharedMemoryController::exportRepositoryOfGet returns 401 for a user without groups")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  auto request = requestWithMatches({ "uuid-1" });
  httplib::Response response;
  ContentType contentType("application/json");

  controller.exportRepositoryOfGet(request, response, contentType, tokenForGroups({ Group{ "Other", {}, true } }));
  REQUIRE(response.status == 401);
}

TEST_CASE("SharedMemoryController::allRepositoriesOfGet returns 401 for a user without groups")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  auto request = requestWithMatches({ ".*", ".*" });
  httplib::Response response;
  ContentType contentType("application/json");

  controller.allRepositoriesOfGet(request, response, contentType, tokenForGroups({ Group{ "Other", {}, true } }));
  REQUIRE(response.status == 401);
}

TEST_CASE("SharedMemoryController::shrinkSegmentOfPut returns 401 without a user")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  auto request = requestWithMatches({ "test" });
  httplib::Response response;
  ContentType contentType("application/json");

  controller.shrinkSegmentOfPut(request, response, contentType, std::nullopt);
  REQUIRE(response.status == 401);
}

TEST_CASE("SharedMemoryController::shrinkSegmentOfPut returns 401 for a user without groups")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  auto request = requestWithMatches({ "test" });
  httplib::Response response;
  ContentType contentType("application/json");

  controller.shrinkSegmentOfPut(request, response, contentType, tokenForGroups({ Group{ "Other", {}, true } }));
  REQUIRE(response.status == 401);
}

TEST_CASE("SharedMemoryController::shrinkSegmentOfPut returns 403 for non-json")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  auto request = requestWithMatches({ "test" });
  httplib::Response response;
  ContentType contentType("text/plain");

  controller.shrinkSegmentOfPut(request, response, contentType, makeToken());
  REQUIRE(response.status == 403);
}

TEST_CASE("SharedMemoryController::growSegmentOfPut returns 401 without a user")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  auto request = requestWithMatches({ "test", "4096" });
  httplib::Response response;
  ContentType contentType("application/json");

  controller.growSegmentOfPut(request, response, contentType, std::nullopt);
  REQUIRE(response.status == 401);
}

TEST_CASE("SharedMemoryController::growSegmentOfPut returns 401 for a user without groups")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  auto request = requestWithMatches({ "test", "4096" });
  httplib::Response response;
  ContentType contentType("application/json");

  controller.growSegmentOfPut(request, response, contentType, tokenForGroups({ Group{ "Other", {}, true } }));
  REQUIRE(response.status == 401);
}

TEST_CASE("SharedMemoryController::growSegmentOfPut returns 403 for non-json")
{
  auto mockAuth = std::make_shared<MockAuthService>();
  auto mockShmService = std::make_shared<MockSharedMemoryService>();
  auto mockShmManager = std::make_shared<MockSharedMemorySegmentManager>();

  TestSharedMemoryController controller(mockAuth, mockShmService, mockShmManager, {});

  auto request = requestWithMatches({ "test", "4096" });
  httplib::Response response;
  ContentType contentType("text/plain");

  controller.growSegmentOfPut(request, response, contentType, makeToken());
  REQUIRE(response.status == 403);
}
}// namespace lintel
