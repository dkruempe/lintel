#include <libgen.h>
#include <map>
#include <memory>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
/*#include <spdlog/sinks/tcp_sink.h>*/
#include <spdlog/spdlog.h>

/**
 * abstract class for initializing and configure Logger
 */
/*class LoggerService {
private:
public:
  std::map<std::string,
};*/
int main(int argc, char *argv[]) {

  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::warn);
  console_sink->set_pattern("[multi_sink_example] [%^%l%$] %v");

//  spdlog::sinks::tcp_sink_config tcpSinkConfig("10.0.1.13", 4560);
//  auto tcpSink = std::make_shared<spdlog::sinks::tcp_sink_mt>(tcpSinkConfig);
//  tcpSink->set_level(spdlog::level::trace);

  std::vector<spdlog::sink_ptr> sinks {console_sink/*, tcpSink*/};

  auto logger = std::make_shared<spdlog::logger>(basename(argv[0]), sinks.begin(), sinks.end());
  SPDLOG_LOGGER_INFO(logger, "Hello, {}!", "World");
  SPDLOG_LOGGER_DEBUG(logger, "Hello, {}!", "World");
  SPDLOG_LOGGER_ERROR(logger, "Hello, {}!", "World");
  for (int i = 0; i < 20; i++) {
    SPDLOG_LOGGER_WARN(logger, "Hello, {}!", "World");
  }
  return 0;
}