#ifndef LINTEL_VIRTUALGROUPBOOTSTRAPPLUGIN_H
#define LINTEL_VIRTUALGROUPBOOTSTRAPPLUGIN_H

#include <memory>

#include "lintel/core/di/Container.h"

#include "lintel/core/persistence/DatabaseConnectionConfigurations.h"
#include "lintel/core/plugins/BootstrapPlugin.h"

namespace lintel {
/** Bootstrap plugin that initializes virtual process groups from the database. */
class VirtualGroupBootstrapPlugin : public BootstrapPlugin
{
private:
  /** Weak on purpose: the plugin is a singleton *inside* the container, so a
   *  shared_ptr here would close a reference cycle (container -> plugin ->
   *  container) and the container would never be destroyed - which in turn kept
   *  every service alive, so no service destructor ever ran. */
  std::weak_ptr<lintel::di::Container> m_container;
  std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;

public:
  /** Construct a VirtualGroupBootstrapPlugin.
   * @param connectionConfigurations database connection configurations
   * @param container                the dependency injection container */
  explicit VirtualGroupBootstrapPlugin(
    const std::shared_ptr<DatabaseConnectionConfigurations> &connectionConfigurations,
    std::shared_ptr<lintel::di::Container> container);

  /** Load virtual group configuration from the database. */
  void onStart() override;

  /** Returns the priority for ordering this plugin in the bootstrap sequence. */
  BootstrapSequence getPriority() override;
};

}// namespace lintel
#endif// LINTEL_VIRTUALGROUPBOOTSTRAPPLUGIN_H
