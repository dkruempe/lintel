#include <iostream>
#include "utils/TypeName.h"
class EnumClass {
public:
  enum enumImpl { Anna, Dominik };

  explicit EnumClass(enumImpl value) : value(value) {}

  enumImpl operator()() const { return value; }
  EnumClass &operator=(const enumImpl &rhs) {
    value = rhs;
    return *this;
  }
  bool operator==(const EnumClass &rhs) const { return value == rhs.value; }
  bool operator!=(const EnumClass &rhs) const { return !(rhs == *this); }
  bool operator==(const enumImpl &rhs) const { return value == rhs; }
  bool operator!=(const enumImpl &rhs) const { return value == rhs; }
  friend std::ostream &operator<<(std::ostream &os, const EnumClass &aClass) {
    os << aClass.toString();
    return os;
  }

private:
  enumImpl value;
  [[nodiscard]] std::string toString() const {
    switch (value) {
    case Anna:
      return "Anna";
    case Dominik:
      return "Dominik";
    }
  }
};

int main(int argc, char *argv[]) {
  EnumClass enumClass(EnumClass::Anna);
  if (enumClass == EnumClass::Anna) {
    std::cout << enumClass << std::endl;
  } else {
    std::cout << "Something went wrong" << std::endl;
  }
  enumClass = EnumClass::Dominik;
  if (enumClass == EnumClass::Dominik) {
    std::cout << enumClass << std::endl;
  } else {
    std::cout << "Something went wrong" << std::endl;
  }
  std::cout << "Current Value: " << enumClass << std::endl;
  switch (enumClass()) {
  case EnumClass::Anna:
    std::cout << "Ich liebe dich Anna" << std::endl;
    break;
  case EnumClass::Dominik:
    std::cout << "Hallo hier bin ich>" << std::endl;
    break;
  }
  return 0;
}