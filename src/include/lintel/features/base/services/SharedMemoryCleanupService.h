#ifndef LINTEL_SHAREDMEMORYCLEANUPSERVICE_H
#define LINTEL_SHAREDMEMORYCLEANUPSERVICE_H

#include <chrono>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "lintel/core/platform/Platform.h"
#include "lintel/core/services/PropertyRegistration.h"

namespace lintel {
class DatabaseConnectionEntry;
class SchedulerService;
class SharedMemorySegment;
class SharedMemoryService;

namespace shm {
  /** Remove a named, trivially destructible object from a segment.
   *  @return true when the object is gone (removed now or already absent) */
  bool destroyNamedObject(SharedMemoryService &service,
    const std::shared_ptr<SharedMemorySegment> &segment,
    const std::string &name);
}// namespace shm

/** One object that was reported as orphaned by a cleanup scan. */
struct SharedMemoryCleanupObject
{
  std::string segmentName;
  std::string objectName;
  ProcessIdentity owner;
  std::chrono::system_clock::time_point lastHeartbeat;
  std::string reason;
};

/** Result of a scan() or apply() run. */
struct SharedMemoryCleanupReport
{
  /** true for scan(), false for apply() */
  bool dryRun = false;
  /** number of registry entries inspected */
  std::size_t scanned = 0;
  /** orphaned objects found by this run */
  std::vector<SharedMemoryCleanupObject> orphans;
  /** orphaned objects actually removed by this run (empty for scan()) */
  std::vector<SharedMemoryCleanupObject> removed;
  /** removals that failed; the entries stay registered and are retried */
  std::size_t failed = 0;

  /** @return true when no orphan was found */
  [[nodiscard]] bool empty() const { return orphans.empty(); }

