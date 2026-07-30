#ifndef CPP_BASE_LIBRARY_CONTENTTYPE_H
#define CPP_BASE_LIBRARY_CONTENTTYPE_H

#include <map>
#include <string>

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

    /** @param value the content type enum value */
    ContentType(Value value);

    /** @param contentType string representation (e.g. "application/json") */
    explicit ContentType(const std::string &contentType);

    operator Value() const { return m_value; }

    explicit operator bool() = delete;

    /** @return the underlying enum value */
    Value getValue() { return m_value; }

    /** @return the MIME type string */
    [[nodiscard]] const std::string &getName() const;

private:
    /** Build enum value from content type string */
    static Value build(const std::string &contentType);

    Value m_value;
    static std::map<std::string_view, Value> m_nameToValue;
    static std::map<Value, std::string> m_valueToName;
};

#endif  // CPP_BASE_LIBRARY_CONTENTTYPE_H
