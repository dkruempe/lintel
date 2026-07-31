#ifndef CPP_BASE_LIBRARY_REGEXUTILS_H
#define CPP_BASE_LIBRARY_REGEXUTILS_H

#include <cstddef>
#include <string>

/**
 * Hardening helpers for user-supplied regular expressions.
 *
 * std::regex is vulnerable to catastrophic backtracking ("ReDoS"). These
 * helpers apply a pragmatic heuristic layer before compiling a pattern:
 * length limit, rejection of clearly pathological constructs, and a
 * compilation check. They never throw on invalid input.
 */
class RegexUtils {
public:
    /** Maximum allowed length of a user-supplied pattern. */
    static constexpr std::size_t kMaxPatternLength = 128;

    /**
     * Validate a user-supplied pattern against basic ReDoS heuristics.
     * @param pattern the pattern to check
     * @param errorMessage receives a human-readable reason on failure
     * @return true if the pattern is safe to compile
     */
    static bool validatePattern(const std::string &pattern,
                                std::string &errorMessage);

    /**
     * Match text against a user-supplied pattern using the hardened checks.
     * Never throws; returns false for invalid/unsafe patterns.
     * @param pattern the pattern to match
     * @param text the text to match against
     * @return true if the pattern is safe and matches
     */
    static bool matches(const std::string &pattern, const std::string &text);

private:
    /** @return true if the pattern contains nested/overlapping quantifiers. */
    static bool hasNestedQuantifier(const std::string &pattern);
};

#endif  // CPP_BASE_LIBRARY_REGEXUTILS_H