  /** @return number of orphans found */
  [[nodiscard]] std::size_t orphanCount() const { return orphans.size(); }
};

/** Cumulative counters of the cleanup service. */
struct SharedMemoryCleanupStats
{
  std::size_t scans = 0;
  std::size_t applies = 0;
  std::size_t orphans = 0;
  std::size_t removed = 0;
  std::size_t failures = 0;
  std::chrono::system_clock::time_point lastRun;
};

/**
 * Automatic cleanup of orphaned shared memory objects.
 *
 * A process registers the objects it creates with the cleanup service and
 * records the owning process identity. The service periodically checks those
 * registry entries and reports resp. removes the ones whose owner is gone.
 * Liveness is decided by ProcessIdentity (pid plus start ticks), so a recycled
 * pid never makes a dead owner look alive - and a live owner is never touched,
 * even when its heartbeat is old. That is the same fail-safe rule the segment
 * lock recovery uses.
 *
 * scan() is the dry-run mode: it only reports. apply() removes the reported
 * objects through their registered remover and drops the registry entry, which
 * makes repeated runs idempotent (the second run finds nothing to do). A failed
 * removal keeps its entry so the next run retries it.
 *
 * The service only removes objects it knows about. Automatic discovery of every
 * named object in a segment is deliberately not attempted: removing a mapped
 * object requires the type that created it, which a foreign process does not
 * have.
 */
class SharedMemoryCleanupService
  : public PropertyRegistration<SharedMemoryCleanupService>
  , public std::enable_shared_from_this<SharedMemoryCleanupService>
{
public:
  /** Callable that removes one registered object from its segment.
   *  @return true when the object is gone (removed now or already absent) */
  using RemoveFunction = std::function<bool()>;

  /**
   * Construct the cleanup service.
   * @param scheduler          scheduler used for the periodic run, may be null
   * @param sharedMemoryService service owning the segments of the registered objects
   * @param databaseConnection optional default database connection; when set,
   *                           matching shared_memory_repositories rows are removed
   *                           together with an orphaned object
   * @param interval           period between automatic runs; zero disables the
   *                           scheduled run (on-demand only)
   */
  SharedMemoryCleanupService(const std::shared_ptr<SchedulerService> &scheduler,
    std::shared_ptr<SharedMemoryService> sharedMemoryService,
    std::shared_ptr<DatabaseConnectionEntry> databaseConnection = nullptr,
    std::chrono::seconds interval = std::chrono::seconds(60));

  ~SharedMemoryCleanupService() override = default;

  /** Schedule the periodic cleanup run. A zero interval leaves the service
   *  on-demand only. */
  void onInitialize() override;

  /**
   * Report orphaned objects without removing anything.
   * @return the dry-run report
   */
  SharedMemoryCleanupReport scan();

  /**
   * Remove orphaned objects and reconcile their registry entries.
   * @return the applied report
   */
  SharedMemoryCleanupReport apply();

  /** @return the configured automatic interval */
  [[nodiscard]] std::chrono::seconds interval() const;

  /** @param interval new automatic interval, zero disables it */
  void setInterval(std::chrono::seconds interval);

  /** @return true when the scheduled run applies instead of only reporting */
  [[nodiscard]] bool applyScheduled() const;

  /** @param apply true to remove orphans in the scheduled run, false to only report */
  void setApplyScheduled(bool apply);

  /**
   * Register an object for orphan detection.
   * @param segmentName   segment containing the object
   * @param objectName    name of the object inside the segment
   * @param owner         identity of the owning process
   * @param remove        callable removing the object
   * @param lastHeartbeat last known sign of life of the owner
   * @return true when the entry was added or replaced
   */
  bool trackObject(const std::string &segmentName,
    const std::string &objectName,
    const ProcessIdentity &owner,
    RemoveFunction remove,
    std::chrono::system_clock::time_point lastHeartbeat = std::chrono::system_clock::now());

  /**
   * Register a trivially destructible named object and remove it by name.
   *
   * @tparam T element type of the object; must be trivially destructible so
   *           the type-erased removal is safe
   * @param service    shared memory service owning the segment
   * @param segment    segment containing the object
   * @param objectName name of the object inside the segment
   * @param owner      identity of the owning process
   * @param lastHeartbeat last known sign of life of the owner
   * @return true when the entry was added or replaced
   */
  template<typename T>
  bool trackObject(const std::shared_ptr<SharedMemoryService> &service,
    const std::shared_ptr<SharedMemorySegment> &segment,
    const std::string &objectName,
    const ProcessIdentity &owner,
    std::chrono::system_clock::time_point lastHeartbeat = std::chrono::system_clock::now())
  {
    static_assert(std::is_trivially_destructible_v<T>,
      "automatic cleanup can only remove trivially destructible named objects; "
      "register a custom remover for a type with a destructor");
    if (service == nullptr || segment == nullptr) { return false; }
    return trackObject(
      segment->getName(),
      objectName,
      owner,
      [service, segment, objectName]() { return shm::destroyNamedObject(*service, segment, objectName); },
      lastHeartbeat);
  }

  /**
   * Refresh the heartbeat of a registered object.
   * @return true when the entry exists
   */
  bool touchObject(const std::string &segmentName,
    const std::string &objectName,
    std::chrono::system_clock::time_point lastHeartbeat = std::chrono::system_clock::now());

  /**
   * Remove an object from the registry without touching the segment.
   * @return true when an entry was removed
   */
  bool untrackObject(const std::string &segmentName, const std::string &objectName);

  /** @return number of registered objects */
  [[nodiscard]] std::size_t size() const;

  /** @return cumulative counters */
  [[nodiscard]] SharedMemoryCleanupStats stats() const;

private:
  /** One registry entry. */
  struct Entry
  {
    std::string segmentName;
    std::string objectName;
    ProcessIdentity owner;
    std::chrono::system_clock::time_point lastHeartbeat;
    RemoveFunction remove;
  };

  [[nodiscard]] static std::string keyOf(const std::string &segmentName, const std::string &objectName);
  /** @return true when the entry's owner is provably gone */
  [[nodiscard]] static bool isOrphaned(const Entry &entry, SharedMemoryCleanupObject &orphan);
  /** Scan the registry; caller must not hold m_mutex. */
  [[nodiscard]] SharedMemoryCleanupReport run(bool apply);
  /** Delete matching rows from shared_memory_repositories, best effort. */
  void reconcileDatabase(const SharedMemoryCleanupObject &object) noexcept;

  std::shared_ptr<SchedulerService> m_scheduler;
  std::shared_ptr<SharedMemoryService> m_sharedMemoryService;
  std::shared_ptr<DatabaseConnectionEntry> m_databaseConnection;
  mutable std::mutex m_mutex;
  std::map<std::string, Entry> m_entries;
  std::chrono::seconds m_interval;
  bool m_applyScheduled = true;
  mutable SharedMemoryCleanupStats m_stats;
};

}// namespace lintel
#endif// LINTEL_SHAREDMEMORYCLEANUPSERVICE_H
