#ifndef CPP_BASE_LIBRARY_CONFIG_ENTRY_H
#define CPP_BASE_LIBRARY_CONFIG_ENTRY_H

#include <ostream>
#include <string>

class Entry {
 private:
  const std::string_view m_component;

 public:
  explicit Entry(std::string_view component);

  std::string_view getConfigurationParserComponent();

  friend std::ostream &operator<<(std::ostream &os, const Entry &entry);
};

#endif  // CPP_BASE_LIBRARY_CONFIG_ENTRY_H
