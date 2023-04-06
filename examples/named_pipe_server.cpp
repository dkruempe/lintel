#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <filesystem>
#include <future>
#include <iostream>
#include <sstream>
#include <system_error>
#include <vector>

/**
 * named pipe service for managing those kind of pipes in macOs and Linux
 */
class NamedPipeService {
 public:
  explicit NamedPipeService(std::filesystem::path path,
                            bool createParentDirs = false)
      : m_path(std::move(path)) {
    if (!exists(m_path.parent_path())) {
      if (!createParentDirs) {
        throw std::runtime_error("Parent directory doesn't exists => abort");
      }
      bool const ret = create_directories(m_path);
      if (!ret) {
        throw std::runtime_error("failed to create parent directories");
      }
    }
    if (!exists(m_path)) {
      mkfifo(m_path.c_str(), 0666);
    }
    if (!is_fifo(m_path)) {
      throw std::runtime_error("file exists but is not an fifo file");
    }
    open();
  }

  ~NamedPipeService() { close(); }

  /**
   * Write a message to the named pipe
   *
   * Message structure:
   * STX|%016x|message|ETX
   * @param message
   */
  void write(const std::string& message) const {
    // 1. build message
    std::stringstream ss;
    ss << m_stx;
    ss << std::setw(16) << std::setfill('0') << std::hex << message.length();
    ss << message;
    ss << m_etx;
    std::string temp = ss.str();
    if (::write(m_pipe_fd, temp.data(), temp.size()) < 0) {
      throw std::system_error(errno, std::system_category(),
                              "Failed to write to named pipe");
    }
  }

  /**
   * read defined maximum of messages and return those
   *
   * @param maxMessages maximum size of messages
   * @return messages
   */
  [[nodiscard]] std::vector<std::string> read(int maxMessages = 100) const {
    ssize_t numBytes = 1;
    std::vector<std::string> messages;
    while (messages.size() < maxMessages) {
      std::vector<char> sizeVec(17);
      numBytes = ::read(m_pipe_fd, sizeVec.data(), sizeVec.size());
      if (numBytes == 0) {
        continue;
      }
      if (numBytes < 0) {
        if (errno == EAGAIN) {
          // named pipe empty
          return messages;
        }
        throw std::system_error(errno, std::system_category(),
                                "Failed to read from named pipe");
      }
      // erase STX
      sizeVec.erase(sizeVec.begin());
      std::string const sizeStr(sizeVec.data(), sizeVec.size());
      std::size_t const size = std::strtol(sizeStr.c_str(), nullptr, 16) + 1;
      std::vector<char> tempVec(size);
      numBytes = ::read(m_pipe_fd, tempVec.data(), tempVec.size());
      if (numBytes < 0) {
        throw std::system_error(errno, std::system_category(),
                                "Failed to read from named pipe");
      }
      tempVec.erase(tempVec.end() - 1);
      messages.emplace_back(tempVec.data(), tempVec.size());
    }
    return messages;
  }

  /**
   * reads total amount of messages in the named pipe and calls for each
   * message the call back function onMessage
   *
   * @param onMessage call back function
   */
  void read(std::function<void(std::string)>&& onMessage) const {
    ssize_t numBytes = 1;
    while (true) {
      std::vector<char> sizeVec(17);
      numBytes = ::read(m_pipe_fd, sizeVec.data(), sizeVec.size());
      if (numBytes == 0) {
        continue;
      }
      if (numBytes < 0) {
        if (errno == EAGAIN) {
          // named pipe empty
          return;
        }
        throw std::system_error(errno, std::system_category(),
                                "Failed to read from named pipe");
      }
      // erase STX
      sizeVec.erase(sizeVec.begin());
      std::string const sizeStr(sizeVec.data(), sizeVec.size());
      std::size_t const size = std::strtol(sizeStr.c_str(), nullptr, 16) + 1;
      std::vector<char> tempVec(size);
      numBytes = ::read(m_pipe_fd, tempVec.data(), tempVec.size());
      if (numBytes < 0) {
        throw std::system_error(errno, std::system_category(),
                                "Failed to read from named pipe");
      }
      tempVec.erase(tempVec.end() - 1);
      onMessage(std::string(tempVec.data(), tempVec.size()));
    }
  }

 private:
  /**
   * open named pipe for reading and writing
   */
  void open() {
    m_pipe_fd = ::open(m_path.c_str(), O_RDWR | O_NONBLOCK);
    if (m_pipe_fd < 0) {
      throw std::system_error(errno, std::system_category(),
                              "Failed to open named pipe");
    }
  }

  /**
   * close the named pipe
   */
  void close() {
    if (m_pipe_fd >= 0) {
      ::close(m_pipe_fd);
      m_pipe_fd = -1;
    }
  }

  std::filesystem::path m_path;
  int m_pipe_fd{-1};
  static constexpr char m_stx = '\x02';
  static constexpr char m_etx = '\x03';
};

void onMessage(const std::string& message) {
  std::cout << "message: " << message << "\n";
}

void performanceTest() {
  auto start = std::chrono::steady_clock::now();
  NamedPipeService const s("/Users/dkruempe/history_service.pipe");
  std::cout << "start write\n";
  for (int i = 0; i < 1000; i++) {
    try {
      s.write("Hello World!");
    } catch (std::exception &ex) {
      std::cout << ex.what() << " i: " << i << "\n";
    }
  }
  std::cout << "finished write\n";
  auto end = std::chrono::steady_clock::now();
  std::cout << "Duration: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                     start).count()
            << "\n";
}

int main(int /*argc*/, char* /*argv*/[]) {
  try {
    performanceTest();
    NamedPipeService const s("/Users/dkruempe/history_service.pipe");
    s.write("Hello World!");
    s.write("Hello World 1!");
    s.write("Hello World 2!");
    s.write("Hello World <1337>!");
    std::vector<std::string> const messages = s.read(2);
    for (const auto& message : messages) {
      std::cout << "message: " << message << "\n";
    }
    std::cout << "Read " << messages.size() << "\n";
    s.write("New World!");
    s.write("New World! 1");
    s.write("New World! 2");
    s.write("New World! 3");
    s.write("New World! 4");
    s.write("New World! 1337");
    s.read([&](const std::string& message) { onMessage(message); });
  } catch (std::exception& ex) {
    std::cerr << ex.what() << "\n";
  } catch (...) {
    std::cerr << "Unknown exception"
              << "\n";
  }
  return 0;
}