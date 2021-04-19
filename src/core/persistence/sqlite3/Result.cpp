#include "base_library/core/persistence/sqlite3/Result.h"

namespace sqlite {
void Result::add(const std::vector<std::string> &entry) {
  entries.push_back(entry);
}
std::string Result::getValue(int row, int attribute) const {
  if (entries.empty()) {
    return "";
  }
  return entries[row][attribute];
}

int Result::getNumOfAttributes() const {
  if (entries.empty()) {
    return 0;
  }
  return entries[0].size();
}

int Result::getSize() const { return entries.size(); }
}  // namespace sqlite