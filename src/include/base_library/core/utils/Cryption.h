#ifndef CPP_BASE_LIBRARY_CRYPTION_H
#define CPP_BASE_LIBRARY_CRYPTION_H

#include <openssl/aes.h>

#include <array>
#include <string>

/** Utility class providing cryptographic hash and Base64 encoding/decoding operations. */
class Cryption {
public:
    /** Default constructor. */
    Cryption();

    /** Compute a hash of the given text.
     * @param text the input text
     * @return the hash as a hex string */
    static std::string hashOf(const std::string &text);

    /** Encode a string to Base64.
     * @param in the input string
     * @return the Base64-encoded string */
    static std::string encodeBase64(const std::string &in);

    /** Decode a Base64-encoded string.
     * @param in the Base64-encoded input
     * @return the decoded string */
    static std::string decodeBase64(const std::string &in);
};

#endif  // CPP_BASE_LIBRARY_CRYPTION_H
