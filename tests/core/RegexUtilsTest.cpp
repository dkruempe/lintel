#include <base_library/core/utils/RegexUtils.h>

#include <catch2/catch_all.hpp>

#include <string>

TEST_CASE("RegexUtils: simple pattern is valid") {
    std::string errorMessage;
    REQUIRE(RegexUtils::validatePattern("Group.*", errorMessage));
}

TEST_CASE("RegexUtils: plain quantifier is valid") {
    std::string errorMessage;
    REQUIRE(RegexUtils::validatePattern("^ab+c$", errorMessage));
}

TEST_CASE("RegexUtils: empty pattern is rejected") {
    std::string errorMessage;
    REQUIRE_FALSE(RegexUtils::validatePattern("", errorMessage));
    REQUIRE_FALSE(errorMessage.empty());
}

TEST_CASE("RegexUtils: pattern exceeding max length is rejected") {
    std::string pattern(RegexUtils::kMaxPatternLength + 1, 'a');
    std::string errorMessage;
    REQUIRE_FALSE(RegexUtils::validatePattern(pattern, errorMessage));
    REQUIRE_FALSE(errorMessage.empty());
}

TEST_CASE("RegexUtils: pattern at max length is accepted") {
    std::string pattern(RegexUtils::kMaxPatternLength, 'a');
    std::string errorMessage;
    REQUIRE(RegexUtils::validatePattern(pattern, errorMessage));
}

TEST_CASE("RegexUtils: nested quantifier is rejected") {
    std::string errorMessage;
    REQUIRE_FALSE(RegexUtils::validatePattern("(a+)+", errorMessage));
    REQUIRE_FALSE(RegexUtils::validatePattern("(a*)*", errorMessage));
    REQUIRE_FALSE(RegexUtils::validatePattern("(a+){2,}", errorMessage));
    REQUIRE_FALSE(RegexUtils::validatePattern("([a-z]+)+$", errorMessage));
}

TEST_CASE("RegexUtils: syntactically invalid pattern is rejected") {
    std::string errorMessage;
    REQUIRE_FALSE(RegexUtils::validatePattern("[", errorMessage));
    REQUIRE_FALSE(RegexUtils::validatePattern("(a", errorMessage));
}

TEST_CASE("RegexUtils: matches against valid pattern") {
    REQUIRE(RegexUtils::matches("Group.*", "GroupAlpha"));
    REQUIRE_FALSE(RegexUtils::matches("Group.*", "AlphaGroup"));
}

TEST_CASE("RegexUtils: matches is case sensitive") {
    REQUIRE(RegexUtils::matches("^[A-Z]+$", "ABC"));
    REQUIRE_FALSE(RegexUtils::matches("^[a-z]+$", "ABC"));
}

TEST_CASE("RegexUtils: matches returns false for unsafe pattern") {
    REQUIRE_FALSE(RegexUtils::matches("(a+)+", "aaaab"));
}

TEST_CASE("RegexUtils: matches returns false for invalid pattern") {
    REQUIRE_FALSE(RegexUtils::matches("[", "abc"));
}
