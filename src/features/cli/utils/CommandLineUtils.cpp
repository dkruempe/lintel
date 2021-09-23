#include "base_library/features/cli/utils/CommandLineUtils.h"

#include <iostream>

void CommandLineUtils::disableOfInputEcho() {
#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
  if (m_isEchoDisabled) {
    return;
  }
  termios tty{};
  tcgetattr(STDIN_FILENO, &tty);
  /* we want to disable echo */
  tty.c_lflag &= static_cast<unsigned long>(~ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &tty);
  m_isEchoDisabled = true;
#else
  static_assert(false, "unsupported platform");
#endif
}

void CommandLineUtils::enableOfInputEcho() {
#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
  termios tty{};
  tcgetattr(STDIN_FILENO, &tty);
  /* we want to reenable echo */
  tty.c_lflag |= ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &tty);
  m_isEchoDisabled = false;
#else
  static_assert(false, "unsupported platform");
#endif
}
void CommandLineUtils::clear() {
#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
  write(STDOUT_FILENO, "\x1b[H\x1b[2J", 7);
#else
  std::cout << "\033[2J\033[1;1H";
#endif
}
void CommandLineUtils::disableOfCanonicalMode() {
#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
  if (m_isCanonicalMode) {
    return;
  }
  termios tty{};
  tcgetattr(STDIN_FILENO, &tty);
  tty.c_lflag &= (static_cast<unsigned long>(~ICANON));
  tty.c_cc[VTIME] = 0;
  tty.c_cc[VMIN] = 1;
  tcsetattr(STDIN_FILENO, TCSANOW, &tty);
  m_isCanonicalMode = false;
#else
  static_assert(false, "unsupported platform");
#endif
}
void CommandLineUtils::enableOfCanonicalMode() {
#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
  termios tty{};
  tcgetattr(STDIN_FILENO, &tty);
  tty.c_lflag |= ICANON;
  tcsetattr(STDIN_FILENO, TCSANOW, &tty);
  m_isCanonicalMode = true;
#else
  static_assert(false, "unsupported platform");
#endif
}
void CommandLineUtils::beep() {
  fprintf(stderr, "\x7");
  fflush(stderr);
}
bool CommandLineUtils::isCanonicalMode() const { return m_isCanonicalMode; }
bool CommandLineUtils::isEchoMode() const { return !m_isEchoDisabled; }
