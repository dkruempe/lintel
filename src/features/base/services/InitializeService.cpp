#include "base_library/features/base/services/InitializeService.h"

InitializeService::InitializeService(
    const std::vector<std::shared_ptr<AbstractServiceInterface>>
        &abstractServices)
    : abstractServices(abstractServices) {
  for (const std::shared_ptr<AbstractServiceInterface> &abstractService :
       abstractServices) {
    abstractService->onInitialize();
  }
}