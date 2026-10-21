#ifndef LINTEL_SHMVIEW_H
#define LINTEL_SHMVIEW_H

#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace lintel {
class SharedMemoryService;
class SharedMemorySegment;

namespace shm {
  /**
   * Locate an existing named object in a segment without knowing its type.
   *
   * The object is looked up as a byte range, so the caller only has to cast the
   * returned address to the type it expects. The returned pointer refers to the
   * current mapping; a remap of the segment invalidates it.
   *
   * @param service the shared memory service owning the segment
   * @param segment the segment to search
   * @param name    the name of the object inside the segment
   * @return the address of the object, or nullptr when the name is not present
   * @throws ShmSegmentNotFound when the service does not know the segment
   */
  void *findNamedObject(SharedMemoryService &service,
    const std::shared_ptr<SharedMemorySegment> &segment,
    const std::string &name);

  /**
   * Opaque segment generation token. Wrapping the raw counter in a distinct
   * type keeps it from being swapped with a plain element count wherever a
   * view is constructed.
   */
  struct ShmGeneration
  {
    std::size_t value = 0;
  };

  /** @return the current generation of the service, kInvalid when it is gone */
  std::size_t generationOf(const std::weak_ptr<SharedMemoryService> &service) noexcept;

  /** @return the current generation of the service */
  std::size_t generationOf(const std::shared_ptr<SharedMemoryService> &service) noexcept;
}// namespace shm

/**
 * Non-owning view into a shared memory segment.
 *
 * A ShmView is the span-like counterpart of shm::SharedMemoryRepository: it
 * points directly into a mapped segment (no copy, no allocation) and carries
 * the segment generation the pointer was obtained from. A grow/shrink of the
 * segment remaps the file, which invalidates every pointer - including the one
 * inside this view. The generation lets callers detect that without touching
 * the stale memory:
 *
 * @code
 * auto view = shm::viewOf<int>(service, segment, "values", 4);
 * if (view.isStale()) { view = shm::viewOf<int>(service, segment, "values", 4); }
 * @endcode
 *
 * Lifecycle hooks (SegmentLifecycle::OnRemap) are the notification side of the
 * same mechanism; a view never re-resolves itself, because only the caller
 * knows the object name and type it wants. It is therefore a lightweight read
 * handle, not a repository.
 *
 * A view without a generation provider reports valid() as long as it points at
 * something; use valid(currentGeneration) or a provider-backed view (created by
 * shm::viewOf) to get remap detection.
 *
 * @tparam T element type; may be const for a read-only view
 */
