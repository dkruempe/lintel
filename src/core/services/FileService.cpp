#include "base_library/core/services/FileService.h"

#include <boost/next_prior.hpp>
#include <streambuf>

import base_library.core.exceptions;

FileService::Stream::Stream(const std::filesystem::path &path) : m_file(path.string())
{
  getLine();
}

bool FileService::Stream::isEndOfFile() const { return m_endOfFile; }

std::string FileService::Stream::getLine()
{
  std::string line = m_nextLine;
  m_endOfFile = !std::getline(m_file, m_nextLine);
  return line;
}

FileService::FileService(std::filesystem::path path) : m_path(std::move(path)) {}

bool FileService::isFile() { return std::filesystem::is_regular_file(m_path); }

bool FileService::exists() { return std::filesystem::exists(m_path) && isFile(); }

std::string FileService::readFile()
{
  std::ifstream file(m_path.string());
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

std::string FileService::getName() { return m_path.filename(); }

std::filesystem::path FileService::getPath() { return m_path; }

void FileService::writeToFile(const std::string &content, bool overwrite /* default = false */)
{
  std::ofstream out;
  if (overwrite) {
    out = std::ofstream(m_path, std::ofstream::trunc | std::ofstream::out);
  } else {
    out = std::ofstream(m_path);
  }
  out << content;
}

void FileService::createSymlinkTo(const std::filesystem::path &to)
{
  std::filesystem::create_symlink(m_path, to);
}

std::vector<std::string> FileService::matches(const std::regex &regex)
{
  FileService::Stream stream(m_path.string());
  std::vector<std::string> matches;
  while (!stream.isEndOfFile()) {
    const std::string &line = stream.getLine();
    if (std::regex_match(line, regex)) { matches.push_back(line); }
  }
  return matches;
}

FileService::Stream FileService::createStream()
{
  if (!isFile()) { throw FileServiceIsNotFileException(m_path); }
  return Stream(m_path);
}

void FileService::deleteFile()
{
  if (!exists()) { return; }
  std::filesystem::remove(m_path);
}

std::size_t FileService::getSize() { return std::filesystem::file_size(m_path); }

void FileService::createFile(std::size_t sizeOfFile)
{
  if (exists()) { throw FileServiceFileExists(m_path); }
  std::filebuf fbuf;
  auto result =
    fbuf.open(m_path, std::ios_base::in | std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
  if (result == nullptr || !result->is_open()) { throw std::runtime_error("File is not able to be opened"); }
  // Set the size
  if (sizeOfFile > 0) {
    fbuf.pubseekoff(static_cast<long long>(sizeOfFile - 1), std::ios_base::beg);
    fbuf.sputc(0);
  }
}
