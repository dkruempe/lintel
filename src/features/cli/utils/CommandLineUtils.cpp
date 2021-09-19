#include "base_library/features/cli/utils/CommandLineUtils.h"

#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
#include <termios.h>
#endif
#include <unistd.h>

#include <iostream>

void CommandLineUtils::disableOfInputEcho() {
#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
  termios tty{};
  tcgetattr(STDIN_FILENO, &tty);
  /* we want to disable echo */
  tty.c_lflag &= static_cast<unsigned long>(~ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &tty);
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
#else
  static_assert(false, "unsupported platform");
#endif
}
void CommandLineUtils::clear() { std::cout << "\033[2J\033[1;1H"; }
void CommandLineUtils::disableOfCanonicalMode() {
  termios tty{};
  tcgetattr(STDIN_FILENO, &tty);
  tty.c_lflag &=(static_cast<unsigned long>(~ICANON));
  tty.c_cc[VTIME] = 0;
  tty.c_cc[VMIN] = 1;
  tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}
void CommandLineUtils::enableOfCanonicalMode() {
#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
  termios tty{};
  tcgetattr(STDIN_FILENO, &tty);
  tty.c_lflag |= ICANON;
  tcsetattr(STDIN_FILENO, TCSANOW, &tty);
#else
  static_assert(false, "unsupported platform");
#endif
}
