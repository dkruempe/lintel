#ifndef CPP_BASE_LIBRARY_CRYPTION_H
#define CPP_BASE_LIBRARY_CRYPTION_H

#include <cstdint>

#include <string>

/** Utility class providing cryptographic hash and Base64 encoding/decoding operations. */
class Cryption {
public:
    /** Default constructor. */
    Cryption();

    /**
     * Compute a salted, iterated hash (PBKDF2-HMAC-SHA512) of the given password.
     * The result embeds salt and iteration count, so it is safe for password
     * storage. Format: `$pbkdf2-sha512$<iterations>$<salt-hex>$<derived-hex>`.
     * @param password the password to hash
     * @return the salted password hash
     */
    static std::string hashOf(const std::string &password);

    /**
     * Verify a password against a stored hash. Supports the modern salted
     * PBKDF2 format as well as legacy plain SHA-512 hex hashes.
     * @param password the password to check
     * @param storedHash the stored hash
     * @return true if the password matches the stored hash
     */
    static bool verifyOf(const std::string &password,
                         const std::string &storedHash);

    /** @return true if the stored hash uses the modern salted format */
    static bool isModernHash(const std::string &storedHash);

    /**
     * Compute the legacy plain SHA-512 hex hash (kept for migration purposes).
     * @param text the input text
     * @return the SHA-512 hash as a hex string
     */
    static std::string hashOfSha512(const std::string &text);

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
