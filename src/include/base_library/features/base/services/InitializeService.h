#ifndef PLC_INITIALIZESERVICE_H
#define PLC_INITIALIZESERVICE_H

#include <memory>
#include <vector>

#include "base_library/core/services/AbstractService.h"

/**
 * Service that triggers initialization on a collection of AbstractServiceInterface implementations.
 */
class InitializeService {
private:
    std::vector<std::shared_ptr<AbstractServiceInterface>> m_abstractServices;

public:
    /**
     * Constructor.
     * @param abstractServices services to initialize
     */
    explicit InitializeService(
            const std::vector<std::shared_ptr<AbstractServiceInterface>>
            &abstractServices);

    /** Call onInitialize() on all registered services. */
    void onInitialize();
};

#endif  // PLC_INITIALIZESERVICE_H
