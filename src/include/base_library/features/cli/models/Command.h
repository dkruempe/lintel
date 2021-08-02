#ifndef CPP_BASE_LIBRARY_COMMAND_H
#define CPP_BASE_LIBRARY_COMMAND_H

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

class Command {
 public:
  // These are the possible variables the options may point to. Bool and
  // std::string are handled in a special way, all other values are parsed
  // with a std::stringstream. This std::variant can be easily extended if
  // the stream operator>> is overloaded. If not, you have to add a special
  // case to the parse() method.
  typedef std::variant<int32_t*, uint32_t*, double*, float*, bool*,
                       std::string*>
      Value;

  // The description is printed as part of the help message.
  Command(std::string command, std::string description);

  Command(std::string_view command, std::string description);

  // Adds a possible option. A typical call would be like this:
  // bool printHelp = false;
  // cmd.addArgument({"--help", "-h"}, &printHelp, "Print this help message");
  // Then, after parse() has been called, printHelp will be true if the user
  // provided the flag.
  void addArgument(const std::vector<std::string>& flags, Value defaultValue,
                   const std::string& help);

  // Prints the description given to the constructor and the help
  // for each option.
  void printHelp(std::ostream& os = std::cout) const;

  [[nodiscard]] std::string getCommand() const;

  // The command line arguments are traversed from start to end. That means,
  // if an option is set multiple times, the last will be the one which is
  // finally used. This call will throw a std::runtime_error if a value is
  // missing for a given option. Unknown flags will cause a warning on
  // std::cerr.
  void parse(const std::string& command,
             const std::vector<std::string>& flags) const;

 private:
  struct Argument {
    std::vector<std::string> m_flags;
    Value m_value;
    std::string m_help;
  };

  std::string m_command;
  std::string m_description;
  std::map<std::string, Argument> m_argumentsMap;
  std::vector<Argument> m_arguments;
};

#endif  // CPP_BASE_LIBRARY_COMMAND_H
