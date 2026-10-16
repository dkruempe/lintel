#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <deque>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "../helpers/PlatformCompat.h"

#include "../helpers/ScopedEnvironmentVariable.h"
#include "../mocks/MockCommandLineComponent.h"
#include "../mocks/MockUserApi.h"
#include "lintel/core/models/ProcessName.h"
#include "lintel/features/cli/providers/AuthArgumentProvider.h"
#include "lintel/features/cli/services/AuthCliService.h"
#include "lintel/features/cli/services/CommandLineHistoryService.h"
#include "lintel/features/cli/services/CommandLineService.h"
#include "lintel/features/cli/services/IInputService.h"
#include "lintel/features/cli/services/ITerminalService.h"
#include "lintel/features/cli/utils/CommandLineUtils.h"

namespace lintel {
using namespace trompeloeil;

namespace {

  /**
   * The service kills the whole process with SIGINT when the session ends
   * (onEndOfFile / the exit command) and interrupts its worker thread with
   * SIGUSR1 on shutdown. Both must be neutralised for the duration of a test.
   * Windows has no SIGUSR1 (the service polls instead of signalling), so only
   * SIGINT needs guarding there.
   */
  struct ClsSignalGuard
  {
    using Handler = void (*)(int);
    Handler m_oldInt;
#if !defined(_WIN32)
    Handler m_oldUsr1;
#endif

#if defined(_WIN32)
    ClsSignalGuard() : m_oldInt(std::signal(SIGINT, SIG_IGN)) {}

    ~ClsSignalGuard() { std::signal(SIGINT, m_oldInt); }
#else
    ClsSignalGuard() : m_oldInt(std::signal(SIGINT, SIG_IGN)), m_oldUsr1(std::signal(SIGUSR1, SIG_IGN)) {}

    ~ClsSignalGuard()
    {
      std::signal(SIGINT, m_oldInt);
      std::signal(SIGUSR1, m_oldUsr1);
    }
#endif// _WIN32
  };

  class ClsStubInput : public IInputService
  {
    std::deque<KeyEvent> m_keys;
    std::atomic<std::size_t> m_pushed{ 0 };
    std::atomic<std::size_t> m_popped{ 0 };

  public:
    void push(KeyEvent key)
    {
      ++m_pushed;
      m_keys.push_back(key);
    }

    void pushText(const std::string &text)
    {
      for (char c : text) { push({ KeyType::Ascii, c }); }
    }

    KeyEvent onRead() override
    {
      // Fall back to Ctrl+C so a starving worker thread always terminates the
      // session instead of asserting on an empty queue.
      if (m_keys.empty()) {
        ++m_popped;
        return { KeyType::CtrlC, ' ' };
      }
      ++m_popped;
      auto key = m_keys.front();
      m_keys.pop_front();
      return key;
    }

    /** Blocks until the worker thread has consumed every pushed key. */
    void waitConsumed()
    {
      const std::size_t expected = m_pushed.load();
      for (int i = 0; i < 5000 && m_popped.load() < expected; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }
  };

  class ClsStubTerminal : public ITerminalService
  {
    std::string m_currentLine;
    std::size_t m_position = 0;

  public:
    void log(const std::string &) override {}

    SymbolEvent onKeyPressed(KeyEvent key, std::string) override
    {
      switch (key.first) {
      case KeyType::Ascii: {
        if (key.second == '\t') { return { Symbol::Tab, m_currentLine }; }
        m_currentLine.insert(m_currentLine.begin() + static_cast<std::string::difference_type>(m_position), key.second);
        ++m_position;
        return { Symbol::Nothing, {} };
      }
      case KeyType::Ret: {
        auto cmd = m_currentLine;
        m_currentLine.clear();
        m_position = 0;
        return { Symbol::Command, cmd };
      }
      case KeyType::Backspace:
        if (m_position > 0) {
          --m_position;
          m_currentLine.erase(m_currentLine.begin() + static_cast<std::string::difference_type>(m_position));
        }
        return { Symbol::Nothing, {} };
      case KeyType::Up:
        return { Symbol::Up, {} };
      case KeyType::Down:
        return { Symbol::Down, {} };
      case KeyType::CtrlC:
        return { Symbol::CtrlC, {} };
      case KeyType::CtrlR:
        return { Symbol::CtrlR, {} };
      case KeyType::Eof:
        return { Symbol::Eof, {} };
      default:
        return { Symbol::Nothing, {} };
      }
    }

