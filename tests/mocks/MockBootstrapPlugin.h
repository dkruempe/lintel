#ifndef LINTEL_MOCKBOOTSTRAPPLUGIN_H
#define LINTEL_MOCKBOOTSTRAPPLUGIN_H

#include <catch2/trompeloeil.hpp>

#include "lintel/core/plugins/BootstrapPlugin.h"

class MockBootstrapPlugin : public BootstrapPlugin {
public:
    MAKE_MOCK0(onStart, void(), override);
    MAKE_MOCK0(getPriority, BootstrapSequence(), override);
};

#endif  // LINTEL_MOCKBOOTSTRAPPLUGIN_H
