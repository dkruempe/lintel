#ifndef LINTEL_ARRAYSHM_H
#define LINTEL_ARRAYSHM_H

namespace lintel {
template<typename T, std::size_t N> class ArrayShm
{
public:
  T data[N]{};
};
}// namespace lintel
#endif
