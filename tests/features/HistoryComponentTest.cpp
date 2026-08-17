#include <base_library/features/base/configuration/HistoryComponent.h>
#include <base_library/features/base/configuration/HistoryServiceEntry.h>
#include <base_library/features/base/configuration/ConfigurationException.h>
#include <base_library/core/utils/TypeName.h>

#include <catch2/catch_all.hpp>

#include <cstdlib>

TEST_CASE("HistoryComponent: parse single history service") {
    HistoryComponent component;

    std::string xml = R"(<HistoryServices>
        <HistoryService process_name="main" queue="history" max_messages="1000"/>
    </HistoryServices>)";

    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.size() == 1);

    auto entry = std::static_pointer_cast<HistoryServiceEntry>(entries[0]);
    REQUIRE(entry->get_process_name() == "main");
    REQUIRE(entry->get_queue_name() == "history");
    REQUIRE(entry->get_max_messages() == 1000);
    REQUIRE(entry->getConfigurationParserComponent() == type_name<HistoryComponent>());
}

TEST_CASE("HistoryComponent: defaults for omitted queue and max_messages") {
    HistoryComponent component;

    std::string xml = R"(<HistoryServices>
        <HistoryService process_name="worker"/>
    </HistoryServices>)";

    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.size() == 1);

    auto entry = std::static_pointer_cast<HistoryServiceEntry>(entries[0]);
    REQUIRE(entry->get_process_name() == "worker");
    REQUIRE(entry->get_queue_name() == "history");
    REQUIRE(entry->get_max_messages() == 1000);
}

TEST_CASE("HistoryComponent: parse multiple history services") {
    HistoryComponent component;

    std::string xml = R"(<HistoryServices>
        <HistoryService process_name="main" queue="history" max_messages="500"/>
        <HistoryService process_name="other" queue="other_queue" max_messages="10"/>
    </HistoryServices>)";

    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.size() == 2);

    auto first = std::static_pointer_cast<HistoryServiceEntry>(entries[0]);
    auto second = std::static_pointer_cast<HistoryServiceEntry>(entries[1]);
    REQUIRE(first->get_process_name() == "main");
    REQUIRE(first->get_queue_name() == "history");
    REQUIRE(first->get_max_messages() == 500);
    REQUIRE(second->get_process_name() == "other");
    REQUIRE(second->get_queue_name() == "other_queue");
    REQUIRE(second->get_max_messages() == 10);
}

TEST_CASE("HistoryComponent: empty HistoryServices returns empty") {
    HistoryComponent component;
    std::string xml = R"(<HistoryServices></HistoryServices>)";
    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.empty());
}

TEST_CASE("HistoryComponent: missing HistoryServices returns empty") {
    HistoryComponent component;
    std::string xml = R"(<Unrelated></Unrelated>)";
    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.empty());
}

TEST_CASE("HistoryComponent: throw on missing process_name") {
    HistoryComponent component;
    std::string xml = R"(<HistoryServices>
        <HistoryService/>
    </HistoryServices>)";
    REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}
