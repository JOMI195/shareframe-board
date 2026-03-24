#include "auth/ServerSignedResponse.hpp"
#include "util/Base64.hpp"
#include <chrono>
#include <stdexcept>
#include <vector>

#include <nlohmann/json.hpp>
#include <openssl/evp.h>

std::string ServerSignedResponse::verify(
    const std::string& signedResponse,
    const AppConfig& config,
    const int maxAgeSecs)
{
    if (config.secrets.serverEd25519PublicKey.empty())
        throw std::runtime_error("ServerSignedResponse: serverEd25519PublicKey is empty");
    if (config.secrets.publicSerialNumber.empty())
        throw std::runtime_error("ServerSignedResponse: publicSerialNumber is empty");

    // 1. Base64url-decode the outer envelope
    auto decodedBytes = Base64::urlDecode(signedResponse);
    std::string decodedStr(decodedBytes.begin(), decodedBytes.end());

    // 2. Parse JSON envelope → extract "message" and "signature"
    auto envelope = nlohmann::json::parse(decodedStr);
    auto messageStr = envelope.at("message").get<std::string>();
    const auto signatureB64 = envelope.at("signature").get<std::string>();

    // 3. Decode the server's public key and the signature
    const auto pubKeyBytes = Base64::decode(config.secrets.serverEd25519PublicKey);
    if (pubKeyBytes.size() != 32)
        throw std::runtime_error("ServerSignedResponse: server public key must be 32 bytes");

    const auto sigBytes = Base64::decode(signatureB64);

    // 4. Verify Ed25519 signature
    EVP_PKEY* pkey = EVP_PKEY_new_raw_public_key(
        EVP_PKEY_ED25519, nullptr, pubKeyBytes.data(), pubKeyBytes.size());
    if (!pkey)
        throw std::runtime_error("ServerSignedResponse: failed to load server public key");

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx)
    {
        EVP_PKEY_free(pkey);
        throw std::runtime_error("ServerSignedResponse: failed to create MD context");
    }

    if (EVP_DigestVerifyInit(mdctx, nullptr, nullptr, nullptr, pkey) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("ServerSignedResponse: DigestVerifyInit failed");
    }

    const int result = EVP_DigestVerify(
        mdctx,
        sigBytes.data(), sigBytes.size(),
        reinterpret_cast<const unsigned char*>(messageStr.data()), messageStr.size());

    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(pkey);

    if (result != 1)
        throw std::runtime_error("ServerSignedResponse: invalid signature");

    // 5. Parse the signed message
    auto message = nlohmann::json::parse(messageStr);

    // 6. Verify frame_id matches our own
    if (const auto frameId = message.at("frame_id").get<std::string>(); frameId != config.secrets.publicSerialNumber)
        throw std::runtime_error("ServerSignedResponse: frame_id mismatch");

    // 7. Check timestamp freshness
    const int64_t timestamp = message.at("timestamp").get<int64_t>();
    const auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    if (now - timestamp > maxAgeSecs)
        throw std::runtime_error("ServerSignedResponse: response expired");

    // 8. Return the "data" field as a JSON string
    return message.at("data").dump();
}
