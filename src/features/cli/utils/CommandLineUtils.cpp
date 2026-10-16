#include "lintel/features/cli/utils/CommandLineUtils.h"

#include <iostream>
#include <tuple>

#if defined(_WIN32)
// Keep the transitive Windows headers from defining the min/max macros.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>// GetConsoleMode / SetConsoleMode
#endif

namespace lintel {
#if defined(_WIN32)
namespace {
  /**
   * Apply a console mode change to the standard input handle.
   * @param bitsToClear mode bits to disable
   * @param bitsToSet   mode bits to enable
   * @return true when the console mode was updated
   */
  bool updateInputConsoleMode(DWORD bitsToClear, DWORD bitsToSet)
  {
    HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE) { return false; }
    DWORD mode = 0;
    if (GetConsoleMode(handle, &mode) == 0) { return false; }
    return SetConsoleMode(handle, (mode & ~bitsToClear) | bitsToSet) != 0;
  }
}// namespace
#endif

void CommandLineUtils::disableOfInputEcho()
{
#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
  if (m_isEchoDisabled) { return; }
  termios tty{};
  tcgetattr(STDIN_FILENO, &tty);
  /* we want to disable echo */
  tty.c_lflag &= static_cast<tcflag_t>(~ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &tty);
  m_isEchoDisabled = true;
#elif defined(_WIN32)
  if (m_isEchoDisabled) { return; }
  /* we want to disable echo */
  static_cast<void>(updateInputConsoleMode(ENABLE_ECHO_INPUT, 0));
  m_isEchoDisabled = true;
#else
  static_assert(false, "unsupported platform");
#endif
}

void CommandLineUtils::enableOfInputEcho()
{
#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
  termios tty{};
  tcgetattr(STDIN_FILENO, &tty);
  /* we want to reenable echo */
  tty.c_lflag |= ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &tty);
  m_isEchoDisabled = false;
#elif defined(_WIN32)
  /* we want to reenable echo */
  static_cast<void>(updateInputConsoleMode(0, ENABLE_ECHO_INPUT));
  m_isEchoDisabled = false;
#else
  static_assert(false, "unsupported platform");
#endif
}

void CommandLineUtils::clear()
{
#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
  std::ignore = write(STDOUT_FILENO, "\x1b[H\x1b[2J", 7);
#elif defined(_WIN32)
  // The ANSI sequences below need VT processing on the output handle; modern
  // conhost has it on, stripped-down/older consoles ship with it off.
  HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
  if (handle != nullptr && handle != INVALID_HANDLE_VALUE) {
    DWORD mode = 0;
    if (GetConsoleMode(handle, &mode) != 0) {
      static_cast<void>(SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING));
    }
  }
  std::cout << "\033[2J\033[1;1H" << std::flush;
#else
  std::cout << "\033[2J\033[1;1H";
#endif
}

void CommandLineUtils::disableOfCanonicalMode()
{
#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
  if (m_isCanonicalMode) { return; }
  termios tty{};
  tcgetattr(STDIN_FILENO, &tty);
  tty.c_lflag &= static_cast<tcflag_t>(~ICANON);
  tty.c_cc[VTIME] = 0;
  tty.c_cc[VMIN] = 1;
  tcsetattr(STDIN_FILENO, TCSANOW, &tty);
  m_isCanonicalMode = false;
#elif defined(_WIN32)
  if (m_isCanonicalMode) { return; }
  static_cast<void>(updateInputConsoleMode(ENABLE_LINE_INPUT, 0));
  m_isCanonicalMode = false;
#else
  static_assert(false, "unsupported platform");
#endif
}

void CommandLineUtils::enableOfCanonicalMode()
{
#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
  termios tty{};
  tcgetattr(STDIN_FILENO, &tty);
  tty.c_lflag |= ICANON;
  tcsetattr(STDIN_FILENO, TCSANOW, &tty);
  m_isCanonicalMode = true;
#elif defined(_WIN32)
  static_cast<void>(updateInputConsoleMode(0, ENABLE_LINE_INPUT));
  m_isCanonicalMode = true;
#else
  static_assert(false, "unsupported platform");
#endif
}

void CommandLineUtils::beep()
{
  static_cast<void>(fprintf(stderr, "\x7"));
  static_cast<void>(fflush(stderr));
}

bool CommandLineUtils::isCanonicalMode() const { return m_isCanonicalMode; }

bool CommandLineUtils::isEchoMode() const { return !m_isEchoDisabled; }
}// namespace lintel
