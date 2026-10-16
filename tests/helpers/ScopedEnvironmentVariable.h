#ifndef LINTEL_SCOPEDENVIRONMENTVARIABLE_H
#define LINTEL_SCOPEDENVIRONMENTVARIABLE_H

#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

// setenv()/unsetenv() do not exist on Windows; the two fallbacks are _putenv_s() (MSVC). It takes
// "name=value" semantics through separate arguments and removes a variable when the value is empty.
// A silent no-op is not an option: it would make every guarded test run against the real environment
// instead of the intended one - the exact class of bug this helper exists to remove.

/** Overwrites an environment variable for the lifetime of an object and restores the previous state in the destructor.
 *
 * Why this exists: `setenv()` mutates process-global state that outlives a single test case. A test
 * that sets `CONFIG_DIRECTORY` and never restores it poisons every fixture constructed afterwards
 * *in the same process*, because `EnvironmentConfiguration` reads the variable in its constructor
 * (`src/features/base/configuration/EnvironmentConfiguration.cpp`) and caches it. The leaked value
 * silently redirects config, log and certificate lookups into a directory that does not exist.
 * `catch_discover_tests` (tests/CMakeLists.txt) hides this by starting each test case as its own
 * ctest process, so a green `ctest` run proves nothing about running the test binary directly.
 *
 * Usage - the guard must be declared *before* the object that reads the variable, otherwise the
 * object still sees the old value:
 * @code
 * ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test" };
 * auto envConfig = std::make_shared<EnvironmentConfiguration>();// reads the value set above
 * @endcode
 *
 * In a fixture the guard belongs in the member list: a local in the constructor body would be
 * destroyed when the constructor returns, so the test case itself would run with the leaked value.
 * Declare it as the first member - members initialize in declaration order, so it then runs before
 * the constructor body and is destroyed after everything that still reads the variable.
 */
namespace lintel {
class ScopedEnvironmentVariable
{
public:
  /** @param name name of the variable to overwrite
   *  @param value value to set for the lifetime of this object, or std::nullopt to remove it */
  ScopedEnvironmentVariable(std::string name, std::optional<std::string> value) : m_name(std::move(name))
  {
    // Remember the previous state verbatim, including "was set at all?": an empty string is a legal
    // value that has to be restored as such, which a plain std::string could not distinguish from unset.
    if (const char *previous = std::getenv(m_name.c_str())) { m_previousValue = previous; }
    if (value.has_value()) { set(value->c_str()); }
  }

  ~ScopedEnvironmentVariable() noexcept
  {
    // Best effort and deliberately non-throwing: a throwing destructor terminates the test binary. A
    // failure here can only mean the environment was already unusable, which the constructor reported.
    if (m_previousValue.has_value()) {
      set(m_previousValue->c_str());
    } else {
      unset();
    }
  }

  // Copying or moving would restore the old value twice or at the wrong time, so both are refused.
  ScopedEnvironmentVariable(const ScopedEnvironmentVariable &) = delete;
  ScopedEnvironmentVariable &operator=(const ScopedEnvironmentVariable &) = delete;
  ScopedEnvironmentVariable(ScopedEnvironmentVariable &&) = delete;
  ScopedEnvironmentVariable &operator=(ScopedEnvironmentVariable &&) = delete;

private:
  /** @param value new value of the variable */
  void set(const char *value)
  {
#if defined(_WIN32)
    if (_putenv_s(m_name.c_str(), value) != 0) {
      // Without this the test would silently run against the wrong value, which is the failure mode
      // this helper is meant to eliminate.
      throw std::runtime_error("ScopedEnvironmentVariable: _putenv_s(" + m_name + ") failed");
    }
#else
    if (setenv(m_name.c_str(), value, 1) != 0) {
      // Without this the test would silently run against the wrong value, which is the failure mode
      // this helper is meant to eliminate.
      throw std::runtime_error("ScopedEnvironmentVariable: setenv(" + m_name + ") failed");
    }
#endif// _WIN32
  }

  /** Remove the variable from the environment. */
  void unset() noexcept
  {
#if defined(_WIN32)
    // _putenv_s() removes the variable when the value is empty.
    static_cast<void>(_putenv_s(m_name.c_str(), ""));
#else
    unsetenv(m_name.c_str());
#endif// _WIN32
  }

  std::string m_name;
  std::optional<std::string> m_previousValue;
};

}// namespace lintel
#endif// LINTEL_SCOPEDENVIRONMENTVARIABLE_H