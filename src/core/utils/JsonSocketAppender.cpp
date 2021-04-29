#include "base_library/core/utils/JsonSocketAppender.h"

#include <log4cxx/helpers/charsetencoder.h>
#include <log4cxx/helpers/loglog.h>
#include <log4cxx/helpers/optionconverter.h>
#include <log4cxx/helpers/outputstreamwriter.h>
#include <log4cxx/helpers/socketoutputstream.h>
#include <log4cxx/helpers/synchronized.h>
#include <log4cxx/helpers/transcoder.h>
#include <log4cxx/xml/xmllayout.h>

using namespace log4cxx;
using namespace log4cxx::helpers;
using namespace log4cxx::net;
using namespace log4cxx::xml;

IMPLEMENT_LOG4CXX_OBJECT(JsonSocketAppender)

// The default port number of remote logging server (4560)
int JsonSocketAppender::DEFAULT_PORT = 4560;

// The default reconnection delay (30000 milliseconds or 30 seconds).
int JsonSocketAppender::DEFAULT_RECONNECTION_DELAY = 30000;

const int JsonSocketAppender::MAX_EVENT_LEN = 1024;

JsonSocketAppender::JsonSocketAppender()
    : SocketAppenderSkeleton(DEFAULT_PORT, DEFAULT_RECONNECTION_DELAY) {
  layout = new XMLLayout();
}

JsonSocketAppender::JsonSocketAppender(const InetAddressPtr &address1,
                                       int port1)
    : SocketAppenderSkeleton(address1, port1, DEFAULT_RECONNECTION_DELAY) {
  layout = new XMLLayout();
  Pool p;
  activateOptions(p);
}

JsonSocketAppender::JsonSocketAppender(const LogString &host, int port1)
    : SocketAppenderSkeleton(host, port1, DEFAULT_RECONNECTION_DELAY) {
  layout = new XMLLayout();
  Pool p;
  activateOptions(p);
}

JsonSocketAppender::~JsonSocketAppender() { finalize(); }

int JsonSocketAppender::getDefaultDelay() const {
  return DEFAULT_RECONNECTION_DELAY;
}

int JsonSocketAppender::getDefaultPort() const { return DEFAULT_PORT; }

void JsonSocketAppender::setSocket(log4cxx::helpers::SocketPtr &socket,
                                   Pool &p) {
  OutputStreamPtr os(new SocketOutputStream(socket));
  CharsetEncoderPtr charset(CharsetEncoder::getUTF8Encoder());
  synchronized sync(mutex);
  m_writer = new OutputStreamWriter(os, charset);
}

void JsonSocketAppender::cleanUp(Pool &p) {
  if (m_writer != 0) {
    try {
      m_writer->close(p);
      m_writer = 0;
    } catch (std::exception &) {
    }
  }
}

void JsonSocketAppender::append(const spi::LoggingEventPtr &event,
                                log4cxx::helpers::Pool &p) {
  if (m_writer != 0) {
    LogString output;
    layout->format(output, event, p);

    try {
      m_writer->write(output, p);
      m_writer->flush(p);
    } catch (std::exception &e) {
      m_writer = 0;
      LogLog::warn(LOG4CXX_STR("Detected problem with connection: "), e);

      if (getReconnectionDelay() > 0) {
        fireConnector();
      }
    }
  }
}
