#include <filesystem>
#include <string>
#include <type_traits>

#include <boost/interprocess/managed_mapped_file.hpp>

#include <catch2/catch_all.hpp>

#include "base_library/core/services/SharedMemoryService.h"
#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/configuration/EnvironmentConfiguration.h"
#include "base_library/features/base/configuration/SharedMemorySegmentComponent.h"
#include "base_library/features/base/configuration/SharedMemorySegmentEntry.h"
#include "base_library/features/base/events/Event.h"
#include "base_library/features/base/events/EventBus.h"
#include "base_library/features/base/models/ProcessName.h"
#include "base_library/features/base/services/SchedulerService.h"
#include "base_library/features/base/services/SharedMemorySegmentManager.h"
#include "base_library/features/property/configuration/PropertyRepositoryComponent.h"
#include "base_library/features/property/configuration/PropertyRepositoryEntry.h"
#include "base_library/features/property/events/PropertyChange.h"
#include "base_library/features/property/models/Property.h"
#include "base_library/features/property/models/PropertyValueStorage.h"
#include "base_library/features/property/repositories/SharedMemoryPropertyRepository.h"
#include "base_library/features/property/services/PropertyService.h"

namespace {

using Segment = boost::interprocess::managed_mapped_file;

std::filesystem::path uniqueShmPath() {
    static std::uint64_t counter = 0;
    return std::filesystem::temp_directory_path() /
           ("property_change_test_" + std::to_string(counter++) + ".bin");
}

struct BusFixture {
    EventBusConfig config;
    std::filesystem::path path;
    std::size_t size;
    Segment segment;
    EventBus *bus;

    explicit BusFixture(EventBusConfig fixtureConfig = EventBusConfig{})
        : config(fixtureConfig),
          path(uniqueShmPath()),
          size(EventBus::requiredSize(config) + 4096u),
          segment(boost::interprocess::open_or_create, path.c_str(), size) {
        bus = segment.find_or_construct<EventBus>("bus")(
                "bus", config, segment.get_segment_manager());
    }

    ~BusFixture() {
        segment.destroy_ptr(bus);
        segment.flush();
        std::filesystem::remove(path);
    }
};

/** Wires a real shared memory property repository over a unique segment. */
struct PropertyRepositoryFixture {
    std::shared_ptr<Configuration> configuration;
    std::shared_ptr<ProcessName> processName;
    std::shared_ptr<SchedulerService> scheduler;
    std::shared_ptr<SharedMemorySegmentManager> segmentManager;
    std::shared_ptr<SharedMemoryService> sharedMemoryService;
    std::shared_ptr<SharedMemoryPropertyRepository> repository;
    std::filesystem::path path;

    PropertyRepositoryFixture()
        : path(uniqueShmPath()) {
        auto environmentConfiguration =
                std::make_shared<EnvironmentConfiguration>();
        environmentConfiguration->overrides(
                EnvironmentConfiguration::ConfigDirectory,
                std::filesystem::temp_directory_path().string() + "/nonexistent");
        configuration = std::make_shared<Configuration>(
                std::vector<std::shared_ptr<Component>>{},
                std::move(environmentConfiguration));
        configuration->setEntries({
                std::make_shared<SharedMemorySegmentEntry>(
                        type_name<SharedMemorySegmentComponent>(),
                        std::make_shared<SharedMemorySegment>(
                                path, "shm_property", 5 * 1024 * 1024)),
                std::make_shared<PropertyRepositoryEntry>(
                        type_name<PropertyRepositoryComponent>(),
                        PropertyRepositoryType::SHM_REPOSITORY, true, false),
        });
        processName = std::make_shared<ProcessName>("testProcess");
        scheduler = std::make_shared<SchedulerService>(processName);
        segmentManager =
                std::make_shared<SharedMemorySegmentManager>(configuration);
        sharedMemoryService = std::make_shared<SharedMemoryService>(
                segmentManager, scheduler, processName);
        repository = std::make_shared<SharedMemoryPropertyRepository>(
                sharedMemoryService, segmentManager, configuration, processName);
    }
};

}  // namespace

