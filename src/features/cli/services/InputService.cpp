#include "lintel/features/cli/services/InputService.h"

#include "lintel/features/cli/CliTypes.h"

#include <csignal>
#include <cstdlib>
#include <mutex>

#include <cerrno>
#include <iostream>
#include <utility>

#if defined(_WIN32)
// Keep the transitive Windows headers from defining the min/max macros.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <conio.h>// _kbhit / _getch
#include <windows.h>// GetConsoleMode / SetConsoleMode
#endif

#include "lintel/core/services/LoggerService.h"

namespace lintel {
#if defined(_WIN32)
namespace {
  /** Console mode saved on raw-mode entry, restored by restoreTerminal (Windows counterpart of g_originalTermios). */
  DWORD g_originalConsoleMode = 0;
  bool g_consoleModeSaved = false;

  void restoreTerminal()
  {
    if (!g_consoleModeSaved) { return; }
    HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE) { return; }
    static_cast<void>(SetConsoleMode(handle, g_originalConsoleMode));
  }

  /**
   * Wait for a single byte from the console input buffer.
   * @param timeoutMs how long to wait; -1 blocks indefinitely
   * @return the byte (0..255) or -1 when the timeout elapsed
   */
  int waitForByte(int timeoutMs)
  {
    const ULONGLONG deadline = timeoutMs < 0 ? 0 : GetTickCount64() + static_cast<ULONGLONG>(timeoutMs);
    for (;;) {
      if (_kbhit() != 0) { return _getch(); }
      if (timeoutMs >= 0 && GetTickCount64() >= deadline) { return -1; }
      Sleep(10);
    }
  }

  /**
   * Translate the second byte of an extended console key (after the 0x00/0xE0
   * prefix) into the same logical KeyType the POSIX ANSI decoding produces.
   */
  KeyEvent decodeScanCode(int scanCode)
  {
    switch (scanCode) {
    case 71:
      return std::make_pair(KeyType::Home, ' ');
    case 72:
      return std::make_pair(KeyType::Up, ' ');
    case 75:
      return std::make_pair(KeyType::Left, ' ');
    case 77:
      return std::make_pair(KeyType::Right, ' ');
    case 79:
      return std::make_pair(KeyType::End, ' ');
    case 80:
      return std::make_pair(KeyType::Down, ' ');
    case 83:// Delete, matches "ESC [ 3 ~" on POSIX
      return std::make_pair(KeyType::Canc, ' ');
    default:
      return std::make_pair(KeyType::Ignored, ' ');
    }
  }
}// namespace
#else
namespace {
  termios g_originalTermios{};

  void restoreTerminal()
  {
    if (g_originalTermios.c_cflag != 0) { tcsetattr(STDIN_FILENO, TCSANOW, &g_originalTermios); }
  }

  bool g_sigusr1Registered = false;

  extern "C" void sigusr1Handler(int /*sig*/) {}
}// namespace
#endif

KeyEvent InputService::onRead()
{
#if defined(_WIN32)
  std::cout.flush();
  // The 100 ms idle timeout keeps the CLI loop able to observe m_running being
  // cleared: the POSIX EINTR wake-up (SIGUSR1 via pthread_kill) has no Windows
  // counterpart, so the reader has to time out instead of blocking forever.
  const int ch = waitForByte(100);
  if (ch < 0) { return std::make_pair(KeyType::Ignored, ' '); }
  if (ch == 0 || ch == 0xE0) { return decodeScanCode(waitForByte(100)); }
  switch (ch) {
  case 3:// CtrlC
    return std::make_pair(KeyType::CtrlC, ' ');
  case 4:// EOT
    return std::make_pair(KeyType::Eof, ' ');
  case 18:
    return std::make_pair(KeyType::CtrlR, ' ');
  case 8:// the console delivers BS where POSIX delivers DEL
  case 127:
    return std::make_pair(KeyType::Backspace, ' ');
  case 10:
  case 13:// Enter arrives as CR once line input is disabled
    return std::make_pair(KeyType::Ret, ' ');
  case 27:// symbol
    return readEscapeSequence();
  default:// ascii
    return std::make_pair(KeyType::Ascii, static_cast<char>(ch));
  }
#else
  std::cout.flush();
  unsigned char ch;
  ssize_t n = ::read(STDIN_FILENO, &ch, 1);
  if (n < 0 && errno == EINTR) { return std::make_pair(KeyType::CtrlC, ' '); }
  if (n <= 0) { return std::make_pair(KeyType::CtrlC, ' '); }
  int chInt = static_cast<int>(ch);
  switch (chInt) {
  case 3:// CtrlC
    return std::make_pair(KeyType::CtrlC, ' ');
  case 4:// EOT
    return std::make_pair(KeyType::Eof, ' ');
  case 18:
    return std::make_pair(KeyType::CtrlR, ' ');
  case 127:
    return std::make_pair(KeyType::Backspace, ' ');
  case 10:
    return std::make_pair(KeyType::Ret, ' ');
  case 27:// symbol
    return readEscapeSequence();
  default:// ascii
  {
    const char c = static_cast<char>(chInt);
    return std::make_pair(KeyType::Ascii, c);
  }
  }
#endif
}

