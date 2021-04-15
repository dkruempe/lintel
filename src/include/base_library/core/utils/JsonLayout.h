#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif
#ifndef CPP_BASIC_LIBRARIES_JSONLAYOUT_H
#define CPP_BASIC_LIBRARIES_JSONLAYOUT_H

#include <log4cxx/layout.h>

namespace log4cxx::json {

class LOG4CXX_EXPORT JsonLayout : public Layout {
private:
  // Print no location info by default
  bool locationInfo; //= false
  bool properties;   // = false

public:
  DECLARE_LOG4CXX_OBJECT(JsonLayout)
  BEGIN_LOG4CXX_CAST_MAP()
  LOG4CXX_CAST_ENTRY(JsonLayout)
  LOG4CXX_CAST_ENTRY_CHAIN(Layout)
  END_LOG4CXX_CAST_MAP()

  JsonLayout();

  /**
  The <b>LocationInfo</b> option takes a boolean value. By
  default, it is set to false which means there will be no location
  information output by this layout. If the the option is set to
  true, then the file name and line number of the statement
  at the origin of the log statement will be output.
  <p>If you are embedding this layout within a SMTPAppender
  then make sure to set the
  <b>LocationInfo</b> option of that appender as well.
  */
  inline void setLocationInfo(bool locationInfo1) {
    this->locationInfo = locationInfo1;
  }

  /**
Returns the current value of the <b>LocationInfo</b> option.
*/
  inline bool getLocationInfo() const { return locationInfo; }

  /**
   * Sets whether MDC key-value pairs should be output, default false.
   * @param flag new value.
   *
   */
  inline void setProperties(bool flag) { properties = flag; }

  /**
   * Gets whether MDC key-value pairs should be output.
   * @return true if MDC key-value pairs are output.
   *
   */
  inline bool getProperties() const { return properties; }

  /** No options to activate. */
  void activateOptions(log4cxx::helpers::Pool & /* p */) override {}

  /**
  Set options
  */
  void setOption(const LogString &option, const LogString &value) override;

  /**
   * Formats a {@link spi::LoggingEvent LoggingEvent}
   * in conformance with the log4cxx.dtd.
   **/
  void format(LogString &output, const spi::LoggingEventPtr &event,
              log4cxx::helpers::Pool &p) const override;

  /**
  The XMLLayout prints and does not ignore exceptions. Hence the
  return value <code>false</code>.
  */
  bool ignoresThrowable() const override { return false; }

}; // class XMLLayout
LOG4CXX_PTR_DEF(JsonLayout);
} // namespace log4cxx::json

#endif // CPP_BASIC_LIBRARIES_JSONLAYOUT_H

#pragma clang diagnostic pop