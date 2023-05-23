#include "base_library/core/utils/StringUtils.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <sstream>

std::vector<std::string> StringUtils::split(const std::string &s,
                                            char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

bool StringUtils::startsWith(const std::string &s, const std::string &start) {
    return s.rfind(start, 0) == 0;
}

bool StringUtils::endsWith(const std::string &s, const std::string &end) {
    return boost::algorithm::ends_with(s, end);
}

std::string StringUtils::replaceAll(const std::string &s, std::string &&replace,
                                    std::string &&with) {
    std::string res = s;
    boost::replace_all(res, replace, with);
    return res;
}
