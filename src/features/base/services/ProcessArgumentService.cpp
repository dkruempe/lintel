#include "base_library/features/base/services/ProcessArgumentService.h"

#include <iomanip>
#include <sstream>

#include "base_library/core/services/LoggerService.h"

void ProcessArgumentService::parseArguments(
    const std::vector<std::string> &arguments) {
  std::string flag;
  std::vector<std::string> args;
  Argument *temp = nullptr;
  for (const auto &argument : arguments) {
    if (argument.empty()) {
      continue;
    }
    LOG_TRACE("{}", argument);
    // is flag ?
    // save existing flag informations
    if (argument[0] == '-' && !flag.empty()) {
      if (temp == nullptr) {
        flag.clear();
        args.clear();
      }
      if (args.size() > 1) {
        LOG_ERROR(
            "ignores other parameters bc. only 1 value per flag is currently "
            "allowed");
      }
      if (args.empty()) {
        temp->parse("");
        LOG_TRACE("parse flag {} with not argument", flag);
      } else {
        temp->parse(args[0]);
        LOG_TRACE("parse flag {} with argument {}", flag, args[0]);
      }
      flag.clear();
      args.clear();
    }
    // temp save of new flag
    if (argument[0] == '-') {
      flag = argument;
      try {
        temp = &m_flagArgumentMap.at(flag);
      } catch (const std::exception &exception) {
        // extra handling for help function
        if (flag == "-h" || flag == "--help") {
          std::cout << "Arguments:\n";
          for (const auto &arg : m_arguments) {
            arg.printHelp();
          }
          std::cout << "\t --help, -h: list all arguments\n";
        } else {
          LOG_ERROR("flag {} not defined", flag);
          std::cerr << flag << ": not defined => ignore flag \n";
        }
        flag.clear();
        args.clear();
      }
      continue;
    }
    args.push_back(argument);
  }
  if (!flag.empty()) {
    if (args.size() > 1) {
      LOG_ERROR(
          "ignores other parameters bc. only 1 value per flag is currently "
          "allowed");
    }
    if (temp == nullptr) {
      return;
    }
    if (args.empty()) {
      temp->parse("");
      LOG_TRACE("parse flag {} with not argument", flag);
    } else {
      temp->parse(args[0]);
      LOG_TRACE("parse falg {} with argument {}", flag, args[0]);
    }
  }
}
ProcessArgumentService::ProcessArgumentService(
    const std::vector<std::shared_ptr<ArgumentProvider>> &argumentProviders)
    : m_flagArgumentMap(init(argumentProviders)),
      m_arguments(initArgs(argumentProviders)) {
  LOG_TRACE("count of argument providers {}", argumentProviders.size());
}
std::map<std::string, Argument> ProcessArgumentService::init(
    const std::vector<std::shared_ptr<ArgumentProvider>> &argumentProviders) {
  std::map<std::string, Argument> map;
  for (auto &argumentProvider : argumentProviders) {
    for (const auto &argument : argumentProvider->provide()) {
      map.insert({argument.getFlag(), argument});
      map.insert({argument.getShortFlag(), argument});
    }
  }
  return map;
}
std::vector<Argument> ProcessArgumentService::initArgs(
    const std::vector<std::shared_ptr<ArgumentProvider>> &vector) {
  std::vector<Argument> args;
  for (const auto &item : vector) {
    const auto &vec = item->provide();
    for (const auto &iter : vec) {
      args.push_back(iter);
    }
  }
  return args;
}
const std::vector<Argument> &ArgumentProvider::provide() const {
  return m_arguments;
}
ArgumentProvider::ArgumentProvider(std::vector<Argument> arguments)
    : m_arguments(std::move(arguments)) {}
Argument::Argument(std::string flag, std::string shortFlag,
                   std::string description, const Argument::Value &value)
    : m_flag(std::move(flag)), m_shortFlag(std::move(shortFlag)),
      m_description(std::move(description)), m_value(value) {}
void Argument::printHelp(std::ostream &os) const {
  os << "\t" << m_flag << ", " << m_shortFlag << " " << m_description << "\n";
}
void Argument::parse(const std::string &argument) {
  if (std::holds_alternative<std::optional<std::string> *>(m_value)) {
    *std::get<std::optional<std::string> *>(m_value) = std::nullopt;
  } else if (std::holds_alternative<std::optional<bool> *>(m_value)) {
    *std::get<std::optional<bool> *>(m_value) = std::nullopt;
  } else if (std::holds_alternative<std::optional<float> *>(m_value)) {
    *std::get<std::optional<float> *>(m_value) = std::nullopt;
  } else if (std::holds_alternative<std::optional<double> *>(m_value)) {
    *std::get<std::optional<double> *>(m_value) = std::nullopt;
  } else if (std::holds_alternative<std::optional<uint32_t> *>(m_value)) {
    *std::get<std::optional<uint32_t> *>(m_value) = std::nullopt;
  } else if (std::holds_alternative<std::optional<int32_t> *>(m_value)) {
    *std::get<std::optional<int32_t> *>(m_value) = std::nullopt;
  }
  if (std::holds_alternative<bool *>(m_value)) {
    if (!argument.empty() && argument != "true" && argument != "false") {
      return;
    }
    *std::get<bool *>(m_value) = (argument != "false");
  } else if (std::holds_alternative<std::optional<bool> *>(m_value)) {
    if (!argument.empty() && argument != "true" && argument != "false") {
      return;
    }
    *std::get<std::optional<bool> *>(m_value) =
        std::make_optional(argument != "false");
  }
  // In all other cases there must be a value.
  else if (argument.empty()) {
    throw std::runtime_error("Failed to parse command line arguments: "
                             "Missing value for argument \"" +
                             m_shortFlag + "\"!");
  }
  // For a std::string, we take the entire value.
  else if (std::holds_alternative<std::string *>(m_value)) {
    *std::get<std::string *>(m_value) = argument;
  } else if (std::holds_alternative<std::optional<std::string> *>(m_value)) {
    *std::get<std::optional<std::string> *>(m_value) =
        std::make_optional(argument);
    LOG_TRACE("set optional value of argument {}", argument);
  } else {
    std::visit(
        [&argument](auto &&arg) {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same<T, std::optional<std::string> *>::value) {
            std::string temp;
            std::stringstream sstr(argument);
            sstr >> temp;
            *arg = std::make_optional(temp);
          } else if constexpr (std::is_same<T, std::optional<bool> *>::value) {
            bool temp;
            std::stringstream sstr(argument);
            sstr >> temp;
            *arg = std::make_optional(temp);
          } else if constexpr (std::is_same<T,
                                            std::optional<double> *>::value) {
            double temp;
            std::stringstream sstr(argument);
            sstr >> temp;
            *arg = std::make_optional(temp);
          } else if constexpr (std::is_same<T, std::optional<float> *>::value) {
            double temp;
            std::stringstream sstr(argument);
            sstr >> temp;
            *arg = std::make_optional(temp);
          } else if constexpr (std::is_same<T,
                                            std::optional<uint32_t> *>::value) {
            uint32_t temp;
            std::stringstream sstr(argument);
            sstr >> temp;
            *arg = std::make_optional(temp);
          } else if constexpr (std::is_same<T,
                                            std::optional<int32_t> *>::value) {
            int32_t temp;
            std::stringstream sstr(argument);
            sstr >> temp;
            *arg = std::make_optional(temp);
          } else {
            std::stringstream sstr(argument);
            sstr >> *arg;
          }
        },
        m_value);
  }
}
const std::string &Argument::getFlag() const { return m_flag; }
const std::string &Argument::getShortFlag() const { return m_shortFlag; }
