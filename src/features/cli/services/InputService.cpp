#include "base_library/features/cli/services/InputService.h"

#include <iostream>
#include <utility>

#include "base_library/core/services/LoggerService.h"

KeyEvent InputService::onRead() {
  int ch = std::getchar();
  switch (ch) {
    case EOF:
    case 3:  // CtrlC
      return std::make_pair(KeyType::CtrlC, ' ');
    case 4:  // EOT
      return std::make_pair(KeyType::Eof, ' ');
    case 127:
      return std::make_pair(KeyType::Backspace, ' ');
    case 10:
      return std::make_pair(KeyType::Ret, ' ');
      break;
    case 27:  // symbol
      ch = std::getchar();
      if (ch == 91)  // arrow keys
      {
        ch = std::getchar();
        switch (ch) {
          case 51:
            ch = std::getchar();
            if (ch == 126)
              return std::make_pair(KeyType::Canc, ' ');
            else
              return std::make_pair(KeyType::Ignored, ' ');
            break;
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
      }
      break;
    default:  // ascii
    {
      const char c = static_cast<char>(ch);
      return std::make_pair(KeyType::Ascii, c);
    }
  }
  return std::make_pair(KeyType::Ignored, ' ');
}
InputService::InputService() {
  // activate manuel mode
  constexpr tcflag_t ICANON_FLAG = ICANON;
  constexpr tcflag_t ECHO_FLAG = ECHO;
  constexpr tcflag_t ISIG_FALG = ISIG;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON_FLAG | ECHO_FLAG | ISIG_FALG);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}
InputService::~InputService() { tcsetattr(STDIN_FILENO, TCSANOW, &oldt); }
