#ifndef LINTEL_ADMINUSERBOOTSTRAPPLUGIN_H
#define LINTEL_ADMINUSERBOOTSTRAPPLUGIN_H

#include <memory>

#include "lintel/core/persistence/DatabaseConnectionConfigurations.h"
#include "lintel/core/plugins/BootstrapPlugin.h"

/**
 * Bootstrap plugin that creates an initial admin user from environment
 * variables (ADMIN_USERNAME / ADMIN_PASSWORD) when no user with that name
 * exists yet. It never falls back to a default password: without
 * ADMIN_PASSWORD no user is created.
 *
 * Runs after the database schema is initialized but uses direct SQL because
 * repositories are not awakened yet at bootstrap time.
 */
class AdminUserBootstrapPlugin : public BootstrapPlugin {
private:
    std::shared_ptr<DatabaseConnectionConfigurations> m_connectionConfigurations;

public:
    /** Construct an AdminUserBootstrapPlugin.
     * @param connectionConfigurations database connection configurations */
    explicit AdminUserBootstrapPlugin(
            std::shared_ptr<DatabaseConnectionConfigurations>
                    connectionConfigurations);

    /** Create the initial admin user if ADMIN_PASSWORD is set. */
    void onStart() override;

    /** Runs after virtual groups are registered. */
    BootstrapSequence getPriority() override;
};

#endif  // LINTEL_ADMINUSERBOOTSTRAPPLUGIN_H
