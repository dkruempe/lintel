#ifndef CPP_BASE_LIBRARY_PROCESSCLICOMPONENT_H
#define CPP_BASE_LIBRARY_PROCESSCLICOMPONENT_H

#include "base_library/features/http/controllers/ProcessApi.h"
#include "base_library/features/cli/models/CommandLineComponent.h"
#include "base_library/features/cli/models/CommandParser.h"

/** CLI component for process management (list, start, stop, terminate processes) */
class ProcessCliComponent : public CommandLineComponent {
private:
    static constexpr std::string_view m_name = "Process";
    static constexpr std::string_view m_alias = "Pros";
    static constexpr std::string_view m_description =
            "The component can be used to show all kind of processes.";
    std::shared_ptr<ProcessApi> m_processApi;

    // Commands
    enum Commands {
        Undefined,
        ShowProcesses,
        ShowProcessGroups,
        StartProcess,
        StopProcess,
        TerminateProcess
    };

    // Flags
    std::optional<std::string> m_processName;
    std::optional<std::string> m_processGroup;
    std::vector<std::string> m_arguments;
    std::optional<int32_t> m_restarts;
    std::string m_processId;

    CommandParser<Commands, Undefined> m_commandParser;

    /** Print formatted process info to console */
    static void printProcesses(const std::vector<ProcessInfoDto> &processInfo);

    /** Print formatted process groups to console */
    static void printProcessGroups(
            const std::vector<ProcessGroupDto> &processGroup);

public:
    /** @param processApi API for process operations */
    explicit ProcessCliComponent(std::shared_ptr<ProcessApi> processApi);

    void onCommand(const UserDto &userDto, const std::string &input,
                   const std::vector<std::string> &parameters) override;

    void onHelp() override;

    void onShowMenu() override;

    bool onMenu(const std::string &component) override;

    bool onExit() override;

    void printCommandList(std::set<std::string> menuAlias) override;

    std::vector<std::string> allCommandsOf() override;
};

#endif  // CPP_BASE_LIBRARY_PROCESSCLICOMPONENT_H
