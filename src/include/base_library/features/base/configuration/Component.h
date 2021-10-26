#ifndef CPP_BASE_LIBRARY_COMPONENT_H
#define CPP_BASE_LIBRARY_COMPONENT_H

#include <memory>
#include <string>
#include <vector>

#include "Entry.h"

class Component {
 private:
  std::string m_configRoot;

 protected:
  /**
   * convert string to bytes
   * @param size
   * @return
   */
  static std::size_t convertToBytes(const std::string &size);

 public:
  explicit Component(std::string configRoot);

  virtual ~Component() = default;

  [[nodiscard]] const std::string &getConfigRoot() const;

  virtual std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                                    const std::string &fileName,
                                                    int32_t lineOffset) = 0;
};

#endif  // CPP_BASE_LIBRARY_COMPONENT_H
