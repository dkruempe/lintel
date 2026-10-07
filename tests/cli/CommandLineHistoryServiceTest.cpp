#include <catch2/catch_all.hpp>

#include <chrono>
#include <filesystem>
#include <string>

#include "lintel/config.h"
#include "lintel/core/services/StringifyService.h"
#include "lintel/features/base/models/ProcessName.h"
#include "lintel/features/cli/models/CommandHistoryEntry.h"
#include "lintel/features/cli/services/CommandLineHistoryService.h"
#include "lintel/features/property/models/Property.h"

namespace {

const char *const TEST_HISTORY_FILE = ".commandline_history_test.history";

// date::sys_time<Duration> is defined as std::chrono::time_point<std::chrono::system_clock, Duration>;
// spelling it out keeps this helper free of the third-party namespace.
using MicrosecondSysTime = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;

MicrosecondSysTime now()
{
    return std::chrono::time_point_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now());
}

/** Removes the test history file at program start and exit */
struct HistoryFileCleanup {
    HistoryFileCleanup()
    {
        std::filesystem::remove(std::filesystem::path(CONFIG_DIRECTORY) / TEST_HISTORY_FILE);
    }
    ~HistoryFileCleanup()
    {
        std::filesystem::remove(std::filesystem::path(CONFIG_DIRECTORY) / TEST_HISTORY_FILE);
    }
} g_historyFileCleanup;

/** Test helper that allows redirecting the history file away from the real config */
class TestCommandLineHistoryService : public CommandLineHistoryService {
public:
    using CommandLineHistoryService::CommandLineHistoryService;

    void setHistoryFileName(const std::string &fileName)
    {
        for (auto &property : m_properties) {
            if (property->getName() == "m_historyFileName") {
                std::static_pointer_cast<Property<std::string>>(property)
                        ->setValue(fileName);
                return;
            }
        }
        FAIL("m_historyFileName property not registered");
    }

    void addEntries(const std::vector<std::string> &commands, const std::string &menu)
    {
        for (const auto &command : commands) {
            historizeOf(CommandHistoryEntry(command, "tester", now(), menu));
        }
    }
};

}  // namespace

TEST_CASE("CommandLineHistoryService: previousOf returns newest first and clamps at start")
{
    auto processName = std::make_shared<ProcessName>("test-process");
    TestCommandLineHistoryService service(processName);
    service.setHistoryFileName(TEST_HISTORY_FILE);

    service.addEntries({"cmd1", "cmd2", "cmd3"}, "Root");

    REQUIRE(service.previousOf("Root")->getCommand() == "cmd3");
    REQUIRE(service.previousOf("Root")->getCommand() == "cmd2");
    REQUIRE(service.previousOf("Root")->getCommand() == "cmd1");
    REQUIRE_FALSE(service.previousOf("Root").has_value());
    REQUIRE_FALSE(service.previousOf("Root").has_value());
}

TEST_CASE("CommandLineHistoryService: nextOf returns oldest first and clamps at end")
{
    auto processName = std::make_shared<ProcessName>("test-process");
    TestCommandLineHistoryService service(processName);
    service.setHistoryFileName(TEST_HISTORY_FILE);

    service.addEntries({"cmd1", "cmd2", "cmd3"}, "Root");

    REQUIRE(service.nextOf("Root")->getCommand() == "cmd1");
    REQUIRE(service.nextOf("Root")->getCommand() == "cmd2");
    REQUIRE(service.nextOf("Root")->getCommand() == "cmd3");
    REQUIRE_FALSE(service.nextOf("Root").has_value());
    REQUIRE_FALSE(service.nextOf("Root").has_value());
}

