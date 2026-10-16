#include <catch2/catch_all.hpp>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "lintel/core/utils/Argument.h"
#include "lintel/features/base/services/ProcessArgumentService.h"

namespace lintel {
namespace {

  /** RAII redirector for `std::cout`, restored on destruction even when a REQUIRE throws. */
  class pasCoutCapture
  {
  public:
    pasCoutCapture() : m_oldBuffer(std::cout.rdbuf(m_stream.rdbuf())) {}
    ~pasCoutCapture() { std::cout.rdbuf(m_oldBuffer); }
    pasCoutCapture(const pasCoutCapture &) = delete;
    pasCoutCapture &operator=(const pasCoutCapture &) = delete;
    [[nodiscard]] std::string str() const { return m_stream.str(); }

  private:
    std::ostringstream m_stream;
    std::streambuf *m_oldBuffer;
  };

  /** RAII redirector for `std::cerr`, restored on destruction even when a REQUIRE throws. */
  class pasCerrCapture
  {
  public:
    pasCerrCapture() : m_oldBuffer(std::cerr.rdbuf(m_stream.rdbuf())) {}
    ~pasCerrCapture() { std::cerr.rdbuf(m_oldBuffer); }
    pasCerrCapture(const pasCerrCapture &) = delete;
    pasCerrCapture &operator=(const pasCerrCapture &) = delete;
    [[nodiscard]] std::string str() const { return m_stream.str(); }

  private:
    std::ostringstream m_stream;
    std::streambuf *m_oldBuffer;
  };

}// namespace

TEST_CASE("ProcessArgumentService: parses long and short flags with values")
{
  std::string name;
  int32_t count = 0;
  std::vector<std::shared_ptr<ArgumentProvider>> providers{
    std::make_shared<ArgumentProvider>(std::vector<Argument>{
      Argument("--name", "-n", "the name", &name), Argument("--count", "-c", "the count", &count) })
  };
  ProcessArgumentService service(providers);

  service.parseArguments({ "--name", "abc", "-c", "7" });

  REQUIRE(name == "abc");
  REQUIRE(count == 7);
}

TEST_CASE("ProcessArgumentService: a flag without a value sets a bool argument")
{
  bool verbose = false;
  std::vector<std::shared_ptr<ArgumentProvider>> providers{
    std::make_shared<ArgumentProvider>(std::vector<Argument>{ Argument("--verbose", "-v", "be verbose", &verbose) })
  };
  ProcessArgumentService service(providers);

  service.parseArguments({ "--verbose" });

  REQUIRE(verbose);
}

TEST_CASE("ProcessArgumentService: skips empty argument strings")
{
  bool verbose = false;
  std::vector<std::shared_ptr<ArgumentProvider>> providers{
    std::make_shared<ArgumentProvider>(std::vector<Argument>{ Argument("--verbose", "-v", "be verbose", &verbose) })
  };
  ProcessArgumentService service(providers);

  service.parseArguments({ "", "--verbose" });

  REQUIRE(verbose);
}

TEST_CASE("ProcessArgumentService: extra values of a flag are ignored with an error log")
{
  std::string name;
  std::vector<std::shared_ptr<ArgumentProvider>> providers{
    std::make_shared<ArgumentProvider>(std::vector<Argument>{ Argument("--name", "-n", "the name", &name) })
  };
  ProcessArgumentService service(providers);

  service.parseArguments({ "--name", "first", "second" });

  REQUIRE(name == "first");
}

TEST_CASE("ProcessArgumentService: an unknown flag is reported on stderr and ignored")
{
  std::string name;
  std::vector<std::shared_ptr<ArgumentProvider>> providers{
    std::make_shared<ArgumentProvider>(std::vector<Argument>{ Argument("--name", "-n", "the name", &name) })
  };
  ProcessArgumentService service(providers);

  std::string cerr;
  {
    pasCerrCapture capture;
    service.parseArguments({ "--unknown", "value" });
    cerr = capture.str();
  }

  REQUIRE(name.empty());
  REQUIRE(cerr.find("--unknown") != std::string::npos);
}

TEST_CASE("ProcessArgumentService: -h prints the help text")
{
  std::string name;
  std::vector<std::shared_ptr<ArgumentProvider>> providers{
    std::make_shared<ArgumentProvider>(std::vector<Argument>{ Argument("--name", "-n", "the name", &name) })
  };
  ProcessArgumentService service(providers);

  std::string cout;
  {
    pasCoutCapture capture;
    service.parseArguments({ "-h" });
    cout = capture.str();
  }

  REQUIRE(cout.find("Arguments:") != std::string::npos);
  REQUIRE(cout.find("--name") != std::string::npos);
  REQUIRE(cout.find("--help, -h") != std::string::npos);
}

TEST_CASE("ProcessArgumentService: --help prints the help text")
{
  std::string name;
  std::vector<std::shared_ptr<ArgumentProvider>> providers{
    std::make_shared<ArgumentProvider>(std::vector<Argument>{ Argument("--name", "-n", "the name", &name) })
  };
  ProcessArgumentService service(providers);

  std::string cout;
  {
    pasCoutCapture capture;
    service.parseArguments({ "--help" });
    cout = capture.str();
  }

  REQUIRE(cout.find("Arguments:") != std::string::npos);
}

}// namespace lintel
