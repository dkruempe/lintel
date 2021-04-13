#ifndef CPP_BASE_LIBRARY_SQLITE_RESULT_H
#define CPP_BASE_LIBRARY_SQLITE_RESULT_H

#include <string>
#include <vector>

namespace sqlite {
class Result {
private:
  std::vector<std::vector<std::string>> entries;

public:
  void add(const std::vector<std::string> &entry);

  [[nodiscard]] std::string getValue(int row, int attribute) const;

  [[nodiscard]] int getNumOfAttributes() const;

  [[nodiscard]] int getSize() const;
};
} // namespace sqlite

#endif // CPP_BASE_LIBRAR_SQLITE_RESULT_H
