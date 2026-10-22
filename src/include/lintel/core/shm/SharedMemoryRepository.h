#ifndef LINTEL_CORE_SHM_SHAREDMEMORYREPOSITORY_H
#define LINTEL_CORE_SHM_SHAREDMEMORYREPOSITORY_H

#include <cstddef>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "lintel/core/models/SharedMemorySegment.h"
#include "lintel/core/shm/SharedMemoryRepositoryRegistry.h"

namespace lintel {

class SharedMemoryService;

/**
 * Process local binding of one named cell inside a shared memory segment.
 *
 * A cell consists of two parts, both living inside the segment:
 * - a fixed size record that the segment manager can find by name (boost
 *   interprocess find_or_construct), and
 * - a payload of `payloadSize` bytes that the record allocated from the
 *   segment through ShmSegmentAccessor while the cell was created.
 *
 * The record refers to its payload by a byte offset relative to itself.
 * Both addresses shift by the same amount in every mapping of the segment,
 * so the relative layout is identical in every process and each one resolves
 * the same payload regardless of the address it mapped the segment at (the
 * same pattern EventBus::ShmRing uses for its node buffer).
 *
 * The boost interprocess types stay out of this header: finding/constructing
 * the record, allocating the payload and reading the offsets is done by the
 * out-of-line methods in SharedMemoryRepository.cpp. The header only needs
 * ShmSegmentAccessor-sized building blocks - see ShmSegmentAccessor.h for the
 * same "keep boost in one .cpp" approach.
 *
 * A cell lives as long as its segment: neither the record nor the payload is
 * destroyed when the binding goes out of scope, and T's destructor is never
 * called by the repository (T therefore has to tolerate being torn down with
 * the segment file, as every shared memory type does).
 *
 * The binding is not internally synchronized: concurrent access to one cell
 * from several threads of this process needs external locking. Across
 * processes the segment's construct lock serializes the single creation of
 * the record; everything above it is up to the payload type (lock-free
 * containers such as RingBufferShm synchronize themselves).
 */
class ShmCell
{
public:
  /** Highest payload alignment the relative addressing supports. A mapping
   *  base is page aligned, so an offset stays comparable across processes
   *  only while the payload alignment does not exceed one page. */
  static constexpr std::size_t kMaxAlignment = 4096;

  /** Payload size and alignment of a cell. Grouping the two same-typed sizes
   *  keeps them from being swapped at every construction and lookup. */
  struct Layout
  {
    /** payload size in bytes (> 0) */
    std::size_t size = 0;
    /** payload alignment in bytes (power of two, <= kMaxAlignment) */
    std::size_t alignment = 1;
  };

  /** Type-erased pointer to the payload constructor's arguments. Wrapping the
   *  raw pointer in a distinct type keeps the two pointer parameters of
   *  InitFunction from being swapped by mistake. */
  struct InitArgument
  {
    const void *value = nullptr;
  };

  /** Runs the payload constructor of T. Invoked exactly once - by the process
   *  that creates the cell, while the segment's construct lock is held - and
   *  never by a process that only finds the cell again.
   *  @param payload      raw, not yet constructed payload memory
   *  @param initArgument type erased constructor arguments; the binding keeps
   *                      them alive for as long as it exists */
  using InitFunction = void (*)(void *payload, InitArgument initArgument);

  /** Bind to the named cell without touching the segment yet; the first
   *  resolve() finds the cell or creates it (record + payload constructor).
   *  @param service       shared memory service owning the segment
   *  @param segment       segment the cell lives in
   *  @param name          name of the cell inside the segment
   *  @param layout        payload size and alignment
   *  @param init          payload constructor, run when the cell is created
   *  @param initArgument  constructor arguments for init, kept alive by this binding
   *  @throws std::invalid_argument on null service/segment, empty name, empty
   *          payload, unsupported alignment or missing constructor */
  ShmCell(std::shared_ptr<SharedMemoryService> service,
    std::shared_ptr<SharedMemorySegment> segment,
    std::string name,
    Layout layout,
    InitFunction init,
    std::shared_ptr<void> initArgument);

  ShmCell(const ShmCell &) = delete;
  ShmCell &operator=(const ShmCell &) = delete;

  /** Locate the payload, creating the cell on first use and re-reading the
   *  record after the segment was remapped (generation change).
   *  @return pointer to the payload, valid until the next remap of the segment */
  [[nodiscard]] void *resolve() const;

