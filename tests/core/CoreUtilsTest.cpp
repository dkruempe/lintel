#include <base_library/core/utils/StringUtils.h>
#include <base_library/core/utils/UUID.h>
#include <base_library/core/utils/MemorySize.h>
#include <base_library/core/services/FileService.h>
#include <base_library/core/utils/Cryption.h>

#include <catch2/catch_all.hpp>

#include <filesystem>
#include <regex>

TEST_CASE("StringUtils: split") {
    auto tokens = StringUtils::split("a,b,c", ',');
    REQUIRE(tokens.size() == 3);
    REQUIRE(tokens[0] == "a");
    REQUIRE(tokens[1] == "b");
    REQUIRE(tokens[2] == "c");
}

TEST_CASE("StringUtils: split with empty parts") {
    auto tokens = StringUtils::split("a,,c", ',');
    REQUIRE(tokens.size() == 3);
    REQUIRE(tokens[0] == "a");
    REQUIRE(tokens[1] == "");
    REQUIRE(tokens[2] == "c");
}

TEST_CASE("StringUtils: split single token") {
    auto tokens = StringUtils::split("hello", ',');
    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0] == "hello");
}

TEST_CASE("StringUtils: startsWith") {
    REQUIRE(StringUtils::startsWith("hello world", "hello"));
    REQUIRE(StringUtils::startsWith("hello", "hello"));
    REQUIRE_FALSE(StringUtils::startsWith("hello", "world"));
    REQUIRE_FALSE(StringUtils::startsWith("hi", "hello"));
    REQUIRE(StringUtils::startsWith("", ""));
}

TEST_CASE("StringUtils: endsWith") {
    REQUIRE(StringUtils::endsWith("hello world", "world"));
    REQUIRE(StringUtils::endsWith("hello", "hello"));
    REQUIRE_FALSE(StringUtils::endsWith("hello", "world"));
    REQUIRE_FALSE(StringUtils::endsWith("hi", "hello"));
    REQUIRE(StringUtils::endsWith("", ""));
}

TEST_CASE("StringUtils: replaceAll") {
    std::string result = StringUtils::replaceAll("foo bar foo", "foo", "baz");
    REQUIRE(result == "baz bar baz");
}

TEST_CASE("StringUtils: replaceAll no match") {
    std::string result = StringUtils::replaceAll("hello world", "xyz", "abc");
    REQUIRE(result == "hello world");
}

TEST_CASE("UUID: generate returns valid UUID format") {
    std::string uuid = UUID::generate();
    REQUIRE(uuid.size() == 36);
    REQUIRE(uuid[8] == '-');
    REQUIRE(uuid[13] == '-');
    REQUIRE(uuid[18] == '-');
    REQUIRE(uuid[23] == '-');
    std::regex uuidRegex(
        "[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}");
    REQUIRE(std::regex_match(uuid, uuidRegex));
}

TEST_CASE("UUID: generate produces unique values") {
    std::string u1 = UUID::generate();
    std::string u2 = UUID::generate();
    REQUIRE(u1 != u2);
}

TEST_CASE("MemorySize: deserialize plain number") {
    REQUIRE(MemorySize::deserialize("1024") == 1024);
}

TEST_CASE("MemorySize: deserialize kB") {
    REQUIRE(MemorySize::deserialize("1kB") == 1000);
    REQUIRE(MemorySize::deserialize("256kB") == 256000);
}

TEST_CASE("MemorySize: deserialize MB") {
    REQUIRE(MemorySize::deserialize("1MB") == 1000000);
    REQUIRE(MemorySize::deserialize("10MB") == 10000000);
}

TEST_CASE("MemorySize: deserialize GB") {
    REQUIRE(MemorySize::deserialize("1GB") == 1000000000);
}

TEST_CASE("MemorySize: deserialize TB") {
    REQUIRE(MemorySize::deserialize("1TB") == 1000000000000);
}

TEST_CASE("MemorySize: deserialize byte suffix") {
    REQUIRE(MemorySize::deserialize("512B") == 512);
}

TEST_CASE("MemorySize: serialize bytes") {
    std::string result = MemorySize::serialize(500);
    REQUIRE(result == "500B");
}

TEST_CASE("MemorySize: serialize KB") {
    std::string result = MemorySize::serialize(1500);
    REQUIRE(result.find("KB") != std::string::npos);
    REQUIRE(result.find("1.5") != std::string::npos);
}

TEST_CASE("MemorySize: serialize MB") {
    std::string result = MemorySize::serialize(5000000);
    REQUIRE(result.find("5MB") != std::string::npos);
}

