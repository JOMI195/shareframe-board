#include "util/FileHash.hpp"
#include <array>
#include <fstream>
#include <openssl/evp.h>

namespace
{
std::string toHex(const unsigned char* digest, const size_t size)
{
    static constexpr char hex[] = "0123456789abcdef";
    std::string out;
    out.reserve(size * 2);
    for (size_t i = 0; i < size; ++i)
    {
        out.push_back(hex[digest[i] >> 4]);
        out.push_back(hex[digest[i] & 0xf]);
    }
    return out;
}
} // namespace

std::string FileHash::sha256File(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return "";

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx)
        return "";
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1)
    {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    std::array<char, 64 * 1024> buf{};
    while (f.read(buf.data(), buf.size()) || f.gcount() > 0)
    {
        if (EVP_DigestUpdate(ctx, buf.data(), static_cast<size_t>(f.gcount())) != 1)
        {
            EVP_MD_CTX_free(ctx);
            return "";
        }
    }

    std::array<unsigned char, 32> digest{};
    unsigned int len = 0;
    const bool ok = EVP_DigestFinal_ex(ctx, digest.data(), &len) == 1 && len == digest.size();
    EVP_MD_CTX_free(ctx);
    if (!ok)
        return "";

    return toHex(digest.data(), digest.size());
}

std::string FileHash::sha256(const std::string_view bytes)
{
    std::array<unsigned char, 32> digest{};
    unsigned int len = 0;
    if (EVP_Digest(bytes.data(), bytes.size(), digest.data(), &len, EVP_sha256(), nullptr) != 1 || len != digest.size())
        return "";
    return toHex(digest.data(), digest.size());
}
