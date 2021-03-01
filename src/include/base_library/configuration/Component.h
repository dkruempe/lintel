#ifndef CPP_BASE_LIBRARY_COMPONENT_H
#define CPP_BASE_LIBRARY_COMPONENT_H

#include <memory>
#include <string>
#include <vector>

#include "base_library/configuration/Entry.h"

class Component {
private:
  std::string configRoot;

public:
  explicit Component(std::string configRoot);

  [[nodiscard]] const std::string &getConfigRoot() const;

  virtual std::vector<std::shared_ptr<Entry>>
  parse(const std::string &content, const std::string &fileName,
        const int32_t lineOffset) = 0;
};

#endif // CPP_BASE_LIBRARY_COMPONENT_H
