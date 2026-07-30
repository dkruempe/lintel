#ifndef CPP_BASE_LIBRARY_MOCKINPUTSERVICE_H
#define CPP_BASE_LIBRARY_MOCKINPUTSERVICE_H

#include <catch2/trompeloeil.hpp>

#include "base_library/features/cli/services/IInputService.h"

class MockInputService : public IInputService {
public:
    MAKE_MOCK0(onRead, KeyEvent(), override);
};

#endif
