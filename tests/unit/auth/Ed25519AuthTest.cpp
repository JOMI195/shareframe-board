#include "auth/Ed25519Auth.hpp"
#include "auth/FrameIdentity.hpp"
#include "util/Base64.hpp"
#include <gtest/gtest.h>
#include <openssl/evp.h>
#include <regex>

namespace
{
const std::string kSeed = Base64::encode(
    reinterpret_cast<const unsigned char*>("0123456789abcdef0123456789abcdef"), 32);

bool verifySignature(const std::string& seedB64, const std::string& message, const std::string& sigB64)
{
    const auto seed = Base64::decode(seedB64);
    EVP_PKEY* priv = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, seed.data(), seed.size());
    unsigned char pub[32];
    size_t pubLen = sizeof(pub);
    EVP_PKEY_get_raw_public_key(priv, pub, &pubLen);
    EVP_PKEY_free(priv);

    EVP_PKEY* key = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr, pub, pubLen);
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    const auto sig = Base64::decode(sigB64);
    const bool ok = EVP_DigestVerifyInit(ctx, nullptr, nullptr, nullptr, key) == 1
        && EVP_DigestVerify(ctx, sig.data(), sig.size(),
                            reinterpret_cast<const unsigned char*>(message.data()), message.size()) == 1;
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(key);
    return ok;
}
}

TEST(FrameIdentity, FingerprintIsStableAndGrouped)
{
    const auto id = FrameIdentity::fingerprint(kSeed);
    EXPECT_EQ(id, FrameIdentity::fingerprint(kSeed));
    EXPECT_TRUE(std::regex_match(id, std::regex("[A-Z2-7]{4}-[A-Z2-7]{4}-[A-Z2-7]{4}-[A-Z2-7]{4}"))) << id;
}

TEST(FrameIdentity, RejectsWrongSeedLength)
{
    EXPECT_THROW(FrameIdentity::fingerprint(Base64::encode(reinterpret_cast<const unsigned char*>("short"), 5)),
                 std::runtime_error);
}

TEST(Ed25519Auth, HeadersCarryVerifiableSignatureOverFrameIdAndTimestamp)
{
    AppConfig cfg;
    cfg.secrets.ed25519PrivateKey = kSeed;
    cfg.frameId = FrameIdentity::fingerprint(kSeed);

    const auto headers = Ed25519Auth::buildHTTPAuthHeaders(cfg);
    ASSERT_TRUE(headers.contains("Authorization"));
    EXPECT_EQ(headers.at("X-Frame-ID"), cfg.frameId);

    const auto& auth = headers.at("Authorization");
    ASSERT_TRUE(auth.starts_with("Ed25519-Sig "));
    const auto message = cfg.frameId + ":" + headers.at("X-Timestamp");
    EXPECT_TRUE(verifySignature(kSeed, message, auth.substr(12)));
    EXPECT_FALSE(verifySignature(kSeed, message + "x", auth.substr(12)));
}

TEST(Ed25519Auth, ThrowsWithoutIdentity)
{
    AppConfig cfg;
    EXPECT_THROW(Ed25519Auth::buildHTTPAuthHeaders(cfg), std::runtime_error);
    cfg.secrets.ed25519PrivateKey = kSeed;
    EXPECT_THROW(Ed25519Auth::buildHTTPAuthHeaders(cfg), std::runtime_error);
}
