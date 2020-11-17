#ifndef PLC_INITIALIZESERVICE_H
#define PLC_INITIALIZESERVICE_H

#include <memory>
#include <vector>

#include "base_library/services/AbstractService.h"

class InitializeService {
 private:
  std::vector<std::shared_ptr<AbstractServiceInterface>> abstractServices;

 public:
  explicit InitializeService(
      const std::vector<std::shared_ptr<AbstractServiceInterface>>& abstractServices);
};

#endif  // PLC_INITIALIZESERVICE_H
