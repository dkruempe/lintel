#ifndef LINTEL_TLSRELOADREGISTRY_H
#define LINTEL_TLSRELOADREGISTRY_H

#include <cstddef>
#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace lintel {
/**
 * Process-wide registry of reloadable resources, currently the TLS
 * certificate/key pair of the HTTP server.
 *
 * A certificate is loaded into the server's OpenSSL context once, when the
 * `httplib::SSLServer` is built. Replacing it on disk (for example when
 * certbot renews a Let's Encrypt certificate) therefore has no effect until the
 * process restarts. A controller cannot reach the server to change that: the
 * controller list is resolved from the container *before* the ServerProvider
 * that creates the server, so a controller holding the server would be a
 * dependency cycle. The server instead registers a reload handler here, and
 * `POST /admin/reload-tls` runs it.
 *
 * The registry is a process-wide singleton (the same pattern LoggerService and
 * ReadinessRegistry use). Registering a name that is already present replaces
 * the old handler, so a re-initialization stays idempotent - and it returns a
 * fresh id, because the replaced owner must not be able to unregister its
 * successor: a `ServerProvider` that builds one server after another would
 * otherwise strip the reload handler of the server it just created when the
 * previous instance is destroyed.
 *
 * Thread safety: registerHandler(), unregisterHandler(), clear() and reload()
 * may be called from any thread. The handlers run outside the registry lock, so
 * a reload that re-reads files does not block registration.
 */
class TlsReloadRegistry
{
public:
  /** Identifies one registration, for removal by its owner. */
  using ReloadId = std::size_t;

  /** Outcome of one named reload handler. */
  struct ReloadResult
  {
    /** the name the handler was registered under */
    std::string m_name;
    /** true when the handler reloaded its resource */
    bool m_reloaded = false;
    /** failure reason when m_reloaded is false, empty otherwise */
    std::string m_detail;
  };

  /** A reload handler.
   * @return an error description when the reload failed, an empty string on
   *         success */
  using Handler = std::function<std::string()>;

  /** @return the process-wide registry instance */
  static TlsReloadRegistry &instance();

  /** Register a named handler; an existing handler of the same name is
   *  replaced.
   * @param name    unique name, reported by `POST /admin/reload-tls`
   * @param handler the reload handler
   * @return the id of this registration, for unregisterHandler() */
  ReloadId registerHandler(std::string name, Handler handler);

  /** Remove the registration with @p id; no-op when it is gone.
   *
   * Takes the id rather than the name on purpose: when a name was registered
   * again by someone else, the earlier owner must not remove the new handler. */
  void unregisterHandler(ReloadId id);

  /** Remove every registered handler (used by tests). */
  void clear();

  /** Run all registered handlers in registration order.
   *
   * A handler that throws counts as failed with the exception message as
   * detail.
   * @return one result per registered handler */
  std::vector<ReloadResult> reload() const;

private:
  TlsReloadRegistry() = default;

  struct Entry
  {
    ReloadId m_id = 0;
    std::string m_name;
    Handler m_handler;
  };

  mutable std::mutex m_mutex;
  std::vector<Entry> m_entries;
  ReloadId m_nextId = 1;
};
}// namespace lintel
#endif// LINTEL_TLSRELOADREGISTRY_H
