#ifndef LINTEL_MOCKINPUTSERVICE_H
#define LINTEL_MOCKINPUTSERVICE_H

#include <catch2/trompeloeil.hpp>

#include "lintel/features/cli/services/IInputService.h"

namespace lintel {
class MockInputService : public IInputService
{
public:
  MAKE_MOCK0(onRead, KeyEvent(), override);
};

}// namespace lintel
#endif
