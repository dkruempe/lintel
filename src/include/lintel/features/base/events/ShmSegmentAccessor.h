#ifndef LINTEL_SHMSEGMENTACCESSOR_H
#define LINTEL_SHMSEGMENTACCESSOR_H

#include <cstddef>

/**
 * Typed access to the allocator of a shared memory segment.
 *
 * The accessor is a small value type (one pointer) that keeps the boost
 * interprocess headers out of EventBus.h: objects stored in shared memory hold
 * a copy of the accessor, while the process local allocator it points to is
 * owned by SharedMemoryService. Only the raw block allocation itself is
 * untyped, everything above it stays typed.
 *
 * Copying is intentional: the copy lives inside the shared memory object and
 * therefore keeps a stable address for the whole lifetime of that object,
 * independent of the process that created it.
 */
class ShmSegmentAccessor
{
public:
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
  };

  /** @param allocator process local allocator of the segment */
  explicit ShmSegmentAccessor(Allocator &allocator) noexcept : m_allocator(&allocator) {}

  /**
   * @param bytes to allocate from the segment
   * @return pointer to the allocated block
   */
  [[nodiscard]] void *allocate(std::size_t bytes) const { return m_allocator->allocate(bytes); }

  /**
   * @param pointer block previously returned by allocate
   */
  void deallocate(void *pointer) const { m_allocator->deallocate(pointer); }

private:
  Allocator *m_allocator;
};

#endif// LINTEL_SHMSEGMENTACCESSOR_H