  /** @return true when the segment was remapped since the last resolve(),
   *  i.e. the cached record address has to be refreshed */
  [[nodiscard]] bool isStale() const;

  /** @return the service generation observed by the last resolve() */
  [[nodiscard]] std::size_t generation() const;

  /** @return name of the cell inside the segment */
  [[nodiscard]] const std::string &getName() const { return m_name; }

  /** @return segment the cell lives in */
  [[nodiscard]] const std::shared_ptr<SharedMemorySegment> &getSegment() const { return m_segment; }

private:
  std::shared_ptr<SharedMemoryService> m_service;
  std::shared_ptr<SharedMemorySegment> m_segment;
  std::string m_name;
  std::size_t m_payloadSize;
  std::size_t m_alignment;
  InitFunction m_init;
  std::shared_ptr<void> m_initArgument;
  /** Record inside the segment; opaque because its type (ShmCellRecord) is an
   *  implementation detail of SharedMemoryRepository.cpp. Mutable, because
   *  resolving refreshes the cache from const read access. */
  mutable void *m_record = nullptr;
  mutable std::size_t m_generation = 0;
};

namespace shm {

  /**
   * Typed repository for a single object of type T inside a shared memory
   * segment (Object/Array/RingBuffer/Queue storage).
   *
   * The repository implements find_or_construct semantics: the first process
   * that asks for the name constructs T from the given constructor arguments,
   * every later process attaches to the existing object and ignores them. All
   * access goes through get()/set()/update(), which re-resolve the payload as
   * soon as SharedMemoryService reports a remap through its generation
   * counter, so the repository survives a grow/shrink of the segment (SHM-3
   * hooks bump that generation). References returned by get() are only valid
   * until the next remap - ask again after isStale() turned true.
   *
   * The class lives in namespace lintel::shm because the flat name
   * lintel::SharedMemoryRepository is taken by the JSON serializing repository
   * base class of features/base/repositories/SharedMemoryRepository.h.
   *
   * The header is boost free: the boost interprocess find_or_construct lives
   * in ShmCell / SharedMemoryRepository.cpp.
   *
   * Requirements on T: usable in shared memory (no process local pointers, no
   * throwing destructor), identical in every process that uses the same cell
   * name, and alignof(T) <= ShmCell::kMaxAlignment.
   *
   * @tparam T payload type
   */
  template<typename T> class SharedMemoryRepository
  {
    static_assert(alignof(T) <= ShmCell::kMaxAlignment,
      "the payload alignment of a shared memory repository must not exceed one page");

  public:
    /** Find or create the cell holding one T.
     *  @param service shared memory service of the segment
     *  @param segment segment the object lives in
     *  @param name    name of the object inside the segment
     *  @param args    constructor arguments, used only when the cell is created
     *  @throws std::invalid_argument on invalid arguments
     *  @throws ShmSegmentNotFound when the service does not know the segment */
    template<typename... Args>
    SharedMemoryRepository(std::shared_ptr<SharedMemoryService> service,
      std::shared_ptr<SharedMemorySegment> segment,
      std::string name,
      Args &&...args)
      : m_cell(std::move(service),
          std::move(segment),
          std::move(name),
          ShmCell::Layout{ sizeof(T), alignof(T) },
          &SharedMemoryRepository<T>::template construct<Args...>,
          std::make_shared<std::tuple<std::decay_t<Args>...>>(std::forward<Args>(args)...))
    {
      // Resolve eagerly: an unusable segment or an already existing cell with
      // a different payload layout is a construction error, not a surprise
      // on the first read.
      static_cast<void>(m_cell.resolve());
    }

    SharedMemoryRepository(const SharedMemoryRepository &) = delete;
    SharedMemoryRepository &operator=(const SharedMemoryRepository &) = delete;

    /** Bind to the named cell, reusing the instance this process already holds.
     *
     * The registry key is `<segment name>:<cell name>`, so a second call with
     * the same segment and cell name returns the instance created first: the
     * segment keeps one cell and one payload instead of a second binding - and
     * a second ShmCell - to the same name. The registry keeps the returned
     * repository alive; remove() it from SharedMemoryRepositoryRegistry to
     * release it again.
     *
     * @param service shared memory service of the segment
     * @param segment segment the object lives in
     * @param name    name of the object inside the segment
     * @param args    constructor arguments, used only when the cell is created
     * @return the repository bound to the cell
     * @throws std::invalid_argument on invalid arguments, or when the key is
     *         already registered with another payload type
     * @throws ShmSegmentNotFound when the service does not know the segment */
    template<typename... Args>
    static std::shared_ptr<SharedMemoryRepository<T>> of(std::shared_ptr<SharedMemoryService> service,
      std::shared_ptr<SharedMemorySegment> segment,
      std::string name,
      Args &&...args)
    {
      if (service == nullptr || segment == nullptr) {
        throw std::invalid_argument("shared memory repository factory needs a service and a segment");
      }
      const std::string key = segment->getName() + ":" + name;
      SharedMemoryRepositoryRegistry &registry = SharedMemoryRepositoryRegistry::instance();
      // Throws std::invalid_argument when the key holds another payload type:
      // handing that instance out would fail somewhere inside get()/set().
      std::shared_ptr<SharedMemoryRepository<T>> existing = registry.getAs<SharedMemoryRepository<T>>(key);
      if (existing != nullptr) { return existing; }
      std::shared_ptr<SharedMemoryRepository<T>> repository = std::make_shared<SharedMemoryRepository<T>>(
        std::move(service), std::move(segment), std::move(name), std::forward<Args>(args)...);
      registry.registerRepository(key, repository);
      return repository;
    }

    /** @return const reference to the object in shared memory; the reference
     *  is re-resolved when the segment was remapped */
    [[nodiscard]] const T &get() const { return *static_cast<const T *>(m_cell.resolve()); }

    /** @return mutable reference to the object in shared memory; the
     *  reference is re-resolved when the segment was remapped */
    [[nodiscard]] T &get() { return *static_cast<T *>(m_cell.resolve()); }

    /** Replace the object.
     *  @param value the new value */
    void set(const T &value) { *static_cast<T *>(m_cell.resolve()) = value; }

    /** Apply a modification to the object in place.
     *  @tparam Fn callable invoked as fn(T &)
     *  @param fn the modification
     *  @return whatever fn returns (void is allowed) */
    template<typename Fn> auto update(Fn &&fn) -> decltype(auto) { return fn(*static_cast<T *>(m_cell.resolve())); }

    /** @return true when the segment was remapped since the last access */
    [[nodiscard]] bool isStale() const { return m_cell.isStale(); }

    /** @return the service generation observed by the last access */
    [[nodiscard]] std::size_t generation() const { return m_cell.generation(); }

    /** @return name of the object inside the segment */
    [[nodiscard]] const std::string &getName() const { return m_cell.getName(); }

    /** @return segment the object lives in */
    [[nodiscard]] const std::shared_ptr<SharedMemorySegment> &getSegment() const { return m_cell.getSegment(); }

  private:
    /** Placement-construct the payload from the stored constructor arguments.
     *
     *  Unpacking through an index sequence instead of a variadic lambda keeps
     *  the argument names out of the scope of construct(): CodeQL reports the
     *  lambda's parameter pack as hiding the repository's constructor
     *  parameter pack, regardless of how it is named.
     *  @tparam Arguments decayed tuple type of the constructor arguments
     *  @param payload   raw payload memory of the cell
     *  @param arguments stored constructor arguments
     *  @param indices   index sequence selecting the tuple elements */
    template<typename Arguments, std::size_t... Indices>
    static void constructFromTuple(void *payload, const Arguments &arguments, std::index_sequence<Indices...>)
    {
      ::new (payload) T(std::get<Indices>(arguments)...);
    }

    /** Payload constructor handed to the cell as a plain function pointer.
     *  @param payload      raw payload memory of the cell
     *  @param initArgument the tuple of constructor arguments this repository
     *                      built at construction time */
    template<typename... Args> static void construct(void *payload, ShmCell::InitArgument initArgument)
    {
      using Arguments = std::tuple<std::decay_t<Args>...>;
      const auto &arguments = *static_cast<const Arguments *>(initArgument.value);
      constructFromTuple(payload, arguments, std::index_sequence_for<Args...>{});
    }

    ShmCell m_cell;
  };

}// namespace shm
}// namespace lintel
#endif// LINTEL_CORE_SHM_SHAREDMEMORYREPOSITORY_H
