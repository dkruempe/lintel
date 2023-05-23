#ifndef CPP_BASE_LIBRARY_PROCESSNAME_H
#define CPP_BASE_LIBRARY_PROCESSNAME_H

#include <filesystem>

class ProcessName {
private:
    std::filesystem::path m_path;
    std::vector<std::string> m_args;

public:
    ProcessName(int argc, char *argv[]) {
        m_path = argv[0];
        for (int i = 1; i < argc; i++) {
            m_args.push_back(argv[i]);
        }
    }

    explicit ProcessName(const std::string &processName) : m_path(processName) {};

    [[nodiscard]] std::string getProcessName() const {
        return m_path.filename().string();
    }

    [[nodiscard]] const std::filesystem::path &getPath() const { return m_path; }

    [[nodiscard]] const std::vector<std::string> &getArgs() const {
        return m_args;
    }
};

#endif  // CPP_BASE_LIBRARY_PROCESSNAME_H
