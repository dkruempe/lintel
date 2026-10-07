#ifndef LINTEL_PROCESSNAME_H
#define LINTEL_PROCESSNAME_H

#include <filesystem>
#include <string>
#include <vector>

/**
 * Represents the name and arguments of the current process, typically derived from argc/argv.
 */
class ProcessName {
private:
    std::filesystem::path m_path;
    std::vector<std::string> m_args;

public:
    /**
     * Constructor from command-line arguments.
     * @param argc argument count
     * @param argv argument vector
     */
    ProcessName(int argc, char *const *argv) {
        m_path = argv[0];
        for (int i = 1; i < argc; i++) {
            m_args.push_back(argv[i]);
        }
    }

    /**
     * Constructor from a process path string.
     * @param processName path to the executable
     */
    explicit ProcessName(const std::string &processName) : m_path(processName) {};

    /** @return process executable filename */
    [[nodiscard]] std::string getProcessName() const {
        return m_path.filename().string();
    }

    /** @return full path to the executable */
    [[nodiscard]] const std::filesystem::path &getPath() const { return m_path; }

    /** @return command-line arguments */
    [[nodiscard]] const std::vector<std::string> &getArgs() const {
        return m_args;
    }
};

#endif  // LINTEL_PROCESSNAME_H
