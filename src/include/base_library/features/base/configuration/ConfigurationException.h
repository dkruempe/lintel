#ifndef CPP_BASE_LIBRARY_CONFIGURATIONEXCEPTION_H
#define CPP_BASE_LIBRARY_CONFIGURATIONEXCEPTION_H

#include <exception>
#include <utility>

class ConfigurationException : public std::exception {
 private:
  std::string m_componentName;
  std::string m_message;
  int32_t m_line;
  std::string m_printMessage;

 public:
  ConfigurationException(std::string componentName, std::string message,
                         int32_t line)
      : m_componentName(std::move(componentName)),
        m_message(std::move(message)),
        m_line(line) {
    m_printMessage =
        m_componentName + ":" + std::to_string(m_line) + " -> " + m_message;
  }

  [[nodiscard]] const char *what() const noexcept override {
    return m_printMessage.c_str();
  }
};

#endif  // CPP_BASE_LIBRARY_CONFIGURATIONEXCEPTION_H
