#ifndef CPP_BASE_LIBRARY_VIRTUALGROUPBOOTSTRAPPLUGIN_H
#define CPP_BASE_LIBRARY_VIRTUALGROUPBOOTSTRAPPLUGIN_H

#include <memory>

#include <Hypodermic/Container.h>

#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"
#include "base_library/core/plugins/BootstrapPlugin.h"

class VirtualGroupBootstrapPlugin : public BootstrapPlugin {
private:
    std::shared_ptr<Hypodermic::Container> m_container;
    std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;

public:
    explicit VirtualGroupBootstrapPlugin(
            const std::shared_ptr<DatabaseConnectionConfigurations>
            &connectionConfigurations,
            std::shared_ptr<Hypodermic::Container> container);

    void onStart() override;

    BootstrapSequence getPriority() override;
};

#endif  // CPP_BASE_LIBRARY_VIRTUALGROUPBOOTSTRAPPLUGIN_H
