#ifndef LINTEL_SHMSEGMENTACCESSOR_H
#define LINTEL_SHMSEGMENTACCESSOR_H

#include <cstddef>
#include <cstring>
#include <functional>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>

namespace lintel {
/**
 * Typed access to the allocator of a shared memory segment.
 *
 * The accessor is a small value type that keeps the boost interprocess headers
 * out of EventBus.h: objects stored in shared memory hold a copy of the
 * accessor, while the process local allocator it points to is owned by
 * SharedMemoryService. Only the raw block allocation itself is untyped,
 * everything above it stays typed.
 *
 * Copying is intentional: the copy lives inside the shared memory object and
 * therefore keeps a stable address for the whole lifetime of that object.
 *
 * A pointer alone cannot survive the process boundary: the copy inside the
 * segment would point at the heap of the process that created the object, and
 * that address means nothing in any other process (it may not even be mapped
 * there). The accessor therefore carries the *name* of its segment, which is
 * stable across processes, and resolves that name to a process local allocator
 * on every allocation. `bind()` is what fills the process local side; it is
 * called by SharedMemoryService whenever it hands out an accessor.
 *
 * An accessor constructed without a name (the single argument constructor)
 * keeps working on the allocator it was built with. That is the right thing
 * for objects that never leave the process - a segment used as a plain buffer,
 * tests and benchmarks - and it is what the named form falls back to when the
 * name was never bound in this process.
 */
class ShmSegmentAccessor
{
public:
  /** Capacity of the stored segment name in bytes, including the terminating NUL. */
  static constexpr std::size_t NAME_SIZE = 32;

  /** Process local allocator an accessor forwards to. */
  class Allocator
  {
  public:
    virtual ~Allocator() = default;

    /**
     * @param bytes to allocate
     * @return pointer to the allocated block
     */
    virtual void *allocate(std::size_t bytes) = 0;

    /**
     * @param pointer block previously returned by allocate
     */
    virtual void deallocate(void *pointer) = 0;

    /**
     * Find or construct a named byte array inside the segment.
     *
     * The name is the cross-process identity of the object: the first caller
     * creates the array, every later caller - in this or in any other process
     * - gets the same bytes back. A fresh array is value initialised, so it
     * starts zeroed.
     *
     * The default implementation does not support named objects and reports
     * that with nullptr, which keeps the allocator interface usable for
     * segment stand-ins that only need allocate/deallocate.
     *
     * @param name  name of the array inside the segment
     * @param bytes size of the array in bytes (must be positive)
     * @return pointer to the array, or nullptr when named objects are not supported
     */
    virtual void *findOrConstructArray(const char * /*name*/, std::size_t /*bytes*/) { return nullptr; }
  };

  /**
   * Construct an accessor that is bound to its allocator only, without a
   * segment name. The resulting accessor is process local: a copy stored in
   * the segment is not usable from another process.
   *
   * @param allocator process local allocator of the segment
   */
  explicit ShmSegmentAccessor(Allocator &allocator) noexcept : m_allocator(&allocator) {}

  /**
   * Construct an accessor that identifies its segment by name.
   *
   * The name is stored in the accessor (and therefore in every copy that ends
   * up inside the segment) so that another process can resolve it to its own
   * allocator. The name is not bound here; call bind() for that, which
   * SharedMemoryService does before it hands the accessor out.
   *
   * @param segmentName name of the shared memory segment
   * @param allocator   process local allocator of that segment
   * @throws std::invalid_argument if the name does not fit into NAME_SIZE bytes
   */
  ShmSegmentAccessor(std::string_view segmentName, Allocator &allocator) : m_allocator(&allocator)
  {
    validateName(segmentName);
    std::memcpy(m_segmentName, segmentName.data(), segmentName.size());
    m_segmentName[segmentName.size()] = '\0';
  }

  /**
   * Bind a segment name to the allocator of this process.
   *
   * Every accessor that carries this name resolves it to @p allocator, no
   * matter which process created the accessor or how old the copy is. Binding
   * again replaces the previous allocator, which is what a remapped segment
   * needs: the old allocator belongs to the closed mapping.
   *
   * The registry lives for the whole process; @p allocator has to outlive
   * every accessor that is used in this process, exactly like the allocator of
   * an unnamed accessor.
   *
   * @param segmentName name of the shared memory segment
   * @param allocator   this process' allocator of that segment
   * @throws std::invalid_argument if the name does not fit into NAME_SIZE bytes
   */
  static void bind(std::string_view segmentName, Allocator &allocator)
  {
    validateName(segmentName);
    std::lock_guard<std::mutex> lock(registry().mutex);
    registry().allocators.insert_or_assign(std::string(segmentName), &allocator);
  }

  /**
   * @param bytes to allocate from the segment
   * @return pointer to the allocated block
   */
  [[nodiscard]] void *allocate(std::size_t bytes) const { return allocator()->allocate(bytes); }

  /**
   * @param pointer block previously returned by allocate
   */
  void deallocate(void *pointer) const { allocator()->deallocate(pointer); }

  /**
   * Find or construct a named byte array in the segment of this accessor.
   *
   * @param name  name of the array inside the segment
   * @param bytes size of the array in bytes
   * @return pointer to the array, or nullptr when the allocator does not support named objects
   */
  [[nodiscard]] void *findOrConstructArray(const char *name, std::size_t bytes) const
  {
    return allocator()->findOrConstructArray(name, bytes);
  }

  /** @return the segment name this accessor identifies its segment by; empty for an unnamed accessor */
  [[nodiscard]] std::string_view segmentName() const noexcept { return std::string_view(m_segmentName); }

private:
  /** Process wide name -> allocator map, see bind(). */
  struct Registry
  {
    std::mutex mutex;
    /** std::less<> makes the lookup accept a string_view without building a temporary string. */
    std::map<std::string, Allocator *, std::less<>> allocators;
  };

  /** @return the registry of this process; a function local static is shared by every translation unit */
  static Registry &registry()
  {
    static Registry instance;
    return instance;
  }

  /** @param segmentName name to check against NAME_SIZE
   *  @throws std::invalid_argument if the name does not fit, including its NUL */
  static void validateName(std::string_view segmentName)
  {
    if (segmentName.size() < NAME_SIZE) { return; }
    throw std::invalid_argument("shared memory segment name '" + std::string(segmentName)
                                + "' is too long: a ShmSegmentAccessor stores at most " + std::to_string(NAME_SIZE - 1)
                                + " characters");
  }

  /**
   * Allocator to forward to: the one bound to this accessor's segment name in
   * this process, otherwise the one captured at construction time.
   *
   * @return allocator of this process
   */
  [[nodiscard]] Allocator *allocator() const
  {
    if (m_segmentName[0] == '\0') { return m_allocator; }
    std::lock_guard<std::mutex> lock(registry().mutex);
    const auto found = registry().allocators.find(std::string_view(m_segmentName));
    if (found == registry().allocators.end()) { return m_allocator; }
    return found->second;
  }

  Allocator *m_allocator;
  /** Name of the segment this accessor belongs to; empty for an unnamed accessor. */
  char m_segmentName[NAME_SIZE]{};
};

}// namespace lintel
#endif// LINTEL_SHMSEGMENTACCESSOR_H