template<typename T> class ShmView
{
public:
  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using pointer = T *;
  using reference = T &;
  using iterator = T *;
  using const_iterator = const value_type *;
  using size_type = std::size_t;
  /** Callable returning the current generation of the viewed segment. */
  using GenerationProvider = std::function<std::size_t()>;

  /** Sentinel a provider returns when the service behind the view is gone. */
  static constexpr std::size_t kInvalidGeneration = std::numeric_limits<std::size_t>::max();

  /** Construct an empty (invalid) view. */
  ShmView() noexcept = default;

  /**
   * Construct a view without remap detection.
   * @param data       pointer into the segment
   * @param size       number of elements
   * @param generation segment generation the pointer was obtained from
   */
  ShmView(pointer data, size_type size, shm::ShmGeneration generation) noexcept
    : m_data(data), m_size(data == nullptr ? 0 : size), m_generation(generation.value)
  {}

  /**
   * Construct a view with remap detection.
   * @param data             pointer into the segment
   * @param size             number of elements
   * @param generation       segment generation the pointer was obtained from
   * @param generationProvider callable returning the segment's current generation
   */
  ShmView(pointer data, size_type size, shm::ShmGeneration generation, GenerationProvider generationProvider)
    : m_data(data), m_size(data == nullptr ? 0 : size), m_generation(generation.value),
      m_generationProvider(std::move(generationProvider))
  {}

  /**
   * Construct a view whose remap detection uses a shared memory service.
   * @param data    pointer into the segment
   * @param size    number of elements
   * @param service service owning the segment; may be null for an empty view
   */
  ShmView(pointer data, size_type size, const std::shared_ptr<SharedMemoryService> &service)
    : ShmView(data,
        size,
        shm::ShmGeneration{ service == nullptr ? kInvalidGeneration : shm::generationOf(service) },
        [weakService = std::weak_ptr<SharedMemoryService>(service)]() { return shm::generationOf(weakService); })
  {}

  /** @return pointer to the first element, nullptr for an empty view */
  [[nodiscard]] pointer get() const noexcept { return m_data; }

  /** @return pointer to the first element, nullptr for an empty view */
  [[nodiscard]] pointer data() const noexcept { return m_data; }

  /** @return number of elements */
  [[nodiscard]] size_type size() const noexcept { return m_size; }

  /** @return number of bytes covered by the view */
  [[nodiscard]] size_type sizeBytes() const noexcept { return m_size * sizeof(T); }

  /** @return true when the view does not point at an element */
  [[nodiscard]] bool empty() const noexcept { return m_size == 0; }

  /** @return segment generation the pointer was obtained from */
  [[nodiscard]] std::size_t generation() const noexcept { return m_generation; }

  /** @return true when a generation provider is installed */
  [[nodiscard]] bool hasGenerationProvider() const noexcept { return static_cast<bool>(m_generationProvider); }

  /**
   * Check the view against a known current generation.
   * @param currentGeneration the generation reported by SharedMemoryService
   * @return true when the view still points into the current mapping
   */
  [[nodiscard]] bool valid(std::size_t currentGeneration) const noexcept
  {
    return m_data != nullptr && currentGeneration == m_generation && currentGeneration != kInvalidGeneration;
  }

  /**
   * Check the view against its generation provider.
   *
   * Without a provider a non-empty view counts as valid; it cannot know about a
   * remap on its own.
   *
   * @return true when the view still points into the current mapping
   */
  [[nodiscard]] bool valid() const
  {
    if (m_data == nullptr) { return false; }
    if (!m_generationProvider) { return true; }
    return valid(m_generationProvider());
  }

  /** @return true when the segment was remapped (or the view is empty) */
  [[nodiscard]] bool isStale() const { return !valid(); }

  /**
   * Bounds- and generation-checked element access.
   * @param index the element index
   * @return reference to the element
   * @throws std::out_of_range when index >= size()
   * @throws std::runtime_error when the view is stale
   */
  [[nodiscard]] reference at(size_type index) const
  {
    ensureFresh();
    if (index >= m_size) {
      throw std::out_of_range(
        "shared memory view index " + std::to_string(index) + " is out of range (size " + std::to_string(m_size) + ")");
    }
    return m_data[index];
  }

  /**
   * Bounds- and generation-checked element access.
   * @param index the element index
   * @return reference to the element
   * @throws std::out_of_range when index >= size()
   * @throws std::runtime_error when the view is stale
   */
  [[nodiscard]] reference operator[](size_type index) const { return at(index); }

  /**
   * @return reference to the first element
   * @throws std::out_of_range when the view is empty
   * @throws std::runtime_error when the view is stale
   */
  [[nodiscard]] reference front() const { return at(0); }

  /**
   * @return reference to the last element
   * @throws std::out_of_range when the view is empty
   * @throws std::runtime_error when the view is stale
   */
  [[nodiscard]] reference back() const
  {
    ensureFresh();
    if (m_size == 0) { throw std::out_of_range("shared memory view is empty"); }
    return m_data[m_size - 1];
  }

  /** @return iterator to the first element */
  [[nodiscard]] iterator begin() const noexcept { return m_data; }

  /** @return iterator one past the last element */
  [[nodiscard]] iterator end() const noexcept { return m_data == nullptr ? nullptr : m_data + m_size; }

  /**
   * Create a sub-view of the first count elements.
   * @param count number of elements
   * @return sub-view sharing pointer, generation and provider
   * @throws std::out_of_range when count > size()
   */
  [[nodiscard]] ShmView first(size_type count) const { return slice(0, count); }

  /**
   * Create a sub-view of the last count elements.
   * @param count number of elements
   * @return sub-view sharing pointer, generation and provider
   * @throws std::out_of_range when count > size()
   */
  [[nodiscard]] ShmView last(size_type count) const
  {
    if (count > m_size) { throw std::out_of_range("shared memory view slice exceeds the view size"); }
    return slice(m_size - count, count);
  }

  /**
   * Create a sub-view (slice) of this view.
   * @param offset index of the first element in the slice
   * @param count  number of elements in the slice
   * @return sub-view sharing generation and provider
   * @throws std::out_of_range when offset + count > size()
   */
  [[nodiscard]] ShmView slice(size_type offset, size_type count) const
  {
    ensureFresh();
    if (offset > m_size || count > m_size - offset) {
      throw std::out_of_range("shared memory view slice (" + std::to_string(offset) + ", " + std::to_string(count)
                              + ") exceeds the view size " + std::to_string(m_size));
    }
    return ShmView(
      m_data == nullptr ? nullptr : m_data + offset, count, shm::ShmGeneration{ m_generation }, m_generationProvider);
  }

private:
  /** Reject access after the segment was remapped. */
  void ensureFresh() const
  {
    if (isStale()) {
      throw std::runtime_error(
        "shared memory view is stale: the segment was remapped, re-create the view from its named object");
    }
  }

  pointer m_data = nullptr;
  size_type m_size = 0;
  std::size_t m_generation = 0;
  GenerationProvider m_generationProvider;
};

/** Backwards/alias name for a slice of shared memory. */
template<typename T> using ShmSlice = ShmView<T>;

namespace shm {
  /**
   * Create a generation-aware view of an existing named object.
   *
   * The object must already exist (create it with ShmConstructors.h first); the
   * view itself never allocates. Remap detection uses a weak reference to the
   * service, so the view does not extend the service lifetime.
   *
   * @tparam T      element type of the object
   * @param service shared memory service owning the segment
   * @param segment segment containing the object
   * @param name    name of the object inside the segment
   * @param count   number of elements
   * @return view into the current mapping
   * @throws std::invalid_argument when service or segment is null
   * @throws std::out_of_range when the object does not exist
   */
  template<typename T>
  ShmView<T> viewOf(const std::shared_ptr<SharedMemoryService> &service,
    const std::shared_ptr<SharedMemorySegment> &segment,
    const std::string &name,
    std::size_t count = 1)
  {
    if (service == nullptr) { throw std::invalid_argument("cannot create a shared memory view without a service"); }
    if (segment == nullptr) { throw std::invalid_argument("cannot create a shared memory view without a segment"); }
    void *address = findNamedObject(*service, segment, name);
    if (address == nullptr) {
      throw std::out_of_range(
        "shared memory object '" + name + "' does not exist in segment '" + segment->getName() + "'");
    }
    std::weak_ptr<SharedMemoryService> weakService = service;
    auto provider = [weakService]() -> std::size_t { return shm::generationOf(weakService); };
    return ShmView<T>(
      static_cast<T *>(address), count, shm::ShmGeneration{ shm::generationOf(weakService) }, std::move(provider));
  }
}// namespace shm

}// namespace lintel
#endif// LINTEL_SHMVIEW_H
