#include "base_library/core/persistence/sqlite3/Result.h"

namespace sqlite {
void Result::add(const std::vector<std::string> &entry) {
  entries.push_back(entry);
}
std::string Result::getValue(int row, int attribute) const {
  if (entries.empty()) {
    return "";
  }
  return entries[static_cast<std::size_t>(row)][static_cast<std::size_t>(attribute)];
}

int Result::getNumOfAttributes() const {
  if (entries.empty()) {
    return 0;
  }
  return static_cast<int>(entries[0].size());
}

int Result::getSize() const { return static_cast<int>(entries.size()); }
}  // namespace sqlite