KeyEvent InputService::readEscapeSequence()
{
#if defined(_WIN32)
  // ESC already consumed. Two shapes reach this point on Windows: extended
  // keys (0x00/0xE0 + scan code) and pasted ANSI sequences ("ESC [ A" etc.),
  // which decode exactly like the POSIX path below.
  const int ch = waitForByte(100);
  if (ch < 0) { return std::make_pair(KeyType::Ignored, ' '); }
  if (ch == 0 || ch == 0xE0) { return decodeScanCode(waitForByte(100)); }
  if (ch != 91)// arrow keys
  {
    return std::make_pair(KeyType::Ignored, ' ');
  }
  const int next = waitForByte(100);
  if (next < 0) { return std::make_pair(KeyType::Ignored, ' '); }
  switch (next) {
  case 51: {
    const int last = waitForByte(100);
    if (last == 126) { return std::make_pair(KeyType::Canc, ' '); }
    return std::make_pair(KeyType::Ignored, ' ');
  }
  case 65:
    return std::make_pair(KeyType::Up, ' ');
  case 66:
    return std::make_pair(KeyType::Down, ' ');
  case 68:
    return std::make_pair(KeyType::Left, ' ');
  case 67:
    return std::make_pair(KeyType::Right, ' ');
  case 70:
    return std::make_pair(KeyType::End, ' ');
  case 72:
    return std::make_pair(KeyType::Home, ' ');
  default:
    return std::make_pair(KeyType::Ignored, ' ');
  }
#else
  unsigned char ch = 0;
  ssize_t n = ::read(STDIN_FILENO, &ch, 1);
  if (n != 1) { return std::make_pair(KeyType::Ignored, ' '); }
  if (static_cast<int>(ch) != 91)// arrow keys
  {
    return std::make_pair(KeyType::Ignored, ' ');
  }
  n = ::read(STDIN_FILENO, &ch, 1);
  if (n != 1) { return std::make_pair(KeyType::Ignored, ' '); }
  switch (static_cast<int>(ch)) {
  case 51:
    n = ::read(STDIN_FILENO, &ch, 1);
    if (n != 1) { return std::make_pair(KeyType::Ignored, ' '); }
    if (static_cast<int>(ch) == 126) { return std::make_pair(KeyType::Canc, ' '); }
    return std::make_pair(KeyType::Ignored, ' ');
  case 65:
    return std::make_pair(KeyType::Up, ' ');
  case 66:
    return std::make_pair(KeyType::Down, ' ');
  case 68:
    return std::make_pair(KeyType::Left, ' ');
  case 67:
    return std::make_pair(KeyType::Right, ' ');
  case 70:
    return std::make_pair(KeyType::End, ' ');
  case 72:
    return std::make_pair(KeyType::Home, ' ');
  default:
    return std::make_pair(KeyType::Ignored, ' ');
  }
#endif
}

#if defined(_WIN32)
InputService::InputService()
{
  // Enter raw mode like the POSIX branch does: line input (canonical) off, echo
  // off and processed input off so Ctrl+C arrives as the byte 0x03 and is
  // reported as KeyType::CtrlC, mirroring the POSIX path with ISIG cleared.
  HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
  if (handle != nullptr && handle != INVALID_HANDLE_VALUE) {
    DWORD mode = 0;
    if (GetConsoleMode(handle, &mode) != 0) {
      g_originalConsoleMode = mode;
      g_consoleModeSaved = true;
      static_cast<void>(
        SetConsoleMode(handle, mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT)));
    }
  }

  static std::once_flag atexitFlag;
  std::call_once(atexitFlag, [] { std::atexit(restoreTerminal); });
}
#else
InputService::InputService() : newt()
{
  tcgetattr(STDIN_FILENO, &g_originalTermios);
  newt = g_originalTermios;
  newt.c_lflag &= ~tcflag_t(ICANON | ECHO | ISIG);
  newt.c_cc[VMIN] = 1;
  newt.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  static std::once_flag atexitFlag;
  std::call_once(atexitFlag, [] { std::atexit(restoreTerminal); });

  if (!g_sigusr1Registered) {
    g_sigusr1Registered = true;
    struct sigaction sa{};
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = sigusr1Handler;
    sigaction(SIGUSR1, &sa, nullptr);
  }
}
#endif

InputService::~InputService() { restoreTerminal(); }
}// namespace lintel
