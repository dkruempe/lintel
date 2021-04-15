#ifndef PLC_INITIALIZESERVICE_H
#define PLC_INITIALIZESERVICE_H

#include "base_library/core/services/AbstractService.h"
#include <memory>
#include <vector>

class InitializeService {
private:
  std::vector<std::shared_ptr<AbstractServiceInterface>> abstractServices;

public:
  explicit InitializeService(
      const std::vector<std::shared_ptr<AbstractServiceInterface>>
          &abstractServices);
};

#endif // PLC_INITIALIZESERVICE_H
