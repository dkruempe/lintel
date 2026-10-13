#ifndef LINTEL_HISTORYCOMPONENT_H
#define LINTEL_HISTORYCOMPONENT_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "lintel/core/configuration/Component.h"
#include "lintel/core/configuration/Entry.h"

namespace lintel {
/** Component for parsing the history service configuration from XML */
class HistoryComponent : public Component
{
private:
  /** XML element name constants for history service parsing */
  static const struct Shapes
  {
    const char *const CONFIG_ROOT = "HistoryServices";
    const char *const HISTORY_ROOT = "HistoryService";
    const char *const PROCESS_NAME = "process_name";
    const char *const QUEUE = "queue";
    const char *const MAX_MESSAGES = "max_messages";
  } shape;

public:
  /** Construct a HistoryComponent */
  HistoryComponent();

  /** Parse history service configuration XML
   * @param content XML content to parse
   * @param fileName Source file name for error reporting
   * @param lineOffset Line offset for error reporting
   * @return Vector of parsed history service entries */
  std::vector<std::shared_ptr<Entry>>
    parse(const std::string &content, int32_t lineOffset, const std::string &fileName) override;
};

}// namespace lintel
#endif// LINTEL_HISTORYCOMPONENT_H