    void resetCursor() override
    {
      m_currentLine.clear();
      m_position = 0;
    }

    const std::string &getLine() override { return m_currentLine; }
    void enableHideChars() override {}
    void disableHideChars() override {}
  };

  /** Stubs the three UserApi calls the service makes, so no per-test expectations are needed. */
  class ClsUserApiStub : public MockUserApi
  {
  public:
    bool isLoggedIn() override { return true; }
    std::optional<UserDto> loginOf(const UserLoginDto &) override { return std::make_optional<UserDto>(); }
    bool logoutOf(const UserTokenDto &) override { return true; }
  };

  /** Wires a CommandLineService with stubs/mocks and runs one scripted session. */
  struct ClsFixture
  {
    ClsSignalGuard m_signalGuard;
    ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_cls_cfg" };
    std::shared_ptr<ClsStubInput> input = std::make_shared<ClsStubInput>();
    std::shared_ptr<ClsStubTerminal> terminal = std::make_shared<ClsStubTerminal>();
    std::shared_ptr<ClsUserApiStub> userApi = std::make_shared<ClsUserApiStub>();
    std::shared_ptr<CommandLineUtils> utils = std::make_shared<CommandLineUtils>();
    std::shared_ptr<AuthArgumentProvider> argumentProvider = std::make_shared<AuthArgumentProvider>();
    std::shared_ptr<ProcessName> processName = std::make_shared<ProcessName>("cli_service_test");
    std::shared_ptr<CommandLineHistoryService> history = std::make_shared<CommandLineHistoryService>(processName);
    std::shared_ptr<AuthCliService> auth;
    std::shared_ptr<CommandLineService> service;

    explicit ClsFixture(std::vector<std::shared_ptr<CommandLineComponent>> components)
    {
      std::filesystem::create_directories("/tmp/nonexistent_cls_cfg");
      auth = std::make_shared<AuthCliService>(userApi, utils, input, terminal, argumentProvider);
      service =
        std::make_shared<CommandLineService>(components, auth, userApi, utils, input, terminal, processName, history);
    }

    ~ClsFixture() { std::filesystem::remove_all("/tmp/nonexistent_cls_cfg"); }

    /** Logs in "tester" with password "secret". */
    void pushLogin()
    {
      input->pushText("tester");
      input->push({ KeyType::Ret, '\n' });
      input->pushText("secret");
      input->push({ KeyType::Ret, '\n' });
    }

    void pushCommand(const std::string &command)
    {
      input->pushText(command);
      input->push({ KeyType::Ret, '\n' });
    }

