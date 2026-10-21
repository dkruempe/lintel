#include <boost/interprocess/sync/named_semaphore.hpp>

#include "../helpers/PlatformCompat.h"

#include <catch2/catch_all.hpp>

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <vector>

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/core/models/ProcessName.h"
#include "lintel/core/models/SharedMemorySegment.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/configuration/EventBusComponent.h"
#include "lintel/features/base/configuration/EventBusEntry.h"
#include "lintel/features/base/configuration/SharedMemorySegmentComponent.h"
#include "lintel/features/base/configuration/SharedMemorySegmentEntry.h"
#include "lintel/features/base/events/Event.h"
#include "lintel/features/base/events/EventBus.h"
#include "lintel/features/base/models/Group.h"
#include "lintel/features/base/models/User.h"
#include "lintel/features/base/services/EventBusService.h"
#include "lintel/features/base/services/SchedulerService.h"
#include "lintel/features/base/services/SharedMemorySegmentManager.h"
#include "lintel/features/base/services/SharedMemoryService.h"
#include "lintel/features/http/controllers/EventBusController.h"
#include "lintel/features/http/service/ContentType.h"
#include "lintel/features/http/service/HttpStatusCodes.h"

#include "../helpers/ScopedEnvironmentVariable.h"
#include "../mocks/MockAuthService.h"

namespace lintel {
namespace {

  // `ebc` prefix: unity builds merge several test files into one anonymous
  // namespace, so a generic `adminGroup` or `fixture` would collide.
  constexpr std::size_t ebcSegmentSize = 1u << 20;
  constexpr char ebcBusName[] = "orders";

  Group ebcAdminGroup{ "Admin-EventBus", {}, true };
  Group ebcUserGroup{ "User-EventBus", {}, true };

  User ebcAdmin("EventBus", "Admin", User::Male, "eventbus-admin@test.com", "eventbusAdmin", "", { ebcAdminGroup });
  User ebcUser("EventBus", "User", User::Male, "eventbus-user@test.com", "eventbusUser", "", { ebcUserGroup });

  UserToken ebcTokenOf(const User &user)
  {
    return UserToken{ "127.0.0.1",
      "token-id",
      std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now()),
      user };
  }

  std::filesystem::path ebcUniquePath()
  {
    static unsigned int counter = 0;
    const auto path =
      std::filesystem::temp_directory_path()
      / ("eventbus_controller_test_" + std::to_string(::getpid()) + "_" + std::to_string(counter++) + ".bin");
    std::filesystem::remove(path);
    return path;
  }

  // Keep the segment name short: the remap semaphore appends "_remap_sem" and
  // POSIX caps semaphore names at 31 characters.
  std::string ebcUniqueName()
  {
    static unsigned int counter = 0;
    return "ebctl_" + std::to_string(::getpid()) + "_" + std::to_string(counter++);
  }

  /**
   * Build a controller over a real EventBusService in a temporary shared memory
   * segment. The service is cheap to construct and the handlers are exercised
   * directly, so no HTTP server is needed; the status of a successful handler is
   * set by the server in production, hence the assertions look at the body.
   */
  struct EventBusControllerFixture
  {
    // must stay the first member: it has to be set before the configuration is built
    ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_ebc_test_cfg" };
    std::filesystem::path m_path = ebcUniquePath();
    std::string m_segmentName = ebcUniqueName();
    std::shared_ptr<SharedMemorySegment> m_segment;
    std::shared_ptr<SchedulerService> m_scheduler;
    std::shared_ptr<SharedMemorySegmentManager> m_segmentManager;
    std::shared_ptr<SharedMemoryService> m_sharedMemoryService;
    std::shared_ptr<EventBusService> m_eventBusService;
    std::shared_ptr<MockAuthService> m_authService = std::make_shared<MockAuthService>();
    // constructed in the body: the handlers capture `this`, so a copy of the
    // controller would capture the destroyed temporary
    std::unique_ptr<EventBusController> m_controller;

    EventBusControllerFixture()
    {
      m_segment = std::make_shared<SharedMemorySegment>(m_path, m_segmentName, ebcSegmentSize);
      auto configuration = std::make_shared<Configuration>(
        std::vector<std::shared_ptr<Component>>{}, std::make_shared<EnvironmentConfiguration>());
      auto busEntry = std::make_shared<EventBusEntry>(
        type_name<EventBusComponent>(), ebcBusName, m_segmentName, EventBusCapacities{ 8, 8, 8, 8 });
      configuration->setEntries(
        { std::make_shared<SharedMemorySegmentEntry>(type_name<SharedMemorySegmentComponent>(), m_segment), busEntry });
      m_segmentManager = std::make_shared<SharedMemorySegmentManager>(configuration);
      m_scheduler = std::make_shared<SchedulerService>(std::make_shared<ProcessName>("eventbus-controller-test"));
      m_sharedMemoryService = std::make_shared<SharedMemoryService>(
        m_segmentManager, m_scheduler, std::make_shared<ProcessName>("eventbus-controller-test"));
      m_eventBusService = std::make_shared<EventBusService>(configuration,
        std::make_shared<ProcessName>("eventbus-controller-test"),
        m_sharedMemoryService,
        m_segmentManager);
      m_controller = std::make_unique<EventBusController>(m_authService, m_eventBusService);
    }

