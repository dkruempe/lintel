#include "base_library/core/persistence/sqlite3/Result.h"

namespace sqlite {
std::string Result::getValue(int row, int attribute) {
  if (m_arguments.empty()) {
    return "";
  }
  return m_arguments.at(static_cast<std::size_t>(row))
      .of(static_cast<std::size_t>(attribute))
      .getValue();
}

int Result::getNumOfAttributes() const {
  if (m_arguments.empty()) {
    return 0;
  }
  return static_cast<int>(m_arguments[0].size());
}

int Result::getSize() const { return static_cast<int>(m_arguments.size()); }
db::Arguments& Result::of(std::size_t pos) {
  return m_arguments.at(pos);
}
void Result::add(const db::Arguments& arguments) {
  m_arguments.push_back(arguments);
}
}  // namespace sqlite