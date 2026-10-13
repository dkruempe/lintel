#include "lintel/core/services/PersistableService.h"

namespace lintel {
PersistableService::PersistableService(std::vector<std::shared_ptr<PersistableBean>> persistableBeans)
  : m_persistableBeans(std::move(persistableBeans))
{}

void PersistableService::awake()
{
  for (const auto &item : m_persistableBeans) { item->onAwake(); }
}
}// namespace lintel
