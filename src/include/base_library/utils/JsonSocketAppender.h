#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif
#ifndef CPP_BASIC_LIBRARIES_JSONSOCKETAPPENDER_H
#define CPP_BASIC_LIBRARIES_JSONSOCKETAPPENDER_H

#include <log4cxx/helpers/writer.h>
#include <log4cxx/net/socketappenderskeleton.h>

namespace log4cxx::net {

class LOG4CXX_EXPORT JsonSocketAppender : public SocketAppenderSkeleton {
public:
  /**
  The default port number of remote logging server (4560).
  */
  static int DEFAULT_PORT;

  /**
  The default reconnection delay (30000 milliseconds or 30 seconds).
  */
  static int DEFAULT_RECONNECTION_DELAY;

  /**
  An event XML stream cannot exceed 1024 bytes.
  */
  static const int MAX_EVENT_LEN;

  DECLARE_LOG4CXX_OBJECT(JsonSocketAppender)
  BEGIN_LOG4CXX_CAST_MAP()
  LOG4CXX_CAST_ENTRY(JsonSocketAppender)
  LOG4CXX_CAST_ENTRY_CHAIN(AppenderSkeleton)
  END_LOG4CXX_CAST_MAP()

  JsonSocketAppender();
  ~JsonSocketAppender() override;

  /**
  Connects to remote server at <code>address</code> and <code>port</code>.
  */
  JsonSocketAppender(const helpers::InetAddressPtr &address, int port);

  /**
  Connects to remote server at <code>host</code> and <code>port</code>.
  */
  JsonSocketAppender(const LogString &host, int port);

protected:
  void setSocket(log4cxx::helpers::SocketPtr &socket,
                 log4cxx::helpers::Pool &p) override;

  void cleanUp(log4cxx::helpers::Pool &p) override;

  int getDefaultDelay() const override;

  int getDefaultPort() const override;

  void append(const spi::LoggingEventPtr &event,
              log4cxx::helpers::Pool &pool) override;

private:
  log4cxx::helpers::WriterPtr writer;
  //  prevent copy and assignment statements
  JsonSocketAppender(const JsonSocketAppender &);
  JsonSocketAppender &operator=(const JsonSocketAppender &);
}; // class XMLSocketAppender

LOG4CXX_PTR_DEF(JsonSocketAppender);

} // namespace log4cxx::net

#endif // CPP_BASIC_LIBRARIES_JSONSOCKETAPPENDER_H