TEST_CASE("CommandLineHistoryService: down past the end then up must not access out of bounds")
{
    auto processName = std::make_shared<ProcessName>("test-process");
    TestCommandLineHistoryService service(processName);
    service.setHistoryFileName(TEST_HISTORY_FILE);

    service.addEntries({"cmd1", "cmd2", "cmd3"}, "Root");

    // pressing Down repeatedly past the end used to grow the position unboundedly
    for (int i = 0; i < 10; i++) {
        service.nextOf("Root");
    }

    // pressing Up afterwards must still return the newest entry instead of
    // reading out of bounds (previously caused a bad_alloc)
    for (int i = 0; i < 20; i++) {
        auto previous = service.previousOf("Root");
        if (previous.has_value()) {
            REQUIRE((previous->getCommand() == "cmd3" || previous->getCommand() == "cmd2" ||
                     previous->getCommand() == "cmd1"));
        }
        auto next = service.nextOf("Root");
        if (next.has_value()) {
            REQUIRE((next->getCommand() == "cmd1" || next->getCommand() == "cmd2" ||
                     next->getCommand() == "cmd3"));
        }
    }
}

TEST_CASE("CommandLineHistoryService: up and down alternate without invalid position")
{
    auto processName = std::make_shared<ProcessName>("test-process");
    TestCommandLineHistoryService service(processName);
    service.setHistoryFileName(TEST_HISTORY_FILE);

    service.addEntries({"cmd1", "cmd2", "cmd3"}, "Root");

    for (int i = 0; i < 10; i++) {
        auto up = service.previousOf("Root");
        auto down = service.nextOf("Root");
        auto upAgain = service.previousOf("Root");
        const bool validUp = !up.has_value() || up->getCommand() == "cmd1" ||
                             up->getCommand() == "cmd2" || up->getCommand() == "cmd3";
        const bool validDown = !down.has_value() || down->getCommand() == "cmd1" ||
                               down->getCommand() == "cmd2" || down->getCommand() == "cmd3";
        const bool validUpAgain = !upAgain.has_value() || upAgain->getCommand() == "cmd1" ||
                                  upAgain->getCommand() == "cmd2" ||
                                  upAgain->getCommand() == "cmd3";
        REQUIRE(validUp);
        REQUIRE(validDown);
        REQUIRE(validUpAgain);
    }
}

TEST_CASE("CommandLineHistoryService: menu entries are filtered")
{
    auto processName = std::make_shared<ProcessName>("test-process");
    TestCommandLineHistoryService service(processName);
    service.setHistoryFileName(TEST_HISTORY_FILE);

    service.addEntries({"root_cmd1"}, "Root");
    service.addEntries({"user_cmd"}, "UserManagement");
    service.addEntries({"root_cmd2"}, "Root");

    REQUIRE(service.previousOf("Root")->getCommand() == "root_cmd2");
    REQUIRE(service.previousOf("Root")->getCommand() == "root_cmd1");
    REQUIRE_FALSE(service.previousOf("Root").has_value());

    REQUIRE(service.nextOf("Root")->getCommand() == "root_cmd1");
    REQUIRE(service.nextOf("Root")->getCommand() == "root_cmd2");
    REQUIRE_FALSE(service.nextOf("Root").has_value());
}

TEST_CASE("CommandLineHistoryService: startsWith finds the newest matching entry")
{
    auto processName = std::make_shared<ProcessName>("test-process");
    TestCommandLineHistoryService service(processName);
    service.setHistoryFileName(TEST_HISTORY_FILE);

    service.addEntries({"user_add", "show_users", "user_delete"}, "Root");
    service.addEntries({"user_sessions"}, "UserManagement");

    REQUIRE(service.startsWith("user", "Root")->getCommand() == "user_delete");
    REQUIRE(service.startsWith("user", "UserManagement")->getCommand() == "user_sessions");
    REQUIRE_FALSE(service.startsWith("unknown", "Root").has_value());
}

TEST_CASE("CommandLineHistoryService: startsWith matches the oldest single entry")
{
    auto processName = std::make_shared<ProcessName>("test-process");
    TestCommandLineHistoryService service(processName);
    service.setHistoryFileName(TEST_HISTORY_FILE);

    service.addEntries({"abc"}, "Root");

    REQUIRE(service.startsWith("ab", "Root")->getCommand() == "abc");
}
