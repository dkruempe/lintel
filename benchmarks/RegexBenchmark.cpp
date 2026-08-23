#include <benchmark/benchmark.h>

#include "base_library/core/utils/RegexUtils.h"

static void BM_ValidatePattern_Simple(benchmark::State &state) {
    for (auto _ : state) {
        std::string err;
        RegexUtils::validatePattern("admin", err);
    }
}
BENCHMARK(BM_ValidatePattern_Simple);

static void BM_ValidatePattern_Wildcard(benchmark::State &state) {
    for (auto _ : state) {
        std::string err;
        RegexUtils::validatePattern("admins.*", err);
    }
}
BENCHMARK(BM_ValidatePattern_Wildcard);

static void BM_ValidatePattern_Complex(benchmark::State &state) {
    for (auto _ : state) {
        std::string err;
        RegexUtils::validatePattern("user_[a-z]+_[0-9]{2,4}", err);
    }
}
BENCHMARK(BM_ValidatePattern_Complex);

static void BM_ValidatePattern_ReDoS_Attempt(benchmark::State &state) {
    for (auto _ : state) {
        std::string err;
        RegexUtils::validatePattern("(a+)+b", err);
    }
}
BENCHMARK(BM_ValidatePattern_ReDoS_Attempt);

static void BM_Matches_Short(benchmark::State &state) {
    for (auto _ : state) {
        RegexUtils::matches("admin", "admin");
    }
}
BENCHMARK(BM_Matches_Short);

static void BM_Matches_Wildcard(benchmark::State &state) {
    for (auto _ : state) {
        RegexUtils::matches("admin.*", "administrator_123");
    }
}
BENCHMARK(BM_Matches_Wildcard);

static void BM_Matches_NoMatch(benchmark::State &state) {
    for (auto _ : state) {
        RegexUtils::matches("admin", "user");
    }
}
BENCHMARK(BM_Matches_NoMatch);

static void BM_Matches_LongString(benchmark::State &state) {
    std::string longStr(1000, 'x');
    longStr += "target";
    for (auto _ : state) {
        RegexUtils::matches(".*target", longStr);
    }
}
BENCHMARK(BM_Matches_LongString);
