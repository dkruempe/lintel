#ifndef CPP_BASE_LIBRARY_VIRTUALGROUPBOOTSTRAPPLUGIN_H
#define CPP_BASE_LIBRARY_VIRTUALGROUPBOOTSTRAPPLUGIN_H

#include <memory>

#include <Hypodermic/Container.h>

#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"
#include "base_library/core/plugins/BootstrapPlugin.h"

/** Bootstrap plugin that initializes virtual process groups from the database. */
class VirtualGroupBootstrapPlugin : public BootstrapPlugin {
private:
    std::shared_ptr<Hypodermic::Container> m_container;
    std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;

public:
    /** Construct a VirtualGroupBootstrapPlugin.
     * @param connectionConfigurations database connection configurations
     * @param container                the dependency injection container */
    explicit VirtualGroupBootstrapPlugin(
            const std::shared_ptr<DatabaseConnectionConfigurations>
            &connectionConfigurations,
            std::shared_ptr<Hypodermic::Container> container);

    /** Load virtual group configuration from the database. */
    void onStart() override;

    /** Returns the priority for ordering this plugin in the bootstrap sequence. */
    BootstrapSequence getPriority() override;
};

#endif  // CPP_BASE_LIBRARY_VIRTUALGROUPBOOTSTRAPPLUGIN_H