TEST_CASE("property value storage is lock-free for POD types") {
    static_assert(IsLockFreeAtomic<int32_t>::value);
    static_assert(IsLockFreeAtomic<int64_t>::value);
    static_assert(IsLockFreeAtomic<double>::value);
    static_assert(IsLockFreeAtomic<bool>::value);
    static_assert(!IsLockFreeAtomic<std::string>::value);

    static_assert(PropertyValueStorage<int32_t>::isLockFree());
    static_assert(!PropertyValueStorage<std::string>::isLockFree());

    static_assert(Property<int32_t>::isLockFree());
    static_assert(Property<double>::isLockFree());
    static_assert(!Property<std::string>::isLockFree());

    Property<int32_t> property("value", "instance", "className", "processName",
                               7, "", true);
    REQUIRE(property.getValue() == 7);
    property.setValue(42);
    REQUIRE(property.getValue() == 42);
    REQUIRE(property.toString() == "42");

    Property<std::string> stringProperty("text", "instance", "className",
                                         "processName", "hello", "", true);
    REQUIRE(stringProperty.getValue() == "hello");
    stringProperty.setValue("world");
    REQUIRE(stringProperty.getValue() == "world");
}

TEST_CASE("property change payload fits the event content and round trips") {
    static_assert(std::is_trivially_copyable_v<PropertyChange>);
    static_assert(sizeof(PropertyChange) == Event::CONTENT_SIZE);

    Property<int32_t> property("target", "instance", "className", "main", 4711,
                               "", true);
    const PropertyChange change = PropertyChange::of(property);
    REQUIRE(std::string(change.name) == "target");
    REQUIRE(std::string(change.instanceName) == "instance");
    REQUIRE(std::string(change.className) == "className");
    REQUIRE(std::string(change.processName) == "main");
    REQUIRE(std::string(change.value) == "4711");
    REQUIRE_FALSE(change.m_valueTruncated);
    REQUIRE(change.identifier() == "target_instance_className_main");

    Event event("property_changes", "property_change");
    event.assign(change);
    const PropertyChange back = event.as<PropertyChange>();
    REQUIRE(back.identifier() == change.identifier());
    REQUIRE(std::string(back.value) == "4711");
    REQUIRE_FALSE(back.m_valueTruncated);
}

TEST_CASE("long string values are marked as truncated in the payload") {
    const std::string shortValue(PropertyChange::MAX_VALUE_LENGTH, 'a');
    Property<std::string> shortProperty("short", "instance", "className", "main",
                                        shortValue, "", true);
    const PropertyChange shortChange = PropertyChange::of(shortProperty);
    REQUIRE_FALSE(shortChange.m_valueTruncated);
    REQUIRE(std::string(shortChange.value) == shortValue);

    const std::string longValue(PropertyChange::MAX_VALUE_LENGTH + 1, 'x');
    Property<std::string> longProperty("long", "instance", "className", "main",
                                       longValue, "", true);
    const PropertyChange longChange = PropertyChange::of(longProperty);
    REQUIRE(longChange.m_valueTruncated);
    REQUIRE(std::string(longChange.value).size() ==
            PropertyChange::MAX_VALUE_LENGTH);
    REQUIRE(std::string(longChange.value) == longValue.substr(0,
            PropertyChange::MAX_VALUE_LENGTH));
}

TEST_CASE("property change is published and applied through the event bus") {
    BusFixture fixture;
    std::shared_ptr<IEventBus> busA =
            std::make_shared<EventBusView>(*fixture.bus);
    std::shared_ptr<IEventBus> busB =
            std::make_shared<EventBusView>(*fixture.bus);

    PropertyService serviceA({}, {});
    PropertyService serviceB({}, {});
    serviceA.setPropertyChangeBus(busA, "procA");
    serviceB.setPropertyChangeBus(busB, "procB");

    auto propertyA = serviceA.getOrCreate<int32_t>(
            "target", "instance", "className", "main", "", true, 0);
    auto propertyB = serviceB.getOrCreate<int32_t>(
            "target", "instance", "className", "main", "", true, 0);
    REQUIRE(propertyB->getValue() == 0);

    REQUIRE(serviceA.applyChangeNotifications() == 0);
    serviceA.changeValueOf<int32_t>(propertyA, 42);
    REQUIRE(serviceA.applyChangeNotifications() == 0);
    REQUIRE(serviceB.applyChangeNotifications() == 1);
    REQUIRE(propertyB->getValue() == 42);

    serviceA.changeStringValueOf(propertyA, "99");
    REQUIRE(serviceB.applyChangeNotifications() == 1);
    REQUIRE(propertyB->getValue() == 99);

    // unchanged value is not reported as an applied change
    serviceA.changeValueOf<int32_t>(propertyA, 99);
    REQUIRE(serviceB.applyChangeNotifications() == 0);
}

