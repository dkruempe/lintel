#ifndef CPP_BASE_LIBRARY_RESULT_H
#define CPP_BASE_LIBRARY_RESULT_H

#include <iterator>
#include <string>

#include "base_library/core/persistence/Arguments.h"
#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/postgresql/Result.h"
#include "base_library/core/persistence/sqlite3/Result.h"

namespace db {
class Result {
 private:
  std::shared_ptr<postgresql::Result> m_result = nullptr;
  std::shared_ptr<sqlite::Result> m_resultSQLite = nullptr;

 public:
  class Iterator
      : public std::iterator<std::random_access_iterator_tag, Arguments,
                             std::ptrdiff_t, Arguments *, Arguments &> {
   private:
    Result &m_result;
    std::size_t m_pos;

   public:
    explicit Iterator(Result &result, bool end = false)
        : m_result(result),
          m_pos(end ? static_cast<std::size_t>(m_result.getSize()) : 0) {}

    Iterator &operator++() {
      ++m_pos;
      return *this;
    }
    Iterator operator++(int) {
      ++m_pos;
      return *this;
    }
    bool operator==(Iterator other) const { return m_pos == other.m_pos; }
    bool operator!=(Iterator other) const { return !(*this == other); }
    reference operator*() { return m_result.of(m_pos); }
  };

  class ConstIterator
      : public std::iterator<std::random_access_iterator_tag, Arguments,
                             std::ptrdiff_t, const Arguments *,
                             const Arguments &> {
   private:
    const Result &m_result;
    std::size_t m_pos;

   public:
    explicit ConstIterator(const Result &result, bool end = false)
        : m_result(result),
          m_pos(end ? static_cast<std::size_t>(m_result.getSize()) : 0) {}

    ConstIterator &operator++() {
      ++m_pos;
      return *this;
    }
    ConstIterator operator++(int) {
      ++m_pos;
      return *this;
    }
    bool operator==(ConstIterator other) const { return m_pos == other.m_pos; }
    bool operator!=(ConstIterator other) const { return !(*this == other); }
    reference operator*() const { return m_result.of(m_pos); }
  };

  Iterator begin() { return Iterator(*this); }

  Iterator end() { return Iterator(*this, true); }

  ConstIterator begin() const { return ConstIterator(*this); }

  ConstIterator end() const { return ConstIterator(*this, true); }

  Result() = default;

  ~Result();

  explicit Result(std::shared_ptr<postgresql::Result> result);

  explicit Result(std::shared_ptr<sqlite::Result> result);

  [[nodiscard]] std::string getValue(int row, int attribute) const;

  [[nodiscard]] Arguments &of(std::size_t pos);

  [[nodiscard]] const Arguments &of(std::size_t pos) const;

  [[nodiscard]] int getNumOfAttributes() const;

  [[nodiscard]] int getSize() const;
};
}  // namespace db

#endif  // CPP_BASE_LIBRARY_RESULT_H
