#ifndef CPP_BASE_LIBRARY_SQLITE_RESULT_H
#define CPP_BASE_LIBRARY_SQLITE_RESULT_H

#include <string>
#include <vector>

#include "base_library/core/persistence/Arguments.h"

namespace sqlite {
class Result {
 private:
  std::vector<db::Arguments> m_arguments;

 public:
  void add(const db::Arguments& arguments);

  [[nodiscard]] std::string getValue(int row, int attribute);

  [[nodiscard]] int getNumOfAttributes() const;

  [[nodiscard]] int getSize() const;

  [[nodiscard]] db::Arguments& of(std::size_t pos);
};
}  // namespace sqlite

#endif  // CPP_BASE_LIBRARY_SQLITE_RESULT_H