TEST_CASE("MemorySize: serialize GB") {
    std::string result = MemorySize::serialize(2000000000);
    REQUIRE(result.find("2GB") != std::string::npos);
}

TEST_CASE("MemorySize: roundtrip serialize deserialize integer value") {
    std::size_t original = 5000000;
    std::string serialized = MemorySize::serialize(original);
    std::size_t deserialized = MemorySize::deserialize(serialized);
    REQUIRE(deserialized == original);
}

TEST_CASE("FileService: write and read file") {
    auto tmpPath = std::filesystem::temp_directory_path() / "fileservice_test.txt";
    {
        FileService file(tmpPath);
        file.writeToFile("hello world");
    }
    {
        FileService file(tmpPath);
        REQUIRE(file.exists());
        REQUIRE(file.isFile());
        REQUIRE(file.readFile() == "hello world");
        REQUIRE(file.getName() == "fileservice_test.txt");
        file.deleteFile();
    }
    REQUIRE_FALSE(std::filesystem::exists(tmpPath));
}

TEST_CASE("FileService: writeToFile overwrite") {
    auto tmpPath = std::filesystem::temp_directory_path() / "fileservice_overwrite.txt";
    {
        FileService file(tmpPath);
        file.writeToFile("first");
    }
    {
        FileService file(tmpPath);
        file.writeToFile("second", true);
        REQUIRE(file.readFile() == "second");
        file.deleteFile();
    }
}

TEST_CASE("FileService: getSize") {
    auto tmpPath = std::filesystem::temp_directory_path() / "fileservice_size.txt";
    {
        FileService file(tmpPath);
        file.writeToFile("12345");
        REQUIRE(file.getSize() == 5);
        file.deleteFile();
    }
}

TEST_CASE("FileService: exists returns false for non-existent") {
    auto tmpPath = std::filesystem::temp_directory_path() / "nonexistent_file_xyz.txt";
    FileService file(tmpPath);
    REQUIRE_FALSE(file.exists());
}

TEST_CASE("FileService: getPath returns correct path") {
    auto tmpPath = std::filesystem::temp_directory_path() / "fileservice_path.txt";
    FileService file(tmpPath);
    REQUIRE(file.getPath() == tmpPath);
}

TEST_CASE("Cryption: encodeBase64") {
    std::string encoded = Cryption::encodeBase64("hello");
    REQUIRE_FALSE(encoded.empty());
}

TEST_CASE("Cryption: decodeBase64") {
    std::string decoded = Cryption::decodeBase64("aGVsbG8=");
    REQUIRE(decoded == "hello");
}

TEST_CASE("Cryption: base64 roundtrip") {
    std::string original = "test data 123!@#";
    std::string encoded = Cryption::encodeBase64(original);
    std::string decoded = Cryption::decodeBase64(encoded);
    REQUIRE(decoded == original);
}

TEST_CASE("Cryption: base64 roundtrip empty string") {
    std::string original = "";
    std::string encoded = Cryption::encodeBase64(original);
    std::string decoded = Cryption::decodeBase64(encoded);
    REQUIRE(decoded == original);
}

TEST_CASE("Cryption: hashOf produces salted PBKDF2 hash") {
    std::string hash = Cryption::hashOf("test");
    REQUIRE(Cryption::isModernHash(hash));
    REQUIRE(Cryption::verifyOf("test", hash));
    REQUIRE_FALSE(Cryption::verifyOf("wrong", hash));
}

TEST_CASE("Cryption: hashOf is salted (not deterministic)") {
    std::string h1 = Cryption::hashOf("hello");
    std::string h2 = Cryption::hashOf("hello");
    REQUIRE(h1 != h2);
    REQUIRE(Cryption::verifyOf("hello", h1));
    REQUIRE(Cryption::verifyOf("hello", h2));
}

TEST_CASE("Cryption: hashOf different inputs produce different hashes") {
    std::string h1 = Cryption::hashOf("abc");
    std::string h2 = Cryption::hashOf("xyz");
    REQUIRE(h1 != h2);
    REQUIRE(Cryption::verifyOf("abc", h1));
    REQUIRE(Cryption::verifyOf("xyz", h2));
}

TEST_CASE("Cryption: verifyOf supports legacy SHA-512 hash") {
    std::string legacy = Cryption::hashOfSha512("test");
    REQUIRE(legacy.size() == 128);
    REQUIRE_FALSE(Cryption::isModernHash(legacy));
    REQUIRE(Cryption::verifyOf("test", legacy));
    REQUIRE_FALSE(Cryption::verifyOf("wrong", legacy));
}
