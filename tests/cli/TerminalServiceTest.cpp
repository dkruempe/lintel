#include <catch2/catch_all.hpp>

#include <iostream>
#include <sstream>
#include <string>

#include "lintel/features/cli/CliTypes.h"
#include "lintel/features/cli/services/TerminalService.h"

namespace lintel {
namespace {

  /** RAII redirector for `std::cout`, so the escape-sequence output stays out of the test log. */
  class terminalCoutCapture
  {
  public:
    terminalCoutCapture() : m_oldBuffer(std::cout.rdbuf(m_stream.rdbuf())) {}

    ~terminalCoutCapture() { std::cout.rdbuf(m_oldBuffer); }

    terminalCoutCapture(const terminalCoutCapture &) = delete;
    terminalCoutCapture &operator=(const terminalCoutCapture &) = delete;

    [[nodiscard]] std::string str() const { return m_stream.str(); }

  private:
    std::ostringstream m_stream;
    std::streambuf *m_oldBuffer;
  };

  KeyEvent terminalAscii(char c) { return { KeyType::Ascii, c }; }

  void terminalType(TerminalService &service, const std::string &text)
  {
    for (const char c : text) { service.onKeyPressed(terminalAscii(c), "MAIN"); }
  }

}// namespace

TEST_CASE("TerminalService: a fresh service has an empty line")
{
  TerminalService service;
  REQUIRE(service.getLine().empty());
}

TEST_CASE("TerminalService: ascii keys build the line and return Nothing")
{
  TerminalService service;
  terminalCoutCapture out;

  const SymbolEvent event = service.onKeyPressed(terminalAscii('a'), "MAIN");
  REQUIRE(event.first == Symbol::Nothing);
  terminalType(service, "bc");
  REQUIRE(service.getLine() == "abc");
}

TEST_CASE("TerminalService: tab returns the Tab symbol with the current line")
{
  TerminalService service;
  terminalCoutCapture out;
  terminalType(service, "ab");

  const SymbolEvent event = service.onKeyPressed(terminalAscii('\t'), "MAIN");
  REQUIRE(event.first == Symbol::Tab);
  REQUIRE(event.second == "ab");
}

TEST_CASE("TerminalService: return emits the Command symbol and clears the line")
{
  TerminalService service;
  terminalCoutCapture out;
  terminalType(service, "cmd arg");

  const SymbolEvent event = service.onKeyPressed({ KeyType::Ret, '\0' }, "MAIN");
  REQUIRE(event.first == Symbol::Command);
  REQUIRE(event.second == "cmd arg");
  REQUIRE(service.getLine().empty());
}

TEST_CASE("TerminalService: backspace removes the character before the cursor")
{
  TerminalService service;
  terminalCoutCapture out;
  terminalType(service, "abc");

  service.onKeyPressed({ KeyType::Backspace, '\0' }, "MAIN");
  REQUIRE(service.getLine() == "ab");
}

TEST_CASE("TerminalService: backspace at the start of the line is a no-op")
{
  TerminalService service;
  terminalCoutCapture out;

  const SymbolEvent event = service.onKeyPressed({ KeyType::Backspace, '\0' }, "MAIN");
  REQUIRE(event.first == Symbol::Nothing);
  REQUIRE(service.getLine().empty());
}

TEST_CASE("TerminalService: left and right move the insertion point")
{
  TerminalService service;
  terminalCoutCapture out;
  terminalType(service, "ac");

  service.onKeyPressed({ KeyType::Left, '\0' }, "MAIN");
  service.onKeyPressed(terminalAscii('b'), "MAIN");
  REQUIRE(service.getLine() == "abc");

  service.onKeyPressed({ KeyType::Left, '\0' }, "MAIN");
  service.onKeyPressed({ KeyType::Right, '\0' }, "MAIN");
  service.onKeyPressed(terminalAscii('d'), "MAIN");
  REQUIRE(service.getLine() == "abdc");
}

TEST_CASE("TerminalService: left at the start and right at the end are no-ops")
{
  TerminalService service;
  terminalCoutCapture out;
  terminalType(service, "a");

  service.onKeyPressed({ KeyType::Left, '\0' }, "MAIN");
  service.onKeyPressed({ KeyType::Left, '\0' }, "MAIN");
  service.onKeyPressed(terminalAscii('x'), "MAIN");
  REQUIRE(service.getLine() == "xa");

  service.onKeyPressed({ KeyType::Right, '\0' }, "MAIN");
  service.onKeyPressed({ KeyType::Right, '\0' }, "MAIN");
  service.onKeyPressed({ KeyType::Right, '\0' }, "MAIN");
  service.onKeyPressed(terminalAscii('y'), "MAIN");
  REQUIRE(service.getLine() == "xay");
}

TEST_CASE("TerminalService: up and down return their symbols")
{
  TerminalService service;
  terminalCoutCapture out;

  REQUIRE(service.onKeyPressed({ KeyType::Up, '\0' }, "MAIN").first == Symbol::Up);
  REQUIRE(service.onKeyPressed({ KeyType::Down, '\0' }, "MAIN").first == Symbol::Down);
}

TEST_CASE("TerminalService: home and end move the cursor")
{
  TerminalService service;
  terminalCoutCapture out;
  terminalType(service, "ac");

  service.onKeyPressed({ KeyType::Home, '\0' }, "MAIN");
  service.onKeyPressed(terminalAscii('b'), "MAIN");
  REQUIRE(service.getLine() == "bac");

  service.onKeyPressed({ KeyType::End, '\0' }, "MAIN");
  service.onKeyPressed(terminalAscii('d'), "MAIN");
  REQUIRE(service.getLine() == "bacd");
}

TEST_CASE("TerminalService: cancel deletes the character at the cursor")
{
  TerminalService service;
  terminalCoutCapture out;
  terminalType(service, "abc");

  service.onKeyPressed({ KeyType::Home, '\0' }, "MAIN");
  service.onKeyPressed({ KeyType::Canc, '\0' }, "MAIN");
  REQUIRE(service.getLine() == "bc");
}

TEST_CASE("TerminalService: cancel at the end of the line is a no-op")
{
  TerminalService service;
  terminalCoutCapture out;
  terminalType(service, "ab");

  service.onKeyPressed({ KeyType::Canc, '\0' }, "MAIN");
  REQUIRE(service.getLine() == "ab");
}

TEST_CASE("TerminalService: control keys map to their symbols")
{
  TerminalService service;
  terminalCoutCapture out;

  REQUIRE(service.onKeyPressed({ KeyType::CtrlC, '\0' }, "MAIN").first == Symbol::CtrlC);
  REQUIRE(service.onKeyPressed({ KeyType::CtrlR, '\0' }, "MAIN").first == Symbol::CtrlR);
  REQUIRE(service.onKeyPressed({ KeyType::Eof, '\0' }, "MAIN").first == Symbol::Eof);
  REQUIRE(service.onKeyPressed({ KeyType::Ignored, '\0' }, "MAIN").first == Symbol::Nothing);
}

TEST_CASE("TerminalService: log replaces the current line")
{
  TerminalService service;
  terminalCoutCapture out;

  service.log("first");
  REQUIRE(service.getLine() == "first");
  REQUIRE(out.str().find("first") != std::string::npos);

  service.log("second line");
  REQUIRE(service.getLine() == "second line");
}

TEST_CASE("TerminalService: hidden characters mask output but still build the line")
{
  TerminalService service;
  terminalCoutCapture out;

  service.enableHideChars();
  terminalType(service, "secret");
  REQUIRE(service.getLine() == "secret");
  REQUIRE(out.str().find("secret") == std::string::npos);
  REQUIRE(out.str().find("******") != std::string::npos);

  service.disableHideChars();
  service.log("visible");
  REQUIRE(out.str().find("visible") != std::string::npos);
}

TEST_CASE("TerminalService: resetCursor clears the line")
{
  TerminalService service;
  terminalCoutCapture out;
  terminalType(service, "abc");

  service.resetCursor();
  REQUIRE(service.getLine().empty());
}

}// namespace lintel
