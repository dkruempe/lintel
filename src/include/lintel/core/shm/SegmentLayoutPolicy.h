#ifndef LINTEL_SEGMENTLAYOUTPOLICY_H
#define LINTEL_SEGMENTLAYOUTPOLICY_H

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string_view>
#include <utility>

namespace lintel {
namespace shm {

  /** What a process does when it opens a segment written with another object layout. */
  enum class LayoutMismatchPolicy : std::uint8_t {
    /** Refuse to start and name the file - the default, because only the operator
     *  knows whether the segment still holds something that is needed. */
    Fail,
    /** Discard the contents and start with an empty segment. */
    Recreate,
    /** Ask the registered migrator first; it decides whether the contents may be
     *  discarded. */
    Migrate,
  };

  /**
   * Migration hook for a stale shared memory segment.
   *
   * Called with the path of the segment **as it currently is on disk**, before
   * anything is removed, plus the layout version that was found and the version
   * this build writes. The migrator carries whatever it needs into a durable
   * place (a file, the database, a new named object) and returns true once the
   * old contents are expendable; returning false aborts the start.
   *
   * Typical use: export the payloads of the typed repositories
   * (`shm::SharedMemoryRepository<T>`), which are the only objects in a segment
   * that hold data a program may not be able to recompute.
   */
  using SegmentMigrator = std::function<
    bool(const std::filesystem::path &staleSegment, std::uint64_t foundVersion, std::uint64_t currentVersion)>;

  /**
   * Process wide layout policy for shared memory segments.
   *
   * Set it once during startup, before the `SharedMemoryService` is constructed:
   *
   * @code
   * lintel::shm::SegmentLayoutPolicy::setMigrator([](const auto &path, auto from, auto to) {
   *   // read the old segment, write what matters somewhere durable
   *   return true;
   * });
   * @endcode
   *
   * The default is LayoutMismatchPolicy::Fail, so a process never discards data
   * on its own. `LINTEL_SHM_RECREATE=1` in the environment selects Recreate
   * without recompiling, which is what a developer machine usually wants.
   *
   * Not synchronized: like MessageQueueBackendConfig this is startup wiring, not
   * a runtime switch. Call it before the container starts.
   */
  class SegmentLayoutPolicy
  {
  public:
    /** Select the policy explicitly.
     *
     * @param policy what to do on a layout mismatch. Migrate without a registered
     *        migrator behaves like Fail. */
    static void set(LayoutMismatchPolicy policy) noexcept { state().policy = policy; }

    /** Register a migrator and select LayoutMismatchPolicy::Migrate.
     *
     * @param migrator the hook; an empty function selects Fail instead */
    static void setMigrator(SegmentMigrator migrator)
    {
      state().migrator = std::move(migrator);
      state().policy = state().migrator != nullptr ? LayoutMismatchPolicy::Migrate : LayoutMismatchPolicy::Fail;
    }

    /** @return the configured policy */
    [[nodiscard]] static LayoutMismatchPolicy policy() noexcept { return state().policy; }

    /** @return true when a migrator is registered */
    [[nodiscard]] static bool hasMigrator() { return static_cast<bool>(state().migrator); }

    /**
     * Run the registered migrator.
     *
     * @param staleSegment    the segment file as it is on disk, still untouched
     * @param foundVersion    layout version found in the segment
     * @param currentVersion  layout version this build writes
     * @return true when the old contents may be discarded; false to abort the start
     */
    static bool
      migrate(const std::filesystem::path &staleSegment, std::uint64_t foundVersion, std::uint64_t currentVersion)
    {
      if (!state().migrator) { return false; }
      return state().migrator(staleSegment, foundVersion, currentVersion);
    }

    /** Restore the defaults. Intended for tests. */
    static void reset() noexcept { state() = State{}; }

  private:
    /** Process wide policy state. */
    struct State
    {
      LayoutMismatchPolicy policy = LayoutMismatchPolicy::Fail;
      SegmentMigrator migrator;
    };

    /** @return the policy of this process; a function local static is shared by
     *  every translation unit */
    static State &state()
    {
      static State instance;
      return instance;
    }
  };

}// namespace shm
}// namespace lintel
#endif// LINTEL_SEGMENTLAYOUTPOLICY_H
