#ifndef CPP_BASE_LIBRARY_MOCKPERSISTABLEBEAN_H
#define CPP_BASE_LIBRARY_MOCKPERSISTABLEBEAN_H

#include <catch2/trompeloeil.hpp>

#include "base_library/core/services/PersistableBean.h"

class MockPersistableBean : public PersistableBean {
public:
    MAKE_MOCK0(onAwake, void(), override);
};

#endif  // CPP_BASE_LIBRARY_MOCKPERSISTABLEBEAN_H
