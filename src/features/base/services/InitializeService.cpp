#include "lintel/features/base/services/InitializeService.h"

namespace lintel {
InitializeService::InitializeService(const std::vector<std::shared_ptr<AbstractServiceInterface>> &abstractServices)
  : m_abstractServices(abstractServices)
{}

void InitializeService::onInitialize()
{
  for (const std::shared_ptr<AbstractServiceInterface> &abstractService : m_abstractServices) {
    abstractService->onInitialize();
  }
}
}// namespace lintel