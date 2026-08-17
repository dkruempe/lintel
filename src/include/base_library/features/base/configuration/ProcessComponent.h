#ifndef CPP_BASE_LIBRARY_PROCESSCOMPONENT_H
#define CPP_BASE_LIBRARY_PROCESSCOMPONENT_H

#include <tinyxml2.h>

#include "base_library/features/base/configuration/Component.h"
#include "base_library/features/base/configuration/EnvironmentConfiguration.h"

/** Component for parsing process and process group configurations from XML */
class ProcessComponent : public Component {
private:
    /** XML element name constants for process parsing */
    static const struct Shapes {
        const char *const CONFIG_ROOT = "Processes";
        const char *const PROCESS_ROOT = "Process";
        const char *const PROCESS_NAME = "name";
        const char *const PROCESS_AUTO_RESTART = "autoRestart";
        const char *const PROCESS_ARGS = "args";
        const char *const PROCESS_MAX_RESTARTS = "maxRestarts";
        const char *const PROCESS_CONFIG = "config";
        const char *const PROCESS_GROUP_ROOT = "ProcessGroup";
        const char *const PROCESS_GROUP_NAME = "name";
    } m_shapes;

    /** Parse a single process entry from XML
     * @param processElement The XML element to parse
     * @param lineOffset Line offset for error reporting
     * @return Parsed process entry */
    std::shared_ptr<Entry> parseProcess(tinyxml2::XMLElement *processElement,
                                        int32_t lineOffset);

    /** Parse a process group entry from XML
     * @param processGroupElement The XML element to parse
     * @param lineOffset Line offset for error reporting
     * @return Parsed process group entry */
    std::shared_ptr<Entry> parseProcessGroup(
            tinyxml2::XMLElement *processGroupElement, int32_t lineOffset);

public:
    /** Construct a ProcessComponent */
    ProcessComponent();

    /** Parse process configuration XML
     * @param content XML content to parse
     * @param fileName Source file name for error reporting
     * @param lineOffset Line offset for error reporting
     * @return Vector of parsed process/group entries */
    std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                              const std::string &fileName,
                                              int32_t lineOffset) override;
};

#endif  // CPP_BASE_LIBRARY_PROCESSCOMPONENT_H
