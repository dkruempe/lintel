#include <catch2/catch_all.hpp>

#include <filesystem>
#include <fstream>
#include <regex>
#include <string>

#include "lintel/core/exceptions/FileServiceFileExists.h"
#include "lintel/core/exceptions/FileServiceIsNotFileException.h"
#include "lintel/core/services/FileService.h"

namespace lintel {
namespace {

  /** Creates a scratch directory that is removed again when the fixture dies. */
  struct FileServiceFixture
  {
    std::filesystem::path directory;

    FileServiceFixture() : directory(std::filesystem::temp_directory_path() / "lintel_fileservice_test")
    {
      std::filesystem::remove_all(directory);
      std::filesystem::create_directories(directory);
    }

    ~FileServiceFixture() { std::filesystem::remove_all(directory); }

    std::filesystem::path writeFile(const std::string &name, const std::string &content) const
    {
      const auto path = directory / name;
      // write in binary mode so line endings are not translated on Windows
      std::ofstream out(path, std::ios::binary);
      out << content;
      return path;
    }
  };

}// namespace

TEST_CASE("FileService reports file existence")
{
  FileServiceFixture fixture;

  FileService missing(fixture.directory / "missing.txt");
  REQUIRE_FALSE(missing.isFile());
  REQUIRE_FALSE(missing.exists());

  const auto path = fixture.writeFile("present.txt", "hello");
  FileService present(path);
  REQUIRE(present.isFile());
  REQUIRE(present.exists());

  FileService directory(fixture.directory);
  REQUIRE_FALSE(directory.isFile());
  REQUIRE_FALSE(directory.exists());
}

TEST_CASE("FileService reads the whole file and exposes its name and path")
{
  FileServiceFixture fixture;
  const auto path = fixture.writeFile("read.txt", "line one\nline two\n");

  FileService file(path);
  REQUIRE(file.readFile() == "line one\nline two\n");
  REQUIRE(file.getName() == "read.txt");
  REQUIRE(file.getPath() == path);
  REQUIRE(file.getSize() == std::string("line one\nline two\n").size());
}

TEST_CASE("FileService writes file content, truncating in both modes")
{
  FileServiceFixture fixture;
  const auto path = fixture.writeFile("write.txt", "first");

  FileService file(path);
  // NOTE: despite the header wording, the default mode also truncates (std::ofstream
  // opens with ios::out); the overwrite flag only selects the mode explicitly.
  file.writeToFile("second");
  REQUIRE(file.readFile() == "second");

  file.writeToFile("reset", true);
  REQUIRE(file.readFile() == "reset");
}

TEST_CASE("FileService streams lines and matches a regex")
{
  FileServiceFixture fixture;
  const auto path = fixture.writeFile("lines.txt", "alpha\nbeta\ngamma\n");

  FileService file(path);
  auto stream = file.createStream();
  std::vector<std::string> lines;
  while (!stream.isEndOfFile()) { lines.push_back(stream.getLine()); }
  REQUIRE(lines == std::vector<std::string>{ "alpha", "beta", "gamma" });

  const auto matches = file.matches(std::regex(".*a.*"));
  REQUIRE(matches == std::vector<std::string>{ "alpha", "beta", "gamma" });
  REQUIRE(file.matches(std::regex("beta")).size() == 1);
}

TEST_CASE("FileService::createStream throws for a non file")
{
  FileServiceFixture fixture;
  FileService file(fixture.directory / "nope.txt");
  REQUIRE_THROWS_AS(file.createStream(), FileServiceIsNotFileException);
}

TEST_CASE("FileService deletes an existing file and tolerates a missing one")
{
  FileServiceFixture fixture;
  const auto path = fixture.writeFile("delete.txt", "x");

  FileService file(path);
  REQUIRE(file.exists());
  file.deleteFile();
  REQUIRE_FALSE(file.exists());
  file.deleteFile();// already gone: no throw
}

TEST_CASE("FileService creates a file with a requested size and rejects an existing one")
{
  FileServiceFixture fixture;

  FileService empty(fixture.directory / "empty.bin");
  empty.createFile(0);
  REQUIRE(empty.isFile());
  REQUIRE(empty.getSize() == 0);

  FileService sized(fixture.directory / "sized.bin");
  sized.createFile(1024);
  REQUIRE(sized.getSize() == 1024);

  REQUIRE_THROWS_AS(sized.createFile(10), FileServiceFileExists);
}

TEST_CASE("FileService creates a symlink to the file")
{
  FileServiceFixture fixture;
  const auto path = fixture.writeFile("target.txt", "content");
  const auto link = fixture.directory / "link.txt";

  FileService file(path);
  file.createSymlinkTo(link);
  REQUIRE(std::filesystem::is_symlink(link));
  REQUIRE(FileService(link).readFile() == "content");
}
}// namespace lintel
