#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>

#include <deque>
#include <memory>

#include "../mocks/MockUserApi.h"
#include "lintel/features/cli/services/AuthCliService.h"
#include "lintel/features/cli/services/IInputService.h"
#include "lintel/features/cli/services/ITerminalService.h"
#include "lintel/features/cli/utils/CommandLineUtils.h"

using namespace trompeloeil;

namespace {

class StubInputService : public IInputService
{
    std::deque<KeyEvent> m_keys;
public:
    void push(KeyEvent key) { m_keys.push_back(key); }
    void pushText(const std::string &text)
    {
        for (char c : text) { m_keys.emplace_back(KeyType::Ascii, c); }
    }

    KeyEvent onRead() override
    {
        REQUIRE_FALSE(m_keys.empty());
        auto key = m_keys.front();
        m_keys.pop_front();
        return key;
    }
};

class StubTerminalService : public ITerminalService
{
    std::string m_currentLine;
    std::size_t m_position = 0;

public:
    void log(const std::string &) override {}

    SymbolEvent onKeyPressed(KeyEvent key, std::string) override
    {
        switch (key.first) {
        case KeyType::Ascii: {
            if (key.second == '\t') { return {Symbol::Tab, m_currentLine}; }
            m_currentLine.insert(m_currentLine.begin() + static_cast<std::string::difference_type>(m_position), key.second);
            ++m_position;
            return {Symbol::Nothing, {}};
        }
        case KeyType::Ret: {
            auto cmd = m_currentLine;
            m_currentLine.clear();
            m_position = 0;
            return {Symbol::Command, cmd};
        }
        case KeyType::Backspace:
            if (m_position > 0) {
                --m_position;
                m_currentLine.erase(m_currentLine.begin() + static_cast<std::string::difference_type>(m_position));
            }
            return {Symbol::Nothing, {}};
        case KeyType::CtrlC:
            return {Symbol::CtrlC, {}};
        case KeyType::Eof:
            return {Symbol::Eof, {}};
        case KeyType::CtrlR:
            return {Symbol::CtrlR, {}};
        default:
            return {Symbol::Nothing, {}};
        }
    }

    void resetCursor() override { m_currentLine.clear(); m_position = 0; }
    const std::string &getLine() override { return m_currentLine; }
    void enableHideChars() override {}
    void disableHideChars() override {}
};

} // anonymous namespace

TEST_CASE("AuthCliService: onLogin returns nullopt on Ctrl+C during username")
{
    auto input = std::make_shared<StubInputService>();
    auto terminal = std::make_shared<StubTerminalService>();
    auto mockUserApi = std::make_shared<MockUserApi>();
    auto utils = std::make_shared<CommandLineUtils>();
    auto argProvider = std::make_shared<AuthArgumentProvider>();

    input->push({KeyType::CtrlC, ' '});

    AuthCliService service(mockUserApi, utils, input, terminal, argProvider);
    auto result = service.onLogin();
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("AuthCliService: onLogin returns nullopt on Eof during username")
{
    auto input = std::make_shared<StubInputService>();
    auto terminal = std::make_shared<StubTerminalService>();
    auto mockUserApi = std::make_shared<MockUserApi>();
    auto utils = std::make_shared<CommandLineUtils>();
    auto argProvider = std::make_shared<AuthArgumentProvider>();

    input->push({KeyType::Eof, ' '});

    AuthCliService service(mockUserApi, utils, input, terminal, argProvider);
    auto result = service.onLogin();
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("AuthCliService: onLogin returns UserDto on successful login")
{
    auto input = std::make_shared<StubInputService>();
    auto terminal = std::make_shared<StubTerminalService>();
    auto mockUserApi = std::make_shared<MockUserApi>();
    auto utils = std::make_shared<CommandLineUtils>();
    auto argProvider = std::make_shared<AuthArgumentProvider>();

    input->pushText("testuser");
    input->push({KeyType::Ret, '\n'});
    input->pushText("secret");
    input->push({KeyType::Ret, '\n'});

    REQUIRE_CALL(*mockUserApi, loginOf(trompeloeil::_))
        .TIMES(1)
        .LR_RETURN(std::make_optional<UserDto>());

    AuthCliService service(mockUserApi, utils, input, terminal, argProvider);
    auto result = service.onLogin();
    REQUIRE(result.has_value());
}

TEST_CASE("AuthCliService: onLogin retries on empty password")
{
    auto input = std::make_shared<StubInputService>();
    auto terminal = std::make_shared<StubTerminalService>();
    auto mockUserApi = std::make_shared<MockUserApi>();
    auto utils = std::make_shared<CommandLineUtils>();
    auto argProvider = std::make_shared<AuthArgumentProvider>();

    // first readUserName (optUserName is nullopt, so userName is cleared each iteration)
    input->pushText("testuser");
    input->push({KeyType::Ret, '\n'});
    // empty password → continue → userName gets cleared → readUserName again
    input->push({KeyType::Ret, '\n'});
    // second readUserName + readPassword
    input->pushText("testuser");
    input->push({KeyType::Ret, '\n'});
    input->pushText("secret");
    input->push({KeyType::Ret, '\n'});

    REQUIRE_CALL(*mockUserApi, loginOf(trompeloeil::_))
        .TIMES(1)
        .LR_RETURN(std::make_optional<UserDto>());

    AuthCliService service(mockUserApi, utils, input, terminal, argProvider);
    auto result = service.onLogin();
    REQUIRE(result.has_value());
}

TEST_CASE("AuthCliService: onLogin retries on failed login")
{
    auto input = std::make_shared<StubInputService>();
    auto terminal = std::make_shared<StubTerminalService>();
    auto mockUserApi = std::make_shared<MockUserApi>();
    auto utils = std::make_shared<CommandLineUtils>();
    auto argProvider = std::make_shared<AuthArgumentProvider>();

    int loginCallCount = 0;

    // first attempt: username + wrong password
    input->pushText("testuser");
    input->push({KeyType::Ret, '\n'});
    input->pushText("wrongpass");
    input->push({KeyType::Ret, '\n'});
    // second attempt: username + correct password
    input->pushText("testuser");
    input->push({KeyType::Ret, '\n'});
    input->pushText("secret");
    input->push({KeyType::Ret, '\n'});

    REQUIRE_CALL(*mockUserApi, loginOf(trompeloeil::_))
        .TIMES(2)
        .LR_SIDE_EFFECT(++loginCallCount)
        .LR_RETURN(loginCallCount == 1 ? std::optional<UserDto>() : std::make_optional<UserDto>());

    AuthCliService service(mockUserApi, utils, input, terminal, argProvider);
    auto result = service.onLogin();
    REQUIRE(result.has_value());
}
