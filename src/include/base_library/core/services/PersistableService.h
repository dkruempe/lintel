#ifndef CPP_BASE_LIBRARY_PERSISTABLESERVICE_H
#define CPP_BASE_LIBRARY_PERSISTABLESERVICE_H

#include <memory>
#include <vector>

#include "base_library/core/services/PersistableBean.h"

class PersistableService {
private:
    std::vector<std::shared_ptr<PersistableBean>> m_persistableBeans;

public:
    explicit PersistableService(
            std::vector<std::shared_ptr<PersistableBean>> persistableBeans);

    void awake();
};

#endif  // CPP_BASE_LIBRARY_PERSISTABLESERVICE_H
