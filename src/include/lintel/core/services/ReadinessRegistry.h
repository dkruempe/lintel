#ifndef LINTEL_READINESSREGISTRY_H
#define LINTEL_READINESSREGISTRY_H

#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace lintel {
/**
 * Process-wide registry of the dependency checks behind `GET /ready`.
 *
 * Liveness (`GET /health`) is answered by the endpoint itself and must never
 * consult a dependency. Readiness is different: it reports whether the process
 * is able to do its work, so it has to ask the dependencies (database, shared
 * memory, ...). The component that owns such a dependency registers a named
 * check once during startup; the HTTP layer runs every registered check per
 * readiness request and turns the results into 200 or 503.
 *
 * The registry is a process-wide singleton (the same pattern LoggerService
 * uses): checks are registered from components in different layers
 * (persistence plugins, feature services) that share no DI scope with the HTTP
 * server, which is built from configuration alone. Registering a name that is
 * already present replaces the old check, so a re-initialization stays
 * idempotent.
 *
 * Thread safety: registerCheck(), clear() and run() may be called from any
 * thread. The checks themselves run outside the registry lock - a slow
 * dependency must not block other components from registering, and a check may
 * take locks of its own subsystem.
 */
class ReadinessRegistry
{
public:
  /** Outcome of one named check. */
  struct CheckResult
  {
    /** the name the check was registered under (JSON key of `GET /ready`) */
    std::string m_name;
    /** true when the dependency answered and is usable */
    bool m_ready = false;
    /** failure reason when m_ready is false, empty otherwise */
    std::string m_detail;
  };

  /** A dependency check.
   * @param detail set to a human readable reason when returning false
   * @return true when the dependency is ready */
  using Check = std::function<bool(std::string &)>;

  /** @return the process-wide registry instance */
  static ReadinessRegistry &instance();

  /** Register a named check; an existing check of the same name is replaced.
   * @param name  unique name, reported as JSON key by `GET /ready`
   * @param check the dependency check */
  void registerCheck(std::string name, Check check);

  /** Remove every registered check (used by tests). */
  void clear();

  /** Run all registered checks in registration order.
   *
   * A check that throws counts as failed with the exception message as detail.
   * @return one result per registered check */
  std::vector<CheckResult> run() const;

private:
  ReadinessRegistry() = default;

  mutable std::mutex m_mutex;
  std::vector<std::pair<std::string, Check>> m_checks;
};
}// namespace lintel
#endif// LINTEL_READINESSREGISTRY_H
