#ifndef CPP_BASE_LIBRARY_PROCESSCOMPONENT_H
#define CPP_BASE_LIBRARY_PROCESSCOMPONENT_H

#include <tinyxml2.h>

#include "base_library/features/base/configuration/Component.h"
#include "base_library/features/base/configuration/EnvironmentConfiguration.h"

class ProcessComponent : public Component {
 private:
  static struct Shapes {
    const std::string CONFIG_ROOT = "Processes";
    const std::string PROCESS_ROOT = "Process";
    const std::string PROCESS_NAME = "name";
    const std::string PROCESS_AUTO_RESTART = "autoRestart";
    const std::string PROCESS_ARGS = "args";
    const std::string PROCESS_MAX_RESTARTS = "maxRestarts";
    const std::string PROCESS_GROUP_ROOT = "ProcessGroup";
    const std::string PROCESS_GROUP_NAME = "name";
  } m_shapes;

  std::shared_ptr<Entry> parseProcess(tinyxml2::XMLElement* processElement,
                                      int32_t lineOffset);

  std::shared_ptr<Entry> parseProcessGroup(
      tinyxml2::XMLElement* processGroupElement, int32_t lineOffset);

 public:
  ProcessComponent();

  std::vector<std::shared_ptr<Entry>> parse(const std::string& content,
                                            const std::string& fileName,
                                            int32_t lineOffset) override;
};

#endif  // CPP_BASE_LIBRARY_PROCESSCOMPONENT_H
