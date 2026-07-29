#ifndef HISTORYCLICOMPONENT_H
#define HISTORYCLICOMPONENT_H

#include "base_library/features/base/controller/HistoryApi.h"
#include "base_library/features/cli/models/CommandLineComponent.h"
#include "base_library/features/cli/models/CommandParser.h"

class HistoryCliComponent : public CommandLineComponent {
    static constexpr std::string_view n_name = "History";
    static constexpr std::string_view m_alias = "Hist";
    static constexpr std::string_view m_description = "The Component can be used to show the history of components";
    std::shared_ptr<HistoryApi> m_historyApi;

    // Commands
    enum Commands {
        Undefined,
        ShowHistories
    };

    // Flags
    std::optional<std::string> m_processName;
    std::optional<std::string> m_serviceName;
    std::optional<std::string> m_label;

    CommandParser<Commands, Undefined> m_commandParser;

    static void printHistories(const std::vector<HistoryDto> &histories);

public:
    explicit HistoryCliComponent(std::shared_ptr<HistoryApi> historyApi);

    void onHelp() override;

    void onCommand(const UserDto &userDto, const std::string &command,
                   const std::vector<std::string> &parameters) override;

    void onShowMenu() override;

    bool onMenu(const std::string &component) override;

    bool onExit() override;

    void printCommandList(std::set<std::string> menuAlias) override;

    std::vector<std::string> allCommandsOf() override;
};

#endif //HISTORYCLICOMPONENT_H
