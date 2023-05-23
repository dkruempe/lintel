#include "base_library/features/base/services/InitializeService.h"

InitializeService::InitializeService(
        const std::vector<std::shared_ptr<AbstractServiceInterface>>
        &abstractServices)
        : m_abstractServices(abstractServices) {}

void InitializeService::onInitialize() {
    for (const std::shared_ptr<AbstractServiceInterface> &abstractService:
            m_abstractServices) {
        abstractService->onInitialize();
    }
}