TEST_CASE("property change for an unknown property is ignored") {
    BusFixture fixture;
    std::shared_ptr<IEventBus> bus =
            std::make_shared<EventBusView>(*fixture.bus);

    PropertyService serviceA({}, {});
    PropertyService serviceB({}, {});
    serviceA.setPropertyChangeBus(bus, "procA");
    serviceB.setPropertyChangeBus(bus, "procB");

    auto propertyA = serviceA.getOrCreate<int32_t>(
            "target", "instance", "className", "main", "", true, 0);
    serviceA.changeValueOf<int32_t>(propertyA, 21);
    REQUIRE(serviceB.applyChangeNotifications() == 0);
    REQUIRE(serviceA.applyChangeNotifications() == 0);
}

TEST_CASE("no change bus configured means no notifications") {
    PropertyService serviceA({}, {});
    PropertyService serviceB({}, {});
    auto propertyA = serviceA.getOrCreate<int32_t>(
            "target", "instance", "className", "main", "", true, 0);
    auto propertyB = serviceB.getOrCreate<int32_t>(
            "target", "instance", "className", "main", "", true, 0);

    serviceA.changeValueOf<int32_t>(propertyA, 5);
    REQUIRE(serviceB.applyChangeNotifications() == 0);
    REQUIRE(propertyB->getValue() == 0);
    serviceA.changeStringValueOf(propertyA, "7");
    REQUIRE(propertyA->getValue() == 7);
}

TEST_CASE("truncated value is applied in full from shared memory") {
    BusFixture fixture;
    std::shared_ptr<IEventBus> busA =
            std::make_shared<EventBusView>(*fixture.bus);
    std::shared_ptr<IEventBus> busB =
            std::make_shared<EventBusView>(*fixture.bus);

    PropertyRepositoryFixture repositoryFixture;
    PropertyService serviceA({repositoryFixture.repository}, {});
    PropertyService serviceB({repositoryFixture.repository}, {});
    serviceA.setPropertyChangeBus(busA, "procA");
    serviceB.setPropertyChangeBus(busB, "procB");

    const std::string initialValue = "initial";
    const std::string longValue(PropertyChange::MAX_VALUE_LENGTH + 20, 'z');
    auto greetingA = serviceA.getOrCreate<std::string>(
            "greeting", "instance", "className", "main", "", true, initialValue);
    auto greetingB = serviceB.getOrCreate<std::string>(
            "greeting", "instance", "className", "main", "", true, initialValue);

    serviceA.changeStringValueOf(greetingA, longValue);
    REQUIRE(serviceB.applyChangeNotifications() == 1);
    REQUIRE(greetingB->getValue() == longValue);
    REQUIRE(greetingB->getValue().size() == longValue.size());
}

TEST_CASE("truncated change without shared memory repository is not applied") {
    BusFixture fixture;
    std::shared_ptr<IEventBus> busA =
            std::make_shared<EventBusView>(*fixture.bus);
    std::shared_ptr<IEventBus> busB =
            std::make_shared<EventBusView>(*fixture.bus);

    PropertyService serviceA({}, {});
    PropertyService serviceB({}, {});
    serviceA.setPropertyChangeBus(busA, "procA");
    serviceB.setPropertyChangeBus(busB, "procB");

    const std::string initialValue = "initial";
    const std::string longValue(PropertyChange::MAX_VALUE_LENGTH + 20, 'z');
    auto greetingA = serviceA.getOrCreate<std::string>(
            "greeting", "instance", "className", "main", "", true, initialValue);
    auto greetingB = serviceB.getOrCreate<std::string>(
            "greeting", "instance", "className", "main", "", true, initialValue);

    serviceA.changeStringValueOf(greetingA, longValue);
    REQUIRE(serviceB.applyChangeNotifications() == 0);
    REQUIRE(greetingB->getValue() == initialValue);
}
