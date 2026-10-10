#include "lintel/features/cli/components/HistoryCliComponent.h"

#include "lintel/core/services/StringifyService.h"
#include "lintel/core/utils/TableBuilder.h"

HistoryCliComponent::HistoryCliComponent(
        std::shared_ptr<HistoryApi> historyApi) : CommandLineComponent(n_name, m_alias),
                                                  m_historyApi(std::move(historyApi)) {
    m_commandParser.addCommand(
            Command("show_histories", "Shows all histories for set parameters").addArgument(
                            {"--process-name", "-p"}, &m_processName, "Process Name of history entries")
                    .addArgument({"--service-name", "-s"}, &m_serviceName, "Service Name of history entries)")
                    .addArgument({"--label", "-l"}, &m_label, "Label Name of history entries"), ShowHistories);
}

void HistoryCliComponent::onCommand(const UserDto &userDto, const std::string &command,
                                    const std::vector<std::string> &parameters) {
    try {
        Commands cmd = m_commandParser.parse(command, parameters);
        if (cmd == ShowHistories) {
            const std::string processNameTemp = m_processName.has_value() ? m_processName.value() : ".*";
            const std::string serviceNameTemmp = m_serviceName.has_value() ? m_serviceName.value() : ".*";
            const std::string labelTemp = m_label.has_value() ? m_label.value() : ".*";
            auto histories = m_historyApi->allOf(processNameTemp, serviceNameTemmp, labelTemp);
            printHistories(histories);
        }
    } catch (const std::exception &exception) {
        std::cerr << "ERROR: " << exception.what() << "\n";
    }
}

void HistoryCliComponent::printHistories(const std::vector<HistoryDto> &histories) {
    TableBuilder<7> builder;
    builder.add({"No.", "Process", "Service", "Label", "Text", "UUID", "Created Timestamp"});
    std::size_t iter = 0;
    for (const auto &history: histories) {
        builder.add({
                            std::to_string(++iter), history.getProcessName(), history.getServiceName(),
                            history.getLabel(),
                            history.getText(),
                            history.getUuid(),
                            StringifyService<date::sys_time<std::chrono::microseconds> >::serializeToString(
                                    history.getCreatedTimestamp())
                    });
    }
    std::cout << builder.build() << "\n";
}


void HistoryCliComponent::onHelp() {
    m_commandParser.printHelp(getName(), getAlias(), m_description);
}


bool HistoryCliComponent::onMenu(const std::string &component) {
    return false;
}

void HistoryCliComponent::onShowMenu() {
    std::cout << "No submenu available!\n";
}


bool HistoryCliComponent::onExit() {
    return true;
}


void HistoryCliComponent::printCommandList(std::set<std::string> menuAlias) {
    m_commandParser.printCommandList(menuAlias);
}


std::vector<std::string> HistoryCliComponent::allCommandsOf() {
    return m_commandParser.allCommandsOf();
}
