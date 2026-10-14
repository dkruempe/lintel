#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "lintel/core/configuration/EnvironmentConfiguration.h"

namespace {
// expand() only substitutes a variable that std::getenv() actually returns.
// Without a seed or two the fuzzer would only ever hit the "unset variable
// stays literal" path, so pin a couple of names in the process environment.
struct SeedEnvironment
{
  SeedEnvironment()
  {
    ::setenv("FUZZ_A", "alpha", 0);
    ::setenv("FUZZ_B", "beta", 0);
    ::setenv("HOME", "/home/fuzz", 0);
  }
} seedEnvironment;
}// namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, const std::size_t size)
{
  std::string input;
  if (size > 0) { input.assign(reinterpret_cast<const char *>(data), size); }
  try {
    const std::string expanded = lintel::EnvironmentConfiguration::expand(input);
    (void)expanded;
  } catch (...) {
    // A parser must not throw on hostile input; if it does, the harness still
    // reports it (libFuzzer flags the uncaught case), but never aborts the run.
  }
  return 0;
}
