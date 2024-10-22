#include "base_library/features/base/command_line/SharedMemoryCliComponent.h"

#include <base_library/core/utils/MemorySize.h>
#include <base_library/core/utils/StringUtils.h>
#include <base_library/core/utils/TableBuilder.h>
#include <rapidjson/prettywriter.h>

SharedMemoryCliComponent::SharedMemoryCliComponent(
        std::shared_ptr<SharedMemoryApi> sharedMemoryApi)
        : CommandLineComponent(m_name, m_alias),
          m_sharedMemoryApi(std::move(sharedMemoryApi)) {
    m_commandParser.addCommand(
            Command("show_segments", "Show all segments")
                    .addArgument({"--segment-name", "-s"}, &m_segmentName,
                                 "Segment Name or pattern matching"),
            ShowSegments);
    m_commandParser.addCommand(
            Command("show_repositories", "Show all repositories")
                    .addArgument({"--segment-name", "-s"}, &m_segmentName,
                                 "Segment Name or pattern matching")
                    .addArgument({"--repository-name", "-r"}, &m_repositoryName,
                                 "Repository Name or pattern matching"),
            ShowRepositories);
    m_commandParser.addCommand(Command("shrink_segment", "Shrink segment")
                                       .addArgument({"--segment-name", "-s"},
                                                    &m_segmentName, "Segment Name"),
                               ShrinkSegment);
    m_commandParser.addCommand(
            Command("grow_segment", "Shrink segment")
                    .addArgument({"--segment-name", "-s"}, &m_segmentName, "Segment Name")
                    .addArgument({"--grow-size", "-g"}, &m_size, "Grow Size"),
            GrowSegment);
    m_commandParser.addCommand(
            Command("export_repository", "Export Repository to json")
                    .addArgument({"--repository-name", "-r"}, &m_repositoryName,
                                 "Repository name")
                    .addArgument({"--uuid", "-u"}, &m_uuid, "UUID"),
            ExportRepository);
}

void SharedMemoryCliComponent::onCommand(
        const UserDto & /*userDto*/, const std::string &input,
        const std::vector<std::string> &parameters) {
    try {
        Commands command = m_commandParser.parse(input, parameters);
        switch (command) {
            case ShowSegments: {
                if (!m_segmentName.has_value()) {
                    m_segmentName = std::make_optional(".*");
                }
                auto res = m_sharedMemoryApi->allSegmentsOf(m_segmentName.value());
                printSegments(res);
                m_segmentName->clear();
                break;
            }
            case ShowRepositories: {
                if (!m_segmentName.has_value()) {
                    m_segmentName = std::make_optional(".*");
                }
                if (!m_repositoryName.has_value()) {
                    m_repositoryName = std::make_optional(".*");
                }
                auto res = m_sharedMemoryApi->allRepositoriesOf(
                        m_repositoryName.value(), m_segmentName.value());
                printRepositories(res);
                m_segmentName->clear();
                m_repositoryName->clear();
                break;
            }
            case ShrinkSegment: {
                if (!m_segmentName.has_value()) {
                    std::cout << "ERROR: please set segment name\n";
                    return;
                }
                m_sharedMemoryApi->shrinkOf(m_segmentName.value());
                m_segmentName->clear();
                break;
            }
            case GrowSegment: {
                if (!m_segmentName.has_value()) {
                    std::cout << "ERROR: please set segment name\n";
                    return;
                }
                if (!m_size.has_value()) {
                    std::cout << "ERROR: please set grow size\n";
                    return;
                }
                m_sharedMemoryApi->growOf(m_segmentName.value(), m_size.value());
                m_segmentName->clear();
                break;
            }
            case ExportRepository: {
                if (!m_uuid.has_value()) {
                    std::cout << "ERROR: please set uuid\n";
                    m_uuid->clear();
                    return;
                }
                std::string result = m_sharedMemoryApi->repositoryOf(
                        m_uuid.value());
                if (m_file.has_value()) {
                    std::string file =
                            StringUtils::replaceAll(m_file.value(), "~", getenv("HOME"));
                    std::filesystem::path path(file);
                    bool parentPathExits = std::filesystem::exists(path.parent_path());
                    if (!parentPathExits) {
                        std::filesystem::create_directories(path.parent_path());
                    }
                    std::ofstream out =
                            std::ofstream(path, std::ofstream::trunc | std::ofstream::out);
                    out << result;
                    std::cout << "Exported SharedMemory to >" << path.filename() << "<\n";
                } else {
                    rapidjson::Document document;
                    document.Parse(result.c_str());
                    rapidjson::StringBuffer stringBuffer;
                    rapidjson::PrettyWriter writer(stringBuffer);
                    document.Accept(writer);
                    std::cout << stringBuffer.GetString() << "\n";
                }
                m_file->clear();
                m_repositoryName->clear();
                m_segmentName->clear();
                m_type->clear();
                break;
            }
            default:
                break;
            case Undefined:
                break;
        }
    } catch (std::exception &exception) {
        std::cerr << "ERROR: " << exception.what() << "\n";
    }
}

void SharedMemoryCliComponent::printSegments(
        const std::vector<SharedMemorySegmentDto> &segments) {
    TableBuilder<11> builder;
    builder.add({"No.", "SegmentName", "Size", "AutoExtend", "AutoExtendSize",
                 "MaxSize", "CurrentSize", "FreeSize", "NamedObjects",
                 "UniqueObjects", "Sanity"});
    std::size_t count = 0;
    for (const auto &iter: segments) {
        builder.add({std::to_string(++count), iter.getName(),
                     MemorySize::serialize(iter.getSize()),
                     iter.isAutoExtend() ? "true" : "false",
                     MemorySize::serialize(iter.getAutoExtendSize()),
                     MemorySize::serialize(iter.getMaxSize()),
                     MemorySize::serialize(iter.getCurrentSize()),
                     MemorySize::serialize(iter.getFreeSize()),
                     std::to_string(iter.getAmountNamedObjects()),
                     std::to_string(iter.getAmountUniqueObjects()),
                     iter.isSanity() ? "true" : "false"});
    }
    std::cout << builder.build() << "\n";
}

bool SharedMemoryCliComponent::onExit() { return true; }

void SharedMemoryCliComponent::printCommandList(
        std::set<std::string> menuAlias) {
    m_commandParser.printCommandList(menuAlias);
}

void SharedMemoryCliComponent::onShowMenu() {
    std::cout << "No subMenu available\n";
}

bool SharedMemoryCliComponent::onMenu(const std::string & /*component*/) {
    return true;
}

void SharedMemoryCliComponent::onHelp() {
    m_commandParser.printHelp(getName(), getAlias(), m_description);
}

void SharedMemoryCliComponent::printRepositories(
        const std::vector<SharedMemoryRepositoryDto> &repositories) {
    TableBuilder<7> builder;
    builder.add(
            {"No.", "Uuid", "SegmentName", "RepositoryName", "Size", "Type", "Version"});
    std::size_t count = 0;
    for (const auto &iter: repositories) {
        builder.add({std::to_string(++count), iter.getUuid(), iter.getSegmentName(),
                     iter.getName(), std::to_string(iter.getSize()),
                     std::string(magic_enum::enum_name<>(iter.getType())),
                     std::to_string(iter.getCurrentVersion())});
    }
    std::cout << builder.build() << "\n";
}

std::vector<std::string> SharedMemoryCliComponent::allCommandsOf() {
    return m_commandParser.allCommandsOf();
}
