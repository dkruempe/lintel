#include "base_library/core/services/PersistableService.h"

PersistableService::PersistableService(
        std::vector<std::shared_ptr<PersistableBean>> persistableBeans)
        : m_persistableBeans(std::move(persistableBeans)) {}

void PersistableService::awake() {
    for (const auto &item: m_persistableBeans) {
        item->onAwake();
    }
}
