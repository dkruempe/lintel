#include "base_library/features/http/service/ContentType.h"

#include <cctype>
#include <string_view>

// NOLINTNEXTLINE(cert-err58-cpp)
std::map<ContentType::Value, std::string> ContentType::m_valueToName = {
        {ContentType::UNDEFINED,
                                                "text/plain"},  // in case of error send message back as text/plain
        {ContentType::TextCss,                  "text/css"},
        {ContentType::TextCsv,                  "text/csv"},
        {ContentType::TextPlain,                "text/plain"},
        {ContentType::TextVtt,                  "text/vtt"},
        {ContentType::TextHtml,                 "text/html"},
        {ContentType::ImageApng,                "image/apng"},
        {ContentType::ImageAvif,                "image/avif"},
        {ContentType::ImageBmp,                 "image/bmp"},
        {ContentType::ImageGif,                 "image/gif"},
        {ContentType::ImagePng,                 "image/png"},
        {ContentType::ImageSvgXml,              "image/svg+xml"},
        {ContentType::ImageWebp,                "image/webp"},
        {ContentType::ImageXIcon,               "image/x-icon"},
        {ContentType::ImageTiff,                "image/tiff"},
        {ContentType::ImageJpeg,                "image/jpeg"},
        {ContentType::VideoMp4,                 "video/mp4"},
        {ContentType::VideoMpeg,                "video/mpeg"},
        {ContentType::VideoWebm,                "video/webm"},
        {ContentType::AudioMp3,                 "audio/mp3"},
        {ContentType::AudioMpeg,                "audio/mpeg"},
        {ContentType::AudioWebm,                "audio/webm"},
        {ContentType::AudioWave,                "audio/wave"},
        {ContentType::FontOtf,                  "font/otf"},
        {ContentType::FontTtf,                  "font/ttf"},
        {ContentType::FontWoff,                 "font/woff"},
        {ContentType::FontWoff2,                "font/woff2"},
        {ContentType::ApplicationX7vCompressed, "application/x-7z-compressed"},
        {ContentType::ApplicationAtomXml,       "application/atom+xml"},
        {ContentType::ApplicationPdf,           "application/pdf"},
        {ContentType::ApplicationJavascript,    "application/javascript"},
        {ContentType::ApplicationJson,          "application/json"},
        {ContentType::ApplicationRssXml,        "application/rss+xml"},
        {ContentType::ApplicationXTar,          "application/x-tar"},
        {ContentType::ApplicationXhtmlXml,      "application/xhtml+xml"},
        {ContentType::ApplicationXsltXml,       "application/xslt+xml"},
        {ContentType::ApplicationXml,           "application/xml"},
        {ContentType::ApplicationGzip,          "application/gzip"},
        {ContentType::ApplicationZip,           "application/zip"},
        {ContentType::ApplicationWasm,          "application/wasm"}};
// NOLINTNEXTLINE(cert-err58-cpp)
std::map<std::string_view, ContentType::Value> ContentType::m_nameToValue = {
        {"text/css",                    ContentType::TextCss},
        {"text/csv",                    ContentType::TextCsv},
        {"text/plain",                  ContentType::TextPlain},
        {"text/vtt",                    ContentType::TextVtt},
        {"text/html",                   ContentType::TextHtml},
        {"image/apng",                  ContentType::ImageApng},
        {"image/avif",                  ContentType::ImageAvif},
        {"image/bmp",                   ContentType::ImageBmp},
        {"image/gif",                   ContentType::ImageGif},
        {"image/png",                   ContentType::ImagePng},
        {"image/svg+xml",               ContentType::ImageSvgXml},
        {"image/webp",                  ContentType::ImageWebp},
        {"image/x-icon",                ContentType::ImageXIcon},
        {"image/tiff",                  ContentType::ImageTiff},
        {"image/jpeg",                  ContentType::ImageJpeg},
        {"video/mp4",                   ContentType::VideoMp4},
        {"video/mpeg",                  ContentType::VideoMpeg},
        {"video/webm",                  ContentType::VideoWebm},
        {"audio/mp3",                   ContentType::AudioMp3},
        {"audio/mpeg",                  ContentType::AudioMpeg},
        {"audio/webm",                  ContentType::AudioWebm},
        {"audio/wave",                  ContentType::AudioWave},
        {"font/otf",                    ContentType::FontOtf},
        {"font/ttf",                    ContentType::FontTtf},
        {"font/woff",                   ContentType::FontWoff},
        {"font/woff2",                  ContentType::FontWoff2},
        {"application/x-7z-compressed", ContentType::ApplicationX7vCompressed},
        {"application/atom+xml",        ContentType::ApplicationAtomXml},
        {"application/pdf",             ContentType::ApplicationPdf},
        {"application/javascript",      ContentType::ApplicationJavascript},
        {"application/json",            ContentType::ApplicationJson},
        {"application/rss+xml",         ContentType::ApplicationRssXml},
        {"application/x-tar",           ContentType::ApplicationXTar},
        {"application/xhtml+xml",       ContentType::ApplicationXhtmlXml},
        {"application/xslt+xml",        ContentType::ApplicationXsltXml},
        {"application/xml",             ContentType::ApplicationXml},
        {"application/gzip",            ContentType::ApplicationGzip},
        {"application/zip",             ContentType::ApplicationZip},
        {"application/wasm",            ContentType::ApplicationWasm}};

ContentType::Value ContentType::build(const std::string &contentType) {
    if (contentType.empty()) {
        return UNDEFINED;
    }
    // strip parameters like "; charset=utf-8" and surrounding whitespace
    const std::size_t parameterPos = contentType.find(';');
    std::string_view mediaType(
            contentType.data(),
            parameterPos == std::string::npos ? contentType.size() : parameterPos);
    while (!mediaType.empty() &&
           std::isspace(static_cast<unsigned char>(mediaType.front()))) {
        mediaType.remove_prefix(1);
    }
    while (!mediaType.empty() &&
           std::isspace(static_cast<unsigned char>(mediaType.back()))) {
        mediaType.remove_suffix(1);
    }
    auto found = m_nameToValue.find(mediaType);
    if (found == m_nameToValue.end()) {
        return UNDEFINED;
    }
    return found->second;
}

ContentType::ContentType(const std::string &contentType) : m_value(build(contentType)) {}

ContentType::ContentType(ContentType::Value value) : m_value(value) {}

const std::string &ContentType::getName() const {
    auto found = m_valueToName.find(m_value);
    if (found == m_valueToName.end()) {
        return m_valueToName.at(TextPlain);
    }
    return found->second;
}
