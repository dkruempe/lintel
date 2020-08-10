#include "base_library/utils/JsonLayout.h"

#include <log4cxx/helpers/optionconverter.h>
#include <log4cxx/helpers/stringhelper.h>
#include <log4cxx/level.h>
#include <log4cxx/logstring.h>
#include <log4cxx/spi/loggingevent.h>

using namespace log4cxx;
using namespace log4cxx::helpers;
using namespace log4cxx::spi;
using namespace log4cxx::json;

IMPLEMENT_LOG4CXX_OBJECT(JsonLayout)

JsonLayout::JsonLayout() : locationInfo(false), properties(false) {}

void JsonLayout::setOption(const LogString &option, const LogString &value) {
  if (StringHelper::equalsIgnoreCase(option, LOG4CXX_STR("LOCATIONINFO"),
                                     LOG4CXX_STR("locationinfo"))) {
    setLocationInfo(OptionConverter::toBoolean(value, false));
  }

  if (StringHelper::equalsIgnoreCase(option, LOG4CXX_STR("PROPERTIES"),
                                     LOG4CXX_STR("properties"))) {
    setProperties(OptionConverter::toBoolean(value, false));
  }
}

void JsonLayout::format(LogString &output, const spi::LoggingEventPtr &event,
                        Pool &p) const {
  output.append(LOG4CXX_STR("{"));
  output.append(LOG4CXX_STR("\"logger\":"));
  output.append(LOG4CXX_STR("\""));
  output.append(LOG4CXX_STR(event->getLoggerName()));
  output.append(LOG4CXX_STR("\", \"timestamp\":\""));
  output.append(LOG4CXX_STR(std::to_string(event->getTimeStamp())));
  output.append(LOG4CXX_STR("\", \"@severity\":\""));
  output.append(LOG4CXX_STR(event->getLevel()->toString()));
  output.append(LOG4CXX_STR("\", \"thread\":\""));
  output.append(LOG4CXX_STR(event->getThreadName()));
  output.append(LOG4CXX_STR("\", \"message\":\""));
  output.append(LOG4CXX_STR(event->getRenderedMessage()));
  output.append(LOG4CXX_STR("\""));

  LogString ndc;

  if (event->getNDC(ndc)) {
    output.append(LOG4CXX_STR(", \"NDC\":\""));
    output.append(LOG4CXX_STR(ndc));
    output.append(LOG4CXX_STR("\""));
  }

  if (locationInfo) {
    const LocationInfo &locInfo = event->getLocationInformation();
    output.append(LOG4CXX_STR(", \"class\":\""));
    output.append(LOG4CXX_STR(locInfo.getClassName()));
    output.append(LOG4CXX_STR("\", \"method\":\""));
    output.append(LOG4CXX_STR(locInfo.getMethodName()));
    output.append(LOG4CXX_STR("\", \"file\":\""));
    output.append(LOG4CXX_STR(locInfo.getFileName()));
    output.append(LOG4CXX_STR("\", \"line\":\""));
    output.append(LOG4CXX_STR(std::to_string(locInfo.getLineNumber())));
    output.append("\"");
  }

  if (properties) {
    LoggingEvent::KeySet propertySet(event->getPropertyKeySet());
    LoggingEvent::KeySet keySet(event->getMDCKeySet());

    if (!(keySet.empty() && propertySet.empty())) {

      for (const LogString &key : keySet) {
        LogString value;
        if (event->getMDC(key, value)) {
          output.append(", \"mdc_");
          output.append(LOG4CXX_STR(key));
          output.append("\":\"");
          output.append(LOG4CXX_STR(value));
          output.append("\"");
        }
      }

      for (const LogString &key : propertySet) {
        LogString value;
        if (event->getProperty(key, value)) {
          output.append(", \"property_");
          output.append(LOG4CXX_STR(key));
          output.append("\":\"");
          output.append(LOG4CXX_STR(value));
          output.append("\"");
        }
      }
    }
  }

  output.append("}");
  output.append(LOG4CXX_EOL);
  output.append(LOG4CXX_EOL);
}