#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

// Standalone driver for the fuzzing harnesses.
//
// libFuzzer (Clang, -fsanitize=fuzzer) provides its own main(). Compilers
// without libFuzzer - GCC in particular - would not link a harness that only
// defines LLVMFuzzerTestOneInput, so this driver supplies main() and feeds each
// file given on the command line through the harness once. It mirrors LLVM's
// StandaloneFuzzTargetMain and exists so `BUILD_FUZZERS=ON` works everywhere and
// the harnesses can be smoke-tested against the seed corpus.

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size);

int main(int argc, char **argv)
{
  for (int i = 1; i < argc; ++i) {
    std::FILE *file = std::fopen(argv[i], "rb");
    if (file == nullptr) {
      std::perror(argv[i]);
      return 1;
    }
    std::fseek(file, 0, SEEK_END);
    const long length = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);
    std::vector<std::uint8_t> buffer(length > 0 ? static_cast<std::size_t>(length) : 0U);
    if (!buffer.empty()) {
      const std::size_t read = std::fread(buffer.data(), 1, buffer.size(), file);
      buffer.resize(read);
    }
    std::fclose(file);
    LLVMFuzzerTestOneInput(buffer.data(), buffer.size());
  }
  return 0;
}
