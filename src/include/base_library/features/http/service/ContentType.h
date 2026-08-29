#ifndef CPP_BASE_LIBRARY_CONTENTTYPE_H
#define CPP_BASE_LIBRARY_CONTENTTYPE_H

#include <array>
#include <string>
#include <string_view>

/** Represents an HTTP Content-Type with string-to-enum mapping */
class ContentType {
public:
    enum Value {
        UNDEFINED,
        TextCss,                   // text/css
        TextCsv,                   // text/csv
        TextPlain,                 // text/plain
        TextVtt,                   // text/vtt
        TextHtml,                  // text/html
        ImageApng,                 // image/apng
        ImageAvif,                 // image/avif
        ImageBmp,                  // image/bmp
        ImageGif,                  // image/gif
        ImagePng,                  // image/png
        ImageSvgXml,               // image/svg+xml
        ImageWebp,                 // image/webp
        ImageXIcon,                // image/x-icon
        ImageTiff,                 // image/tiff
        ImageJpeg,                 // image/jpeg
        VideoMp4,                  // video/mp4
        VideoMpeg,                 // video/mpeg
        VideoWebm,                 // video/webm
        AudioMp3,                  // audio/mp3
        AudioMpeg,                 // audio/mpeg
        AudioWebm,                 // audio/webm
        AudioWave,                 // audio/wave
        FontOtf,                   // font/otf
        FontTtf,                   // font/ttf
        FontWoff,                  // font/woff
        FontWoff2,                 // font/woff2
        ApplicationX7vCompressed,  // application/x-7z-compressed
        ApplicationAtomXml,        // application/atom+xml
        ApplicationPdf,            // application/pdf
        ApplicationJavascript,     // application/javascript
        ApplicationJson,           // application/json
        ApplicationRssXml,         // application/rss+xml
        ApplicationXTar,           // application/x-tar
        ApplicationXhtmlXml,       // application/xhtml+xml
        ApplicationXsltXml,        // application/xslt+xml
        ApplicationXml,            // application/xml
        ApplicationGzip,           // application/gzip
        ApplicationZip,            // application/zip
        ApplicationWasm            // application/wasm
    };

    struct Mapping {
        Value value;
        std::string_view name;
    };

    /** Sorted by name for binary search in build() */
    static constexpr std::array<Mapping, 39> kNameToValue = {{
        {ApplicationJavascript,    "application/javascript"},
        {ApplicationAtomXml,       "application/atom+xml"},
        {ApplicationGzip,          "application/gzip"},
        {ApplicationJson,          "application/json"},
        {ApplicationPdf,           "application/pdf"},
        {ApplicationRssXml,        "application/rss+xml"},
        {ApplicationWasm,          "application/wasm"},
        {ApplicationX7vCompressed, "application/x-7z-compressed"},
        {ApplicationXTar,          "application/x-tar"},
        {ApplicationXhtmlXml,      "application/xhtml+xml"},
        {ApplicationXml,           "application/xml"},
        {ApplicationXsltXml,       "application/xslt+xml"},
        {ApplicationZip,           "application/zip"},
        {AudioMp3,                 "audio/mp3"},
        {AudioMpeg,                "audio/mpeg"},
        {AudioWave,                "audio/wave"},
        {AudioWebm,                "audio/webm"},
        {FontOtf,                  "font/otf"},
        {FontTtf,                  "font/ttf"},
        {FontWoff,                 "font/woff"},
        {FontWoff2,                "font/woff2"},
        {ImageApng,                "image/apng"},
        {ImageAvif,                "image/avif"},
        {ImageBmp,                 "image/bmp"},
        {ImageGif,                 "image/gif"},
        {ImageJpeg,                "image/jpeg"},
        {ImagePng,                 "image/png"},
        {ImageSvgXml,              "image/svg+xml"},
        {ImageTiff,                "image/tiff"},
        {ImageWebp,                "image/webp"},
        {ImageXIcon,               "image/x-icon"},
        {TextCss,                  "text/css"},
        {TextCsv,                  "text/csv"},
        {TextHtml,                 "text/html"},
        {TextPlain,                "text/plain"},
        {TextVtt,                  "text/vtt"},
        {VideoMp4,                 "video/mp4"},
        {VideoMpeg,                "video/mpeg"},
        {VideoWebm,                "video/webm"},
    }};

    /** Sorted by value for lookup in getName() */
    static constexpr std::array<Mapping, 39> kValueToName = {{
        {TextCss,                  "text/css"},
        {TextCsv,                  "text/csv"},
        {TextPlain,                "text/plain"},
        {TextVtt,                  "text/vtt"},
        {TextHtml,                 "text/html"},
        {ImageApng,                "image/apng"},
        {ImageAvif,                "image/avif"},
        {ImageBmp,                 "image/bmp"},
        {ImageGif,                 "image/gif"},
        {ImagePng,                 "image/png"},
        {ImageSvgXml,              "image/svg+xml"},
        {ImageWebp,                "image/webp"},
        {ImageXIcon,               "image/x-icon"},
        {ImageTiff,                "image/tiff"},
        {ImageJpeg,                "image/jpeg"},
        {VideoMp4,                 "video/mp4"},
        {VideoMpeg,                "video/mpeg"},
        {VideoWebm,                "video/webm"},
        {AudioMp3,                 "audio/mp3"},
        {AudioMpeg,                "audio/mpeg"},
        {AudioWebm,                "audio/webm"},
        {AudioWave,                "audio/wave"},
        {FontOtf,                  "font/otf"},
        {FontTtf,                  "font/ttf"},
        {FontWoff,                 "font/woff"},
        {FontWoff2,                "font/woff2"},
        {ApplicationX7vCompressed, "application/x-7z-compressed"},
        {ApplicationAtomXml,       "application/atom+xml"},
        {ApplicationPdf,           "application/pdf"},
        {ApplicationJavascript,    "application/javascript"},
        {ApplicationJson,          "application/json"},
        {ApplicationRssXml,        "application/rss+xml"},
        {ApplicationXTar,          "application/x-tar"},
        {ApplicationXhtmlXml,      "application/xhtml+xml"},
        {ApplicationXsltXml,       "application/xslt+xml"},
        {ApplicationXml,           "application/xml"},
        {ApplicationGzip,          "application/gzip"},
        {ApplicationZip,           "application/zip"},
        {ApplicationWasm,          "application/wasm"},
    }};

    /** @param value the content type enum value */
    constexpr ContentType(Value value) : m_value(value) {}

    /** @param contentType string representation (e.g. "application/json") */
    explicit ContentType(const std::string &contentType);

    constexpr operator Value() const { return m_value; }

    explicit operator bool() = delete;

    /** @return the underlying enum value */
    [[nodiscard]] constexpr Value getValue() const { return m_value; }

    /** @return the MIME type string */
    [[nodiscard]] std::string getName() const;

private:
    /** Build enum value from content type string */
    static Value build(std::string_view contentType);

    Value m_value;
};

#endif  // CPP_BASE_LIBRARY_CONTENTTYPE_H
