#ifndef CPP_BASE_LIBRARY_ARGUMENTS_H
#define CPP_BASE_LIBRARY_ARGUMENTS_H

#include <ostream>
#include <string>
#include <vector>

#include "base_library/core/services/StringifyService.h"

namespace db {
class Argument {
 private:
  std::string m_value;
  std::string m_name;

 public:
  Argument(const std::string &value, const std::string &name);
  [[nodiscard]] const std::string &getValue() const;
  [[nodiscard]] const std::string &getName() const;

  template <typename TYPE>
  TYPE getValue() const {
    return StringifyService<TYPE>::deserializeFromString(m_value);
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const db::Argument &argument);
};
class Arguments {
 private:
  class Iterator
      : public std::iterator<std::random_access_iterator_tag, Argument,
                             std::ptrdiff_t, Argument *, Argument &> {
   private:
    Arguments &m_arguments;
    std::size_t m_pos;

   public:
    explicit Iterator(Arguments &arguments, bool end = false)
        : m_arguments(arguments),
          m_pos(end ? static_cast<std::size_t>(m_arguments.getSize()) : 0) {}

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
    reference operator*() { return m_arguments.getArgument(m_pos); }
  };

  class ConstIterator
      : public std::iterator<std::random_access_iterator_tag, Argument,
                             std::ptrdiff_t, const Argument *,
                             const Argument &> {
   private:
    const Arguments &m_arguments;
    std::size_t m_pos;

   public:
    explicit ConstIterator(const Arguments &arguments, bool end = false)
        : m_arguments(arguments),
          m_pos(end ? static_cast<std::size_t>(m_arguments.getSize()) : 0) {}

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
    reference operator*() const { return m_arguments.getArgument(m_pos); }
  };

 public:
  Arguments() = default;

  void add(const Argument &argument);
  std::size_t getSize() const;
  Argument &getArgument(std::size_t iterator);
  const Argument &getArgument(std::size_t iterator) const;
  std::vector<Argument> m_arguments;
  Iterator begin() { return Iterator(*this); }

  Iterator end() { return Iterator(*this, true); }
  ConstIterator begin() const { return ConstIterator(*this); }

  ConstIterator end() const { return ConstIterator(*this, true); }
};
};  // namespace db

#endif  // CPP_BASE_LIBRARY_ARGUMENTS_H
