#include "base_library/features/http/service/ContentType.h"

ContentType::Value ContentType::build(const std::string& contentType) {
  auto found = m_nameToValue.find(contentType);
  if (found == m_nameToValue.end()) {
    return UNDEFINED;
  }
  return found->second;
}
ContentType::ContentType(const std::string& contentType) : m_value(UNDEFINED) {
  m_value = build(contentType);
}
ContentType::ContentType(ContentType::Value value) : m_value(value) {}
const std::string& ContentType::getName() const {
  auto found = m_valueToName.find(m_value);
  if (found == m_valueToName.end()) {
    return m_valueToName.at(TextPlain);
  }
  return found->second;
}
