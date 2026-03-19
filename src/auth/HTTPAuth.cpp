#include "auth/HTTPAuth.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include <openssl/hmac.h>
#include <openssl/sha.h>


std::map<std::string, std::string> HTTPAuth::buildHTTPAuthHeaders(
    const AppConfig& config
)
{
    const auto now = std::chrono::system_clock::now();
    auto timestamp = std::to_string(
        std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()
        ).count()
    );

    const std::string hash = generateHTTPAuthHash(
        config.secrets.privateSerialNumber,
        timestamp, config.secrets.frameAuthSecretKey
    );

    return {
        {"Authorization", "Auth-Hash " + hash},
        {"X-Timestamp", timestamp},
        {"Content-Type", "application/json"}
    };
}

std::string HTTPAuth::generateHTTPAuthHash(
    const std::string& privateSerialNumber,
    const std::string& timestamp,
    const std::string& secretKey
)
{
    if (secretKey.empty())
        throw std::runtime_error("HttpAuth: frameAuthSecretKey is empty");
    if (privateSerialNumber.empty())
        throw std::runtime_error("HttpAuth: serialNumber is empty");
    if (timestamp.empty())
        throw std::runtime_error("HttpAuth: timestamp is empty");

    const std::string messageToEncrypt = privateSerialNumber + ":" + timestamp;

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;

    HMAC(
        EVP_sha256(),
        secretKey.data(),
        static_cast<int>(secretKey.size()),
        reinterpret_cast<const unsigned char*>(messageToEncrypt.data()),
        static_cast<int>(messageToEncrypt.size()),
        digest,
        &digestLen
    );

    std::ostringstream hex;
    hex << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digestLen; ++i)
        hex << std::setw(2) << static_cast<int>(digest[i]);

    return hex.str();
}