    ~EventBusControllerFixture()
    {
      m_eventBusService.reset();
      m_sharedMemoryService.reset();
      m_scheduler.reset();
      std::filesystem::remove(m_path);
      boost::interprocess::named_semaphore::remove((m_segmentName + "_remap_sem").c_str());
    }

    EventBusControllerFixture(const EventBusControllerFixture &) = delete;
    EventBusControllerFixture &operator=(const EventBusControllerFixture &) = delete;

    /** @return the configured bus */
    std::shared_ptr<IEventBus> bus() const { return m_eventBusService->of(ebcBusName); }
  };

  /** Build a request whose regex matches are the given path segments. */
  httplib::Request ebcRequestWithMatches(const std::vector<std::string> &parts)
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
    std::smatch matches;
    std::regex_match(subject, matches, std::regex(pattern));
    request.matches = matches;
    return request;
  }

  ContentType ebcJson() { return ContentType("application/json"); }

  constexpr char ebcPublishBody[] =
    R"({"TOPIC":"orders.created","TYPE":"OrderCreated","PAYLOAD":"42","PRIORITY":"High"})";

}// namespace

TEST_CASE("EventBusController: reading needs a token")
{
  EventBusControllerFixture ebcFixture;
  EventBusController &ebcController = *ebcFixture.m_controller;
  httplib::Request ebcReq;
  httplib::Response ebcRes;

  ebcController.busesOfGet(ebcReq, ebcRes, ebcJson(), std::nullopt);
  REQUIRE(ebcRes.status == 401);
}

TEST_CASE("EventBusController: reading needs the user or admin group")
{
  EventBusControllerFixture ebcFixture;
  EventBusController &ebcController = *ebcFixture.m_controller;
  httplib::Request ebcReq;
  httplib::Response ebcRes;

  User ebcOutsider(
    "EventBus", "Outsider", User::Male, "outsider@test.com", "outsider", "", { Group{ "Other", {}, true } });
  ebcController.busesOfGet(ebcReq, ebcRes, ebcJson(), ebcTokenOf(ebcOutsider));
  REQUIRE(ebcRes.status == 401);
}

TEST_CASE("EventBusController: everything but json is forbidden")
{
  EventBusControllerFixture ebcFixture;
  EventBusController &ebcController = *ebcFixture.m_controller;
  httplib::Request ebcReq;
  httplib::Response ebcRes;

  ebcController.busesOfGet(ebcReq, ebcRes, ContentType("text/plain"), ebcTokenOf(ebcUser));
  REQUIRE(ebcRes.status == 403);
}

TEST_CASE("EventBusController: busesOfGet lists the configuration and the counters")
{
  EventBusControllerFixture ebcFixture;
  EventBusController &ebcController = *ebcFixture.m_controller;
  httplib::Request ebcReq;
  httplib::Response ebcRes;

  ebcController.busesOfGet(ebcReq, ebcRes, ebcJson(), ebcTokenOf(ebcUser));

  // the handler does not set the status on success - the server does
  REQUIRE(ebcRes.body.find("\"NAME\":\"orders\"") != std::string::npos);
  REQUIRE(ebcRes.body.find("\"AVAILABLE\":true") != std::string::npos);
  REQUIRE(ebcRes.body.find("\"BUS_CAPACITY\":8") != std::string::npos);
  REQUIRE(ebcRes.body.find("\"SUBSCRIBER_CAPACITY\":8") != std::string::npos);
  REQUIRE(ebcRes.body.find("\"MAX_TOPICS\":8") != std::string::npos);
  REQUIRE(ebcRes.body.find("\"PUB_POLICY\":\"Fail\"") != std::string::npos);
  REQUIRE(ebcRes.body.find("\"DROPPED\":0") != std::string::npos);
  REQUIRE(ebcRes.body.find("\"TOPIC_COUNT\":0") != std::string::npos);
}

TEST_CASE("EventBusController: busOfGet serializes one bus")
{
  EventBusControllerFixture ebcFixture;
  EventBusController &ebcController = *ebcFixture.m_controller;
  auto ebcReq = ebcRequestWithMatches({ "orders" });
  httplib::Response ebcRes;

  ebcController.busOfGet(ebcReq, ebcRes, ebcJson(), ebcTokenOf(ebcAdmin));

  REQUIRE(ebcRes.body.find("\"NAME\":\"orders\"") != std::string::npos);
  REQUIRE(ebcRes.body.find("\"AVAILABLE\":true") != std::string::npos);
}

