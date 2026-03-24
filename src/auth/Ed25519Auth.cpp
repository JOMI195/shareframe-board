#include "auth/Ed25519Auth.hpp"
#include "util/Base64.hpp"
#include <chrono>
#include <stdexcept>
#include <vector>

#include <openssl/evp.h>

std::string Ed25519Auth::sign(const std::string& message, const std::string& base64PrivateKey)
{
    const auto seed = Base64::decode(base64PrivateKey);
    if (seed.size() != 32)
        throw std::runtime_error("Ed25519Auth: private key seed must be 32 bytes");

    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(
        EVP_PKEY_ED25519, nullptr, seed.data(), seed.size());
    if (!pkey)
        throw std::runtime_error("Ed25519Auth: failed to load private key");

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx)
    {
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Ed25519Auth: failed to create MD context");
    }

    if (EVP_DigestSignInit(mdctx, nullptr, nullptr, nullptr, pkey) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Ed25519Auth: DigestSignInit failed");
    }

    // Determine signature length
    size_t sigLen = 0;
    if (EVP_DigestSign(mdctx, nullptr, &sigLen,
                       reinterpret_cast<const unsigned char*>(message.data()), message.size()) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Ed25519Auth: DigestSign length query failed");
    }

    std::vector<unsigned char> sig(sigLen);
    if (EVP_DigestSign(mdctx, sig.data(), &sigLen,
                       reinterpret_cast<const unsigned char*>(message.data()), message.size()) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Ed25519Auth: DigestSign failed");
    }

    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(pkey);

    return Base64::encode(sig.data(), sigLen);
}

std::map<std::string, std::string> Ed25519Auth::buildHTTPAuthHeaders(const AppConfig& config)
{
    if (config.secrets.ed25519PrivateKey.empty())
        throw std::runtime_error("Ed25519Auth: ed25519PrivateKey is empty");
    if (config.secrets.publicSerialNumber.empty())
        throw std::runtime_error("Ed25519Auth: publicSerialNumber is empty");

    const auto now = std::chrono::system_clock::now();
    auto timestamp = std::to_string(
        std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()
        ).count()
    );

    const std::string message = config.secrets.publicSerialNumber + ":" + timestamp;
    const std::string signature = sign(message, config.secrets.ed25519PrivateKey);

    return {
        {"Authorization", "Ed25519-Sig " + signature},
        {"X-Frame-ID", config.secrets.publicSerialNumber},
        {"X-Timestamp", timestamp},
        {"Content-Type", "application/json"}
    };
}
