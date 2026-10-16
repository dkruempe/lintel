#include <catch2/catch_all.hpp>

#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

#include <unistd.h>

#include "lintel/features/cli/CliTypes.h"
#include "lintel/features/cli/services/InputService.h"

namespace lintel {
namespace {

  /** Redirects fd 0 to a pipe preloaded with the given bytes for the object's lifetime. */
  class ScopedStdinPipe
  {
    int m_saved = -1;
    int m_readEnd = -1;
    int m_writeEnd = -1;

  public:
    explicit ScopedStdinPipe(std::initializer_list<unsigned char> bytes)
    {
      m_saved = ::dup(STDIN_FILENO);
      int fds[2] = { -1, -1 };
      REQUIRE(::pipe(fds) == 0);
      m_readEnd = fds[0];
      m_writeEnd = fds[1];
      for (unsigned char value : bytes) {
        const ssize_t written = ::write(m_writeEnd, &value, 1);
        REQUIRE(written == 1);
      }
      ::close(m_writeEnd);
      m_writeEnd = -1;
      REQUIRE(::dup2(m_readEnd, STDIN_FILENO) == STDIN_FILENO);
      ::close(m_readEnd);
      m_readEnd = -1;
    }

    ~ScopedStdinPipe()
    {
      if (m_writeEnd != -1) { ::close(m_writeEnd); }
      if (m_readEnd != -1) { ::close(m_readEnd); }
      REQUIRE(::dup2(m_saved, STDIN_FILENO) == STDIN_FILENO);
      ::close(m_saved);
    }
  };

  /** Feeds the given bytes to fd 0 and returns the decoded key type. */
  KeyType readKey(std::initializer_list<unsigned char> bytes)
  {
    ScopedStdinPipe pipe(bytes);
    InputService service;
    return service.onRead().first;
  }

}// namespace

TEST_CASE("InputService: decodes control characters")
{
  REQUIRE(readKey({ 3 }) == KeyType::CtrlC);
  REQUIRE(readKey({ 4 }) == KeyType::Eof);
  REQUIRE(readKey({ 18 }) == KeyType::CtrlR);
  REQUIRE(readKey({ 127 }) == KeyType::Backspace);
  REQUIRE(readKey({ 10 }) == KeyType::Ret);
}

TEST_CASE("InputService: decodes printable ascii")
{
  REQUIRE(readKey({ 'a' }) == KeyType::Ascii);
  REQUIRE(readKey({ 'Z' }) == KeyType::Ascii);
}

TEST_CASE("InputService: end of input yields Ctrl+C") { REQUIRE(readKey({}) == KeyType::CtrlC); }

TEST_CASE("InputService: decodes arrow and navigation escape sequences")
{
  REQUIRE(readKey({ 27, 91, 65 }) == KeyType::Up);
  REQUIRE(readKey({ 27, 91, 66 }) == KeyType::Down);
  REQUIRE(readKey({ 27, 91, 68 }) == KeyType::Left);
  REQUIRE(readKey({ 27, 91, 67 }) == KeyType::Right);
  REQUIRE(readKey({ 27, 91, 70 }) == KeyType::End);
  REQUIRE(readKey({ 27, 91, 72 }) == KeyType::Home);
  REQUIRE(readKey({ 27, 91, 51, 126 }) == KeyType::Canc);// Delete key
}

TEST_CASE("InputService: incomplete or unknown escape sequences are ignored")
{
  REQUIRE(readKey({ 27 }) == KeyType::Ignored);// ESC then end of input
  REQUIRE(readKey({ 27, 91 }) == KeyType::Ignored);// ESC [ then end of input
  REQUIRE(readKey({ 27, 91, 51 }) == KeyType::Ignored);// ESC [ 3 then end of input
  REQUIRE(readKey({ 27, 91, 51, 65 }) == KeyType::Ignored);// ESC [ 3 then something else
  REQUIRE(readKey({ 27, 91, 90 }) == KeyType::Ignored);// ESC [ Z -> unknown final byte
  REQUIRE(readKey({ 27, 'X' }) == KeyType::Ignored);// ESC followed by a non-'[' byte
}

}// namespace lintel
