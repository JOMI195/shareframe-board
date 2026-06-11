#pragma once
#include <string>

#include "util/NonInstantiable.hpp"

// PBKDF2-HMAC-SHA256 password hashing for the dashboard login.
// Stored format: pbkdf2-sha256$<iterations>$<salt_b64>$<hash_b64>
class PasswordHash : NonInstantiable
{
public:
    static std::string hash(const std::string& password);
    static bool verify(const std::string& password, const std::string& stored);
    static bool isWellFormed(const std::string& stored);

    // Constant-time comparison for the plaintext initial password from secrets.
    static bool constantTimeEquals(const std::string& a, const std::string& b);
};
