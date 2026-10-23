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
#include "lintel/core/shm/ShmObject.h"

namespace lintel {

class SharedMemoryService;

/** Backwards compatible name of the type-erased object binding.
 *
 * The binding used to be a dedicated ShmCell implementation; it is now
 * shm::ShmObject, which adds the stable ShmObjectHeader and payload
 * migrations. Existing code may keep using ShmCell.
 */
using ShmCell = shm::ShmObject;

namespace shm {

  /** @return true when the first type of a parameter pack is ShmObjectOptions */
  template<typename... Args> struct FirstIsObjectOptions : std::false_type
  {
  };
  template<typename First, typename... Rest>
  struct FirstIsObjectOptions<First, Rest...> : std::is_same<std::decay_t<First>, ShmObjectOptions>
  {
  };

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
   * in ShmObject / ShmObject.cpp. The on-disk object is a ShmObjectHeader plus
   * a separate payload allocation; see ShmObjectHeader.h for the layout and
   * migration contract.
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
    /** Find or create the cell holding one T with default options.
     *  @param service shared memory service of the segment
     *  @param segment segment the object lives in
     *  @param name    name of the object inside the segment
     *  @param args    constructor arguments, used only when the cell is created
     *  @throws std::invalid_argument on invalid arguments
     *  @throws ShmSegmentNotFound when the service does not know the segment */
    template<typename... Args>
      requires(!FirstIsObjectOptions<Args...>::value)
    SharedMemoryRepository(std::shared_ptr<SharedMemoryService> service,
      std::shared_ptr<SharedMemorySegment> segment,
      std::string name,
      Args &&...args)
      : SharedMemoryRepository(std::move(service),
          std::move(segment),
          std::move(name),
          ShmObjectOptions{},
          std::forward<Args>(args)...)
    {}

    /** Find or create the cell holding one T with explicit registration options.
     *
     * @param service  shared memory service of the segment
     * @param segment  segment the object lives in
     * @param name     name of the object inside the segment
     * @param options  payload version, persistence class and optional type id
     * @param args     constructor arguments, used only when the cell is created */
    template<typename... Args>
    SharedMemoryRepository(std::shared_ptr<SharedMemoryService> service,
      std::shared_ptr<SharedMemorySegment> segment,
      std::string name,
      ShmObjectOptions options,
      Args &&...args)
      : m_cell(std::move(service),
          std::move(segment),
          std::move(name),
          ShmCell::Layout{ sizeof(T), alignof(T) },
          options.payloadTypeId != 0 ? options.payloadTypeId : payloadTypeIdOf<T>(),
          options,
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
      requires(!FirstIsObjectOptions<Args...>::value)
    static std::shared_ptr<SharedMemoryRepository<T>> of(std::shared_ptr<SharedMemoryService> service,
      std::shared_ptr<SharedMemorySegment> segment,
      std::string name,
      Args &&...args)
    {
      return of(
        std::move(service), std::move(segment), std::move(name), ShmObjectOptions{}, std::forward<Args>(args)...);
    }

    /** Bind to the named cell with explicit registration options.
     *
     * @param service shared memory service of the segment
     * @param segment segment the object lives in
     * @param name    name of the object inside the segment
     * @param options payload version, persistence class and optional type id
     * @param args    constructor arguments, used only when the cell is created
     * @return the repository bound to the cell */
    template<typename... Args>
    static std::shared_ptr<SharedMemoryRepository<T>> of(std::shared_ptr<SharedMemoryService> service,
      std::shared_ptr<SharedMemorySegment> segment,
      std::string name,
      ShmObjectOptions options,
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
        std::move(service), std::move(segment), std::move(name), options, std::forward<Args>(args)...);
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

    /** @return the stable header of the object inside the segment */
    [[nodiscard]] const ShmObjectHeader &getHeader() const { return m_cell.header(); }

    /** @return persistence class recorded in the segment */
    [[nodiscard]] ShmPersistence getPersistence() const { return m_cell.persistence(); }

    /** @return payload version recorded in the segment */
    [[nodiscard]] std::uint16_t getPayloadVersion() const { return m_cell.payloadVersion(); }

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
