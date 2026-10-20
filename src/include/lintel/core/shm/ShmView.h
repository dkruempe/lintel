#ifndef LINTEL_SHMVIEW_H
#define LINTEL_SHMVIEW_H

#include <cstddef>

namespace lintel {

template<typename T> class ShmView
{
public:
  ShmView(T *ptr, std::size_t generation) : m_ptr(ptr), m_generation(generation) {}
  T *get() const { return m_ptr; }
  std::size_t generation() const { return m_generation; }

private:
  T *m_ptr = nullptr;
  std::size_t m_generation = 0;
};

}// namespace lintel
#endif
