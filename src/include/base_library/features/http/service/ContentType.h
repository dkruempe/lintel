#ifndef CPP_BASE_LIBRARY_CONTENTTYPE_H
#define CPP_BASE_LIBRARY_CONTENTTYPE_H

#include <map>
#include <string>

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

  ContentType(Value value);

  explicit ContentType(const std::string &contentType);

  operator Value() const { return m_value; }
  explicit operator bool() = delete;

  Value getValue() { return m_value; }

  [[nodiscard]] const std::string &getName() const;

 private:
  Value build(const std::string &contentType);
  Value m_value;
  std::map<std::string_view, Value> m_nameToValue = {
      {"text/css", TextCss},
      {"text/csv", TextCsv},
      {"text/plain", TextPlain},
      {"text/vtt", TextVtt},
      {"text/html", TextHtml},
      {"image/apng", ImageApng},
      {"image/avif", ImageAvif},
      {"image/bmp", ImageBmp},
      {"image/gif", ImageGif},
      {"image/png", ImagePng},
      {"image/svg+xml", ImageSvgXml},
      {"image/webp", ImageWebp},
      {"image/x-icon", ImageXIcon},
      {"image/tiff", ImageTiff},
      {"image/jpeg", ImageJpeg},
      {"video/mp4", VideoMp4},
      {"video/mpeg", VideoMpeg},
      {"video/webm", VideoWebm},
      {"audio/mp3", AudioMp3},
      {"audio/mpeg", AudioMpeg},
      {"audio/webm", AudioWebm},
      {"audio/wave", AudioWave},
      {"font/otf", FontOtf},
      {"font/ttf", FontTtf},
      {"font/woff", FontWoff},
      {"font/woff2", FontWoff2},
      {"application/x-7z-compressed", ApplicationX7vCompressed},
      {"application/atom+xml", ApplicationAtomXml},
      {"application/pdf", ApplicationPdf},
      {"application/javascript", ApplicationJavascript},
      {"application/json", ApplicationJson},
      {"application/rss+xml", ApplicationRssXml},
      {"application/x-tar", ApplicationXTar},
      {"application/xhtml+xml", ApplicationXhtmlXml},
      {"application/xslt+xml", ApplicationXsltXml},
      {"application/xml", ApplicationXml},
      {"application/gzip", ApplicationGzip},
      {"application/zip", ApplicationZip},
      {"application/wasm", ApplicationWasm}};
  std::map<Value, std::string> m_valueToName = {
      {UNDEFINED,
       "text/plain"},  // in case of error send message back as text/plain
      {TextCss, "text/css"},
      {TextCsv, "text/csv"},
      {TextPlain, "text/plain"},
      {TextVtt, "text/vtt"},
      {TextHtml, "text/html"},
      {ImageApng, "image/apng"},
      {ImageAvif, "image/avif"},
      {ImageBmp, "image/bmp"},
      {ImageGif, "image/gif"},
      {ImagePng, "image/png"},
      {ImageSvgXml, "image/svg+xml"},
      {ImageWebp, "image/webp"},
      {ImageXIcon, "image/x-icon"},
      {ImageTiff, "image/tiff"},
      {ImageJpeg, "image/jpeg"},
      {VideoMp4, "video/mp4"},
      {VideoMpeg, "video/mpeg"},
      {VideoWebm, "video/webm"},
      {AudioMp3, "audio/mp3"},
      {AudioMpeg, "audio/mpeg"},
      {AudioWebm, "audio/webm"},
      {AudioWave, "audio/wave"},
      {FontOtf, "font/otf"},
      {FontTtf, "font/ttf"},
      {FontWoff, "font/woff"},
      {FontWoff2, "font/woff2"},
      {ApplicationX7vCompressed, "application/x-7z-compressed"},
      {ApplicationAtomXml, "application/atom+xml"},
      {ApplicationPdf, "application/pdf"},
      {ApplicationJavascript, "application/javascript"},
      {ApplicationJson, "application/json"},
      {ApplicationRssXml, "application/rss+xml"},
      {ApplicationXTar, "application/x-tar"},
      {ApplicationXhtmlXml, "application/xhtml+xml"},
      {ApplicationXsltXml, "application/xslt+xml"},
      {ApplicationXml, "application/xml"},
      {ApplicationGzip, "application/gzip"},
      {ApplicationZip, "application/zip"},
      {ApplicationWasm, "application/wasm"}};
};

#endif  // CPP_BASE_LIBRARY_CONTENTTYPE_H
