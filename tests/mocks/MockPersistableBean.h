#ifndef LINTEL_MOCKPERSISTABLEBEAN_H
#define LINTEL_MOCKPERSISTABLEBEAN_H

#include <catch2/trompeloeil.hpp>

#include "lintel/core/services/PersistableBean.h"

namespace lintel {
class MockPersistableBean : public PersistableBean
{
public:
  MAKE_MOCK0(onAwake, void(), override);
};

}// namespace lintel
#endif// LINTEL_MOCKPERSISTABLEBEAN_H
