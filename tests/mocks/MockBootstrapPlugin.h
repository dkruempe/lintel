#ifndef CPP_BASE_LIBRARY_MOCKBOOTSTRAPPLUGIN_H
#define CPP_BASE_LIBRARY_MOCKBOOTSTRAPPLUGIN_H

#include <catch2/trompeloeil.hpp>

#include "base_library/features/base/plugins/BootstrapPlugin.h"

class MockBootstrapPlugin : public BootstrapPlugin {
public:
    MAKE_MOCK0(onStart, void(), override);
    MAKE_MOCK0(getPriority, BootstrapSequence(), override);
};

#endif  // CPP_BASE_LIBRARY_MOCKBOOTSTRAPPLUGIN_H
