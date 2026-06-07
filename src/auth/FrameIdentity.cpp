#include "auth/FrameIdentity.hpp"
#include "util/Base64.hpp"

#include <array>
#include <stdexcept>
#include <vector>

#include <openssl/evp.h>

namespace
{

std::vector<unsigned char> publicKeyFromSeed(const std::string& base64PrivateKey)
{
    const auto seed = Base64::decode(base64PrivateKey);
    if (seed.size() != 32)
        throw std::runtime_error("FrameIdentity: private key seed must be 32 bytes");

    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(
        EVP_PKEY_ED25519, nullptr, seed.data(), seed.size());
    if (!pkey)
        throw std::runtime_error("FrameIdentity: failed to load private key");

    std::vector<unsigned char> pub(32);
    size_t pubLen = pub.size();
    if (EVP_PKEY_get_raw_public_key(pkey, pub.data(), &pubLen) != 1 || pubLen != 32)
    {
        EVP_PKEY_free(pkey);
        throw std::runtime_error("FrameIdentity: failed to derive public key");
    }
    EVP_PKEY_free(pkey);
    return pub;
}

std::array<unsigned char, 32> sha256(const std::vector<unsigned char>& data)
{
    std::array<unsigned char, 32> out{};
    unsigned int outLen = 0;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx)
        throw std::runtime_error("FrameIdentity: failed to create MD context");
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1
        || EVP_DigestUpdate(ctx, data.data(), data.size()) != 1
        || EVP_DigestFinal_ex(ctx, out.data(), &outLen) != 1 || outLen != 32)
    {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("FrameIdentity: SHA-256 failed");
    }
    EVP_MD_CTX_free(ctx);
    return out;
}

// RFC 4648 base32 (uppercase, no padding).
std::string base32(const unsigned char* data, size_t len)
{
    static constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    std::string out;
    unsigned int buffer = 0;
    int bits = 0;
    for (size_t i = 0; i < len; ++i)
    {
        buffer = (buffer << 8) | data[i];
        bits += 8;
        while (bits >= 5)
        {
            bits -= 5;
            out += alphabet[(buffer >> bits) & 0x1F];
        }
        buffer &= (1u << bits) - 1;
    }
    if (bits > 0)
        out += alphabet[(buffer << (5 - bits)) & 0x1F];
    return out;
}

} // namespace

std::string FrameIdentity::fingerprint(const std::string& base64PrivateKey)
{
    const auto pub = publicKeyFromSeed(base64PrivateKey);
    const auto digest = sha256(pub);

    const std::string b32 = base32(digest.data(), 10); // first 10 bytes -> 16 chars

    std::string grouped;
    for (size_t i = 0; i < b32.size(); i += 4)
    {
        if (i != 0)
            grouped += '-';
        grouped += b32.substr(i, 4);
    }
    return grouped;
}
