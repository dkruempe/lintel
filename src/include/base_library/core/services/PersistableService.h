#ifndef CPP_BASE_LIBRARY_PERSISTABLESERVICE_H
#define CPP_BASE_LIBRARY_PERSISTABLESERVICE_H

#include <memory>
#include <vector>

#include "base_library/core/services/PersistableBean.h"

/** Service that manages persistable beans and notifies them to reload their state. */
class PersistableService {
private:
    std::vector<std::shared_ptr<PersistableBean>> m_persistableBeans;

public:
    /** Construct a PersistableService with a list of persistable beans.
     * @param persistableBeans the beans to manage */
    explicit PersistableService(
            std::vector<std::shared_ptr<PersistableBean>> persistableBeans);

    /** Notify all managed beans to reload their state from persistent storage. */
    void awake();
};

#endif  // CPP_BASE_LIBRARY_PERSISTABLESERVICE_H
