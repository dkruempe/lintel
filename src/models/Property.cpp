#include "base_library/models/Property.h"

#define REGISTER_PROPERTY(type)                                                \
  bool Property<type>::registered = Property<type>::Registration();

REGISTER_PROPERTY(int8_t)
REGISTER_PROPERTY(int16_t)
REGISTER_PROPERTY(int32_t)
REGISTER_PROPERTY(int64_t)
REGISTER_PROPERTY(uint8_t)
REGISTER_PROPERTY(uint16_t)
REGISTER_PROPERTY(uint32_t)
REGISTER_PROPERTY(uint64_t)
REGISTER_PROPERTY(float)
REGISTER_PROPERTY(double)
REGISTER_PROPERTY(std::string)
REGISTER_PROPERTY(bool)