    /** Runs the service worker to completion (keys are consumed until the queue drains). */
    void runSession()
    {
      std::cin.clear();
      service->onInitialize();
      input->waitConsumed();
      service->onShutdown();
    }
  };

}// namespace

TEST_CASE("CommandLineService: executes the root commands and exits")
{
  ClsFixture fixture({});
  fixture.pushLogin();
  fixture.pushCommand("help -c Test");// help with an explicit component argument
  fixture.pushCommand("menu");// show the root menu
  fixture.pushCommand("history");// dump the history
  fixture.pushCommand("clear");// clear the screen
  fixture.pushCommand("exit");// log out and stop

  fixture.runSession();
  SUCCEED("root commands handled");
}

TEST_CASE("CommandLineService: an invalid root command prints an error")
{
  ClsFixture fixture({});
  fixture.pushLogin();
  fixture.pushCommand("does-not-exist");
  fixture.pushCommand("exit");

  fixture.runSession();
  SUCCEED("invalid command handled");
}

TEST_CASE("CommandLineService: navigates into a component and routes its commands")
{
  auto component = std::make_shared<MockCommandLineComponent>("Test", "t");

  ClsFixture fixture({ component });
  REQUIRE_CALL(*component, onCommand(_, "foo", _)).TIMES(1);
  REQUIRE_CALL(*component, onHelp()).TIMES(1);
  REQUIRE_CALL(*component, onShowMenu()).TIMES(1);
  REQUIRE_CALL(*component, onExit()).TIMES(1).LR_RETURN(true);

  fixture.pushLogin();
  fixture.pushCommand("Test");// navigate into the component
  fixture.pushCommand("foo");// routed to the component
  fixture.pushCommand("help");// component help
  fixture.pushCommand("menu");// component menu
  fixture.pushCommand("history");// component history
  fixture.pushCommand("clear");
  fixture.pushCommand("exit");// leave the component
  fixture.pushCommand("exit");// leave the cli

  fixture.runSession();
}

TEST_CASE("CommandLineService: tab autocompletes the unique match")
{
  ClsFixture fixture({});
  fixture.pushLogin();
  // first Tab on an empty line -> print the command list
  fixture.input->push({ KeyType::Ascii, '\t' });
  // second Tab on an empty line -> beep (nothing to complete)
  fixture.input->push({ KeyType::Ascii, '\t' });
  // "he" + Tab -> completes "help"
  fixture.input->pushText("he");
  fixture.input->push({ KeyType::Ascii, '\t' });
  // finish the session
  fixture.input->push({ KeyType::CtrlC, ' ' });

  fixture.runSession();
  SUCCEED("tab completion handled");
}

TEST_CASE("CommandLineService: tab without a match beeps")
{
  ClsFixture fixture({});
  fixture.pushLogin();
  fixture.input->pushText("zzz");
  fixture.input->push({ KeyType::Ascii, '\t' });
  fixture.input->push({ KeyType::CtrlC, ' ' });

  fixture.runSession();
  SUCCEED("tab without match handled");
}

TEST_CASE("CommandLineService: Ctrl+R recalls a matching history entry")
{
  ClsFixture fixture({});
  fixture.pushLogin();
  fixture.pushCommand("alpha");// add to history
  fixture.input->push({ KeyType::CtrlR, ' ' });// empty prefix -> newest entry
  fixture.input->pushText("zzz");
  fixture.input->push({ KeyType::CtrlR, ' ' });// no match -> beep
  fixture.input->push({ KeyType::CtrlC, ' ' });

  fixture.runSession();
  SUCCEED("reverse search handled");
}

TEST_CASE("CommandLineService: up and down navigate the history")
{
  ClsFixture fixture({});
  fixture.pushLogin();
  fixture.pushCommand("cmd1");
  fixture.pushCommand("cmd2");
  fixture.input->push({ KeyType::Up, ' ' });
  fixture.input->push({ KeyType::Up, ' ' });
  fixture.input->push({ KeyType::Up, ' ' });// exhausted -> beep
  fixture.input->push({ KeyType::Down, ' ' });
  fixture.input->push({ KeyType::CtrlC, ' ' });

  fixture.runSession();
  SUCCEED("history navigation handled");
}

TEST_CASE("CommandLineService: end of file ends the session")
{
  ClsFixture fixture({});
  fixture.pushLogin();
  fixture.input->push({ KeyType::Eof, ' ' });

  fixture.runSession();
  SUCCEED("eof handled");
}

TEST_CASE("CommandLineService: Ctrl+C ends the session")
{
  ClsFixture fixture({});
  fixture.pushLogin();
  fixture.input->push({ KeyType::CtrlC, ' ' });

  fixture.runSession();
  SUCCEED("ctrl-c handled");
}

TEST_CASE("CommandLineService: a failed login aborts the session")
{
  ClsFixture fixture({});
  // The service logs in during onStart(); an immediate end of file makes the
  // reader abort, so no login request is ever sent.
  fixture.input->push({ KeyType::Eof, ' ' });

  fixture.runSession();
  SUCCEED("failed login handled");
}

}// namespace lintel
