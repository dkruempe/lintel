/** Test helper process: sleeps for the number of seconds passed as argv[1] and then exits.
 *
 * ProcessServiceTest needs a child process that stays alive and can be stopped, signalled or killed.
 * That used to be the system `sleep` utility, located through `boost::process::v1::search_path`.
 * When the utility was missing, 16 test cases hit `SKIP("sleep binary not found")` and quietly
 * stopped testing anything - a system package turned into a silent hole in the suite. This binary
 * is built next to the test binary instead, so the helper cannot go missing on a supported platform.
 *
 * Exit code 0 after sleeping, 2 on wrong usage or an unparsable argument. A negative or unparsable
 * duration is treated as zero seconds, matching `sleep 0`, so a malformed argument from a test can
 * never make the child hang forever.
 */

#include <chrono>
#include <cstdio>
#include <exception>
#include <string>
#include <thread>

int main(int argc, char **argv)
{
  if (argc != 2) {
    std::fputs("usage: base_test_sleeper <seconds>\n", stderr);
    return 2;
  }

  try {
    const int seconds = std::stoi(argv[1]);
    std::this_thread::sleep_for(std::chrono::seconds(seconds > 0 ? seconds : 0));
  } catch (const std::exception &exception) {
    std::fprintf(stderr, "base_test_sleeper: '%s' is not a number of seconds (%s)\n", argv[1], exception.what());
    return 2;
  }
  return 0;
}