#include <base_library/services/StringifyService.h>
#include <base_library/utils/TypeName.h>
#include <iostream>

enum class TYPES {
  UNDEFINED = -1,
  CLIENT = 0,
  SERVER = 1,
  STANDALONE = 2
};

std::string convertToString(TYPES e) {
  switch(e) {
  case TYPES::CLIENT:
    return "TYPES::CLIENT";
  case TYPES::SERVER:
    return "TYPES::SERVER";
  case TYPES::STANDALONE:
    return "TYPES::STANDALONE";
  default:
    return "TYPES::UNDEFINED";
  }
  return "TYPES::UNDEFINED";
}

TYPES convertToValue(const std::string &e) {
  if (e == "TYPES::CLIENT") {
    return TYPES::CLIENT;
  } else if (e == "TYPES::SERVER") {
    return TYPES::SERVER;
  } else if (e == "TYPES::STANDALONE") {
    return TYPES::STANDALONE;
  }
  return TYPES::UNDEFINED;
}

IMPLEMENT_STRINGIFY_SERVICE_FOR(TYPES, convertToString, convertToValue)

int main(int argc, char *argv[]) {
  TYPES type = TYPES::CLIENT;
  std::string typeString = StringifyService<TYPES>::serializeToString(type);
  std::cout << typeString << std::endl;
  TYPES typeCopy = StringifyService<TYPES>::deserializeFromString(typeString);
  if (type == typeCopy) {
    std::cout << "WORKED" << std::endl;
  } else {
    std::cout << "FAILED" << std::endl;
  }
  return 0;
}