TEST_CASE("EventBusController: an unknown bus is 404")
{
  EventBusControllerFixture ebcFixture;
  EventBusController &ebcController = *ebcFixture.m_controller;
  auto ebcReq = ebcRequestWithMatches({ "unknown" });
  httplib::Response ebcRes;

  ebcController.busOfGet(ebcReq, ebcRes, ebcJson(), ebcTokenOf(ebcAdmin));
  REQUIRE(ebcRes.status == 404);
}

TEST_CASE("EventBusController: publishOfPost queues the event on the bus")
{
  EventBusControllerFixture ebcFixture;
  EventBusController &ebcController = *ebcFixture.m_controller;
  auto ebcReq = ebcRequestWithMatches({ "orders" });
  ebcReq.body = ebcPublishBody;
  httplib::Response ebcRes;

  // a plain user may publish
  ebcController.publishOfPost(ebcReq, ebcRes, ebcJson(), ebcTokenOf(ebcUser));

  REQUIRE(ebcRes.body.find("\"PUBLISHED\":true") != std::string::npos);
  REQUIRE(ebcRes.body.find("\"PRIORITY\":\"High\"") != std::string::npos);
  REQUIRE(ebcRes.body.find("\"TARGET\":\"BUS\"") != std::string::npos);
  REQUIRE(ebcRes.body.find("\"TOPIC\":\"orders.created\"") != std::string::npos);

  const std::optional<Event> ebcEvent = ebcFixture.bus()->receive();
  REQUIRE(ebcEvent.has_value());
  REQUIRE(std::string(ebcEvent->topic()) == "orders.created");
  REQUIRE(std::string(ebcEvent->type()) == "OrderCreated");
  REQUIRE(ebcEvent->priority() == EventPriority::High);
  REQUIRE(ebcEvent->contentLength() == 2);
}

TEST_CASE("EventBusController: publishOfPost broadcasts to the subscribers")
{
  EventBusControllerFixture ebcFixture;
  EventBusController &ebcController = *ebcFixture.m_controller;
  REQUIRE(ebcFixture.bus()->subscribe("orders.*", "worker"));
  auto ebcReq = ebcRequestWithMatches({ "orders" });
  ebcReq.body = R"({"TOPIC":"orders.created","TYPE":"OrderCreated","TARGET":"SUBSCRIBERS"})";
  httplib::Response ebcRes;

  ebcController.publishOfPost(ebcReq, ebcRes, ebcJson(), ebcTokenOf(ebcUser));

  REQUIRE(ebcRes.body.find("\"PUBLISHED\":true") != std::string::npos);
  REQUIRE(ebcRes.body.find("\"TARGET\":\"SUBSCRIBERS\"") != std::string::npos);
  const std::optional<Event> ebcEvent = ebcFixture.bus()->receiveOf("worker");
  REQUIRE(ebcEvent.has_value());
  REQUIRE(std::string(ebcEvent->topic()) == "orders.created");
  REQUIRE(ebcEvent->priority() == EventPriority::Normal);
}

TEST_CASE("EventBusController: publishOfPost rejects a topic that does not fit into an event")
{
  EventBusControllerFixture ebcFixture;
  EventBusController &ebcController = *ebcFixture.m_controller;
  auto ebcReq = ebcRequestWithMatches({ "orders" });
  ebcReq.body = std::string(R"({"TOPIC":")") + std::string(Event::NAME_SIZE, 'x') + R"(","TYPE":"T"})";
  httplib::Response ebcRes;

  ebcController.publishOfPost(ebcReq, ebcRes, ebcJson(), ebcTokenOf(ebcUser));
  REQUIRE(ebcRes.status == 400);
}

TEST_CASE("EventBusController: publishOfPost rejects an unknown priority")
{
  EventBusControllerFixture ebcFixture;
  EventBusController &ebcController = *ebcFixture.m_controller;
  auto ebcReq = ebcRequestWithMatches({ "orders" });
  ebcReq.body = R"({"TOPIC":"orders.created","TYPE":"T","PRIORITY":"urgent"})";
  httplib::Response ebcRes;

  ebcController.publishOfPost(ebcReq, ebcRes, ebcJson(), ebcTokenOf(ebcUser));
  REQUIRE(ebcRes.status == 400);
}

TEST_CASE("EventBusController: publishOfPost returns 404 for an unknown bus")
{
  EventBusControllerFixture ebcFixture;
  EventBusController &ebcController = *ebcFixture.m_controller;
  auto ebcReq = ebcRequestWithMatches({ "unknown" });
  ebcReq.body = ebcPublishBody;
  httplib::Response ebcRes;

  ebcController.publishOfPost(ebcReq, ebcRes, ebcJson(), ebcTokenOf(ebcUser));
  REQUIRE(ebcRes.status == 404);
}

TEST_CASE("EventBusController: subscribing needs the admin group")
{
  EventBusControllerFixture ebcFixture;
  EventBusController &ebcController = *ebcFixture.m_controller;
  auto ebcReq = ebcRequestWithMatches({ "orders" });
  ebcReq.body = R"({"TOPIC":"orders.created","SUBSCRIBER":"worker"})";
  httplib::Response ebcRes;

  ebcController.subscribeOfPost(ebcReq, ebcRes, ebcJson(), ebcTokenOf(ebcUser));
  REQUIRE(ebcRes.status == 401);
}

TEST_CASE("EventBusController: subscribe and unsubscribe change the registry")
{
  EventBusControllerFixture ebcFixture;
  EventBusController &ebcController = *ebcFixture.m_controller;
  auto ebcReq = ebcRequestWithMatches({ "orders" });
  ebcReq.body = R"({"TOPIC":"orders.created","SUBSCRIBER":"worker"})";
  httplib::Response ebcRes;

  ebcController.subscribeOfPost(ebcReq, ebcRes, ebcJson(), ebcTokenOf(ebcAdmin));
  REQUIRE(ebcRes.body.find("\"SUBSCRIBED\":true") != std::string::npos);
  REQUIRE(ebcFixture.bus()->isSubscribed("orders.created", "worker"));
  // an exact topic is registered implicitly by the subscribe route
  REQUIRE(ebcFixture.bus()->isTopicRegistered("orders.created"));

  auto ebcDeleteReq = ebcRequestWithMatches({ "orders" });
  ebcDeleteReq.body = ebcReq.body;
  httplib::Response ebcDeleteRes;
  ebcController.unsubscribeOfDelete(ebcDeleteReq, ebcDeleteRes, ebcJson(), ebcTokenOf(ebcAdmin));
  REQUIRE(ebcDeleteRes.body.find("\"UNSUBSCRIBED\":true") != std::string::npos);
  REQUIRE(!ebcFixture.bus()->isSubscribed("orders.created", "worker"));
}

TEST_CASE("EventBusController: depthOfGet reports the subscriber queue depth")
{
  EventBusControllerFixture ebcFixture;
  EventBusController &ebcController = *ebcFixture.m_controller;
  REQUIRE(ebcFixture.bus()->subscribe("orders.*", "worker"));
  Event ebcEvent("orders.created", "OrderCreated");
  REQUIRE(ebcFixture.bus()->publishToSubscribers(ebcEvent));
  REQUIRE(ebcFixture.bus()->publishToSubscribers(ebcEvent));

  auto ebcReq = ebcRequestWithMatches({ "orders", "worker" });
  httplib::Response ebcRes;
  ebcController.depthOfGet(ebcReq, ebcRes, ebcJson(), ebcTokenOf(ebcUser));

  REQUIRE(ebcRes.body.find("\"DEPTH\":2") != std::string::npos);
  REQUIRE(ebcRes.body.find("\"SUBSCRIBER\":\"worker\"") != std::string::npos);
}

TEST_CASE("EventBusController: depthOfGet returns 404 for an unknown bus")
{
  EventBusControllerFixture ebcFixture;
  EventBusController &ebcController = *ebcFixture.m_controller;
  auto ebcReq = ebcRequestWithMatches({ "unknown", "worker" });
  httplib::Response ebcRes;

  ebcController.depthOfGet(ebcReq, ebcRes, ebcJson(), ebcTokenOf(ebcUser));
  REQUIRE(ebcRes.status == 404);
}

TEST_CASE("EventBusController: resetting needs the admin group")
{
  EventBusControllerFixture ebcFixture;
  EventBusController &ebcController = *ebcFixture.m_controller;
  auto ebcReq = ebcRequestWithMatches({ "orders" });
  httplib::Response ebcRes;

  ebcController.resetOfPost(ebcReq, ebcRes, ebcJson(), ebcTokenOf(ebcUser));
  REQUIRE(ebcRes.status == 401);
}

TEST_CASE("EventBusController: resetOfPost drops the queued events")
{
  EventBusControllerFixture ebcFixture;
  EventBusController &ebcController = *ebcFixture.m_controller;
  Event ebcEvent("orders.created", "OrderCreated");
  REQUIRE(ebcFixture.bus()->publish(ebcEvent));

  auto ebcReq = ebcRequestWithMatches({ "orders" });
  httplib::Response ebcRes;
  ebcController.resetOfPost(ebcReq, ebcRes, ebcJson(), ebcTokenOf(ebcAdmin));

  REQUIRE(ebcRes.body.find("\"RESET\":true") != std::string::npos);
  REQUIRE(ebcFixture.bus()->isEmpty());
}
}// namespace lintel
