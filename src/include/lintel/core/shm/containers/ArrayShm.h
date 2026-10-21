#ifndef LINTEL_ARRAYSHM_H
#define LINTEL_ARRAYSHM_H

#include <cstddef>
#include <stdexcept>

namespace lintel {
/**
 * Fixed size array of N elements of type T inside shared memory.
 *
 * Plain value semantics: the elements live inline in the object, so an
 * ArrayShm can be placed in a segment by SharedMemoryRepository<ArrayShm<T, N>>
 * (or any other shared memory storage) without further allocation. No process
 * local pointers, no allocator, nothing boost related - the header stays
 * usable from any translation unit.
 *
 * Bounds behavior: get()/operator[]/at() and set() throw std::out_of_range on
 * an index >= size(). set() is checked as well, because a corrupted or
 * concurrently shrunken index in shared memory must not silently write past
 * the end of the array.
 *
 * The object is not internally synchronized; concurrent access to one index
 * from several threads needs external locking.
 *
 * @tparam T element type, must be usable in shared memory
 * @tparam N number of elements, must be positive
 */
template<typename T, std::size_t N> class ArrayShm
{
  static_assert(N > 0, "an ArrayShm needs at least one element");

public:
  using value_type = T;
  using size_type = std::size_t;
  using iterator = T *;
  using const_iterator = const T *;

  /** @return number of elements */
  [[nodiscard]] static constexpr size_type size() noexcept { return N; }

  /** @return true when the array holds no element (only for API symmetry,
   *  N is a compile time constant > 0) */
  [[nodiscard]] static constexpr bool empty() noexcept { return false; }

  /** @param index element index
   *  @return reference to the element
   *  @throws std::out_of_range when index >= size() */
  [[nodiscard]] T &at(size_type index)
  {
    if (index >= N) { throw std::out_of_range("ArrayShm index out of range"); }
    return m_data[index];
  }

  /** @param index element index
   *  @return const reference to the element
   *  @throws std::out_of_range when index >= size() */
  [[nodiscard]] const T &at(size_type index) const
  {
    if (index >= N) { throw std::out_of_range("ArrayShm index out of range"); }
    return m_data[index];
  }

  /** Checked element access (API spelling next to SharedMemoryRepository::get).
   *  @param index element index
   *  @return reference to the element
   *  @throws std::out_of_range when index >= size() */
  [[nodiscard]] T &get(size_type index) { return at(index); }

  /** Checked element access (API spelling next to SharedMemoryRepository::get).
   *  @param index element index
   *  @return const reference to the element
   *  @throws std::out_of_range when index >= size() */
  [[nodiscard]] const T &get(size_type index) const { return at(index); }

  /** Unchecked element access, the index is assumed to be valid.
   *  @param index element index
   *  @return reference to the element */
  [[nodiscard]] T &operator[](size_type index) { return m_data[index]; }

  /** Unchecked element access, the index is assumed to be valid.
   *  @param index element index
   *  @return const reference to the element */
  [[nodiscard]] const T &operator[](size_type index) const { return m_data[index]; }

  /** Replace one element.
   *  @param index element index
   *  @param value the new value
   *  @throws std::out_of_range when index >= size() */
  void set(size_type index, const T &value)
  {
    if (index >= N) { throw std::out_of_range("ArrayShm index out of range"); }
    m_data[index] = value;
  }

  /** Overwrite every element with value.
   *  @param value the new value of all elements */
  void fill(const T &value)
  {
    for (size_type i = 0; i < N; ++i) { m_data[i] = value; }
  }

  /** @return pointer to the first element */
  [[nodiscard]] iterator begin() noexcept { return m_data; }

  /** @return pointer behind the last element */
  [[nodiscard]] iterator end() noexcept { return m_data + N; }

  /** @return const pointer to the first element */
  [[nodiscard]] const_iterator begin() const noexcept { return m_data; }

  /** @return const pointer behind the last element */
  [[nodiscard]] const_iterator end() const noexcept { return m_data + N; }

  /** @return const pointer to the first element */
  [[nodiscard]] const_iterator cbegin() const noexcept { return m_data; }

  /** @return const pointer behind the last element */
  [[nodiscard]] const_iterator cend() const noexcept { return m_data + N; }

  /** @return pointer to the raw element storage */
  [[nodiscard]] T *data() noexcept { return m_data; }

  /** @return const pointer to the raw element storage */
  [[nodiscard]] const T *data() const noexcept { return m_data; }

private:
  T m_data[N]{};
};

}// namespace lintel
#endif// LINTEL_ARRAYSHM_H
