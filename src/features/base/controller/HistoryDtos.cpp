#include "base_library/features/base/controller/HistoryDtos.h"

HistoryDto::Shapes HistoryDto::m_shape{};

const std::vector<HistoryDto> &HistoryDtos::getHistories() const { return m_histories; }

void HistoryDtos::serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartArray();
  for (const auto &history : m_histories) { history.serialize(writer); }
  writer->EndArray();
}

void HistoryDtos::deserialize(const std::string &json)
{
  rapidjson::Document document;
  document.Parse(json.c_str());
  if (!document.IsArray()) { return; }
  for (const auto &history : document.GetArray()) {
    HistoryDto historyDto;
    historyDto.deserialize(history);
    m_histories.push_back(historyDto);
  }
}


std::vector<HistoryDto> HistoryDtos::build(const std::vector<HistoryEntry> entries)
{
  std::vector<HistoryDto> historiesDto;
  historiesDto.reserve(entries.size());
  std::transform(entries.begin(),
    entries.end(),
    std::back_inserter(historiesDto),
    [](const HistoryEntry &entry) { return HistoryDto(entry); });
  return historiesDto;
}

HistoryDtos::HistoryDtos(const std::vector<HistoryEntry> entries)
  : m_histories(build(entries)) {}