#ifndef PLC_INITIALIZESERVICE_H
#define PLC_INITIALIZESERVICE_H

#include <memory>
#include <vector>

#include "base_library/core/services/AbstractService.h"

class InitializeService {
private:
    std::vector<std::shared_ptr<AbstractServiceInterface>> m_abstractServices;

public:
    explicit InitializeService(
            const std::vector<std::shared_ptr<AbstractServiceInterface>>
            &abstractServices);

    void onInitialize();
};

#endif  // PLC_INITIALIZESERVICE_H
