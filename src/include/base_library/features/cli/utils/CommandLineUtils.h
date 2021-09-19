#ifndef CPP_BASE_LIBRARY_COMMANDLINEUTILS_H
#define CPP_BASE_LIBRARY_COMMANDLINEUTILS_H

#include <set>
#include <sstream>

class CommandLineUtils {
 public:
  static void disableOfInputEcho();
  static void enableOfInputEcho();
  static void clear();
  static void disableOfCanonicalMode();
  static void enableOfCanonicalMode();

  template <class CharT, class Traits, class Allocator>
  static std::basic_istream<CharT, Traits>& getline(
      std::basic_istream<CharT, Traits>& is,
      std::basic_string<CharT, Traits, Allocator>& str, std::set<CharT> &&delims,
      CharT& delimReturn) {
    std::ios_base::iostate state = std::ios_base::goodbit;
    typename std::basic_istream<CharT, Traits>::sentry sen(is, true);
    if (sen) {
      try {
        str.clear();
        std::streamsize extr = 0;
        bool isRunning = true;
        while (isRunning) {
          typename Traits::int_type i = is.rdbuf()->sbumpc();
          if (Traits::eq_int_type(i, Traits::eof())) {
            state |= std::ios_base::eofbit;
            break;
          }
          ++extr;
          CharT ch = Traits::to_char_type(i);
          for (const auto& delim : delims) {
            if (Traits::eq(ch, delim)) {
              isRunning = false;
              delimReturn = delim;
              break;
            }
          }
          if (!isRunning) {
            break;
          }
          str.push_back(ch);
          if (str.size() == str.max_size()) {
            state |= std::ios_base::failbit;
            break;
          }
        }
        if (extr == 0) {
          state |= std::ios_base::failbit;
        }
      } catch (...) {
        state |= std::ios_base::badbit;
        is.__setstate_nothrow(state);
        if (is.exceptions() & std::ios_base::badbit) {
          throw;
        }
      }
      is.setstate(state);
    }
    return is;
  };
};

#endif  // CPP_BASE_LIBRARY_COMMANDLINEUTILS_H
