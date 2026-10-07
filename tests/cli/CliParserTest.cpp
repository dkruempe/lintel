#include <lintel/features/cli/models/CommandParser.h>
#include <lintel/features/cli/models/Command.h>

#include <catch2/catch_all.hpp>

#include <sstream>

enum class TestCommand { NONE = 0, HELP, EXIT, STATUS, START };

TEST_CASE("CommandParser: add and parse known command") {
    CommandParser<TestCommand, TestCommand::NONE> parser;

    Command helpCmd("help", "Shows help");
    parser.addCommand(helpCmd, TestCommand::HELP);

    auto result = parser.parse("help", {});
    REQUIRE(result == TestCommand::HELP);
}

TEST_CASE("CommandParser: parse unknown command returns undefined") {
    CommandParser<TestCommand, TestCommand::NONE> parser;

    Command helpCmd("help", "Shows help");
    parser.addCommand(helpCmd, TestCommand::HELP);

    auto result = parser.parse("unknown", {});
    REQUIRE(result == TestCommand::NONE);
}

TEST_CASE("CommandParser: add multiple commands") {
    CommandParser<TestCommand, TestCommand::NONE> parser;

    parser.addCommand(Command("help", "Shows help"), TestCommand::HELP);
    parser.addCommand(Command("exit", "Exits"), TestCommand::EXIT);
    parser.addCommand(Command("status", "Shows status"), TestCommand::STATUS);

    REQUIRE(parser.parse("help", {}) == TestCommand::HELP);
    REQUIRE(parser.parse("exit", {}) == TestCommand::EXIT);
    REQUIRE(parser.parse("status", {}) == TestCommand::STATUS);
}

TEST_CASE("CommandParser: allCommandsOf returns registered commands") {
    CommandParser<TestCommand, TestCommand::NONE> parser;

    parser.addCommand(Command("help", "Shows help"), TestCommand::HELP);
    parser.addCommand(Command("start", "Starts"), TestCommand::START);

    auto commands = parser.allCommandsOf();
    REQUIRE(commands.size() == 2);
    REQUIRE(std::find(commands.begin(), commands.end(), "help") != commands.end());
    REQUIRE(std::find(commands.begin(), commands.end(), "start") != commands.end());
}

TEST_CASE("CommandParser: printHelp for known command") {
    CommandParser<TestCommand, TestCommand::NONE> parser;

    parser.addCommand(Command("help", "Shows help"), TestCommand::HELP);

    std::ostringstream oss;
    parser.printHelp("help", oss);
    REQUIRE(oss.str().find("Shows help") != std::string::npos);
}

TEST_CASE("CommandParser: printHelp for unknown command prints error") {
    CommandParser<TestCommand, TestCommand::NONE> parser;

    std::ostringstream oss;
    parser.printHelp("nonexistent", oss);
    REQUIRE(oss.str().find("ERROR") != std::string::npos);
}

TEST_CASE("CommandParser: printCommandList") {
    CommandParser<TestCommand, TestCommand::NONE> parser;

    parser.addCommand(Command("help", "Shows help"), TestCommand::HELP);
    parser.addCommand(Command("exit", "Exits"), TestCommand::EXIT);

    std::ostringstream oss;
    std::set<std::string> aliases;
    parser.printCommandList(aliases, oss);
    std::string output = oss.str();
    REQUIRE(output.find("help") != std::string::npos);
    REQUIRE(output.find("exit") != std::string::npos);
}

TEST_CASE("CommandParser: addCommand with rvalue reference") {
    CommandParser<TestCommand, TestCommand::NONE> parser;

    parser.addCommand(Command("start", "Starts the service"), TestCommand::START);

    auto result = parser.parse("start", {});
    REQUIRE(result == TestCommand::START);
}

TEST_CASE("CommandParser: parse with flags") {
    CommandParser<TestCommand, TestCommand::NONE> parser;

    bool verbose = false;
    std::string config = "";
    Command startCmd("start", "Starts the service");
    startCmd.addArgument({"--verbose", "-v"}, &verbose, "Verbose output");
    startCmd.addArgument({"--config", "-c"}, &config, "Config file");
    parser.addCommand(startCmd, TestCommand::START);

    auto result = parser.parse("start", {"--verbose", "--config", "/etc/cfg.xml"});
    REQUIRE(result == TestCommand::START);
    REQUIRE(verbose);
    REQUIRE(config == "/etc/cfg.xml");
}

TEST_CASE("CommandParser: parse with bool flag") {
    CommandParser<TestCommand, TestCommand::NONE> parser;

    bool verbose = false;
    Command cmd("status", "Shows status");
    cmd.addArgument({"--verbose", "-v"}, &verbose, "Verbose");
    parser.addCommand(cmd, TestCommand::STATUS);

    parser.parse("status", {"--verbose"});
    REQUIRE(verbose);
}

TEST_CASE("CommandParser: parse with int flag") {
    CommandParser<TestCommand, TestCommand::NONE> parser;

    int32_t count = 0;
    Command cmd("run", "Runs task");
    cmd.addArgument({"--count", "-c"}, &count, "Count");
    parser.addCommand(cmd, TestCommand::START);

    parser.parse("run", {"--count", "42"});
    REQUIRE(count == 42);
}

TEST_CASE("CommandParser: parse with equals syntax") {
    CommandParser<TestCommand, TestCommand::NONE> parser;

    std::string name = "";
    Command cmd("greet", "Greets");
    cmd.addArgument({"--name", "-n"}, &name, "Name");
    parser.addCommand(cmd, TestCommand::START);

    parser.parse("greet", {"--name=World"});
    REQUIRE(name == "World");
}

TEST_CASE("Command: getCommand") {
    Command cmd("mycommand", "My description");
    REQUIRE(cmd.getCommand() == "mycommand");
}

TEST_CASE("Command: addArgument returns reference for chaining") {
    Command cmd("test", "Test command");
    bool flag1 = false;
    bool flag2 = false;
    const auto &ref = cmd.addArgument({"-a"}, &flag1, "Flag A")
                   .addArgument({"-b"}, &flag2, "Flag B");
    REQUIRE(&ref == &cmd);
}

TEST_CASE("Command: printHelp contains command and description") {
    Command cmd("mycommand", "My test command");
    std::ostringstream oss;
    cmd.printHelp(oss);
    std::string output = oss.str();
    REQUIRE(output.find("mycommand") != std::string::npos);
    REQUIRE(output.find("My test command") != std::string::npos);
}

TEST_CASE("Command: parse with optional flag missing") {
    CommandParser<TestCommand, TestCommand::NONE> parser;

    std::string logFile = "default.log";
    Command cmd("run", "Runs");
    cmd.addArgument({"--log", "-l"}, &logFile, "Log file");
    parser.addCommand(cmd, TestCommand::START);

    parser.parse("run", {});
    REQUIRE(logFile == "default.log");
}

TEST_CASE("Command: parse with multiple flags same command") {
    CommandParser<TestCommand, TestCommand::NONE> parser;

    bool a = false;
    bool b = false;
    Command cmd("test", "Test");
    cmd.addArgument({"-a"}, &a, "Flag A");
    cmd.addArgument({"-b"}, &b, "Flag B");
    parser.addCommand(cmd, TestCommand::START);

    parser.parse("test", {"-a", "-b"});
    REQUIRE(a);
    REQUIRE(b);
}
