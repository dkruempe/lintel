#ifndef CPP_BASE_LIBRARY_INPUTSERVICE_H
#define CPP_BASE_LIBRARY_INPUTSERVICE_H

#include <termios.h>
#include <unistd.h>

#include <utility>

enum class KeyType {
  Ascii,
  Up,
  Down,
  Left,
  Right,
  Backspace,
  Canc,
  Home,
  End,
  Ret,
  Eof,
  Ignored
};

using KeyPressed = std::pair<KeyType, char>;

class InputService {
 private:
  termios oldt{};
  termios newt{};

 public:
  InputService();
  ~InputService();
  KeyPressed onRead();
};

#endif  // CPP_BASE_LIBRARY_INPUTSERVICE_H
