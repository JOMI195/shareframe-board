#include "auth/PasswordHash.hpp"
#include "util/Base64.hpp"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <charconv>
#include <stdexcept>
#include <vector>

namespace
{
constexpr const char* kAlgo = "pbkdf2-sha256";
constexpr int kIterations = 100000;
constexpr size_t kSaltLen = 16;
constexpr size_t kHashLen = 32;

struct Parsed
{
    int iterations = 0;
    std::vector<unsigned char> salt;
    std::vector<unsigned char> hash;
};

// Returns false on any structural problem; never throws.
bool parse(const std::string& stored, Parsed& out)
{
    const auto d1 = stored.find('$');
    if (d1 == std::string::npos || stored.compare(0, d1, kAlgo) != 0)
        return false;
    const auto d2 = stored.find('$', d1 + 1);
    const auto d3 = stored.find('$', d2 + 1);
    if (d2 == std::string::npos || d3 == std::string::npos)
        return false;

    const std::string_view iters(stored.data() + d1 + 1, d2 - d1 - 1);
    auto [ptr, ec] = std::from_chars(iters.data(), iters.data() + iters.size(), out.iterations);
    if (ec != std::errc{} || ptr != iters.data() + iters.size())
        return false;
    // Bound so a corrupted row can neither skip work nor stall the board.
    if (out.iterations < 1000 || out.iterations > 10000000)
        return false;

    try
    {
        out.salt = Base64::decode(stored.substr(d2 + 1, d3 - d2 - 1));
        out.hash = Base64::decode(stored.substr(d3 + 1));
    }
    catch (...)
    {
        return false;
    }
    return !out.salt.empty() && !out.hash.empty();
}

std::vector<unsigned char> derive(const std::string& password,
                                  const std::vector<unsigned char>& salt,
                                  int iterations, size_t outLen)
{
    std::vector<unsigned char> out(outLen);
    if (PKCS5_PBKDF2_HMAC(password.data(), static_cast<int>(password.size()),
                          salt.data(), static_cast<int>(salt.size()),
                          iterations, EVP_sha256(),
                          static_cast<int>(out.size()), out.data()) != 1)
        throw std::runtime_error("PBKDF2 derivation failed");
    return out;
}
} // namespace

std::string PasswordHash::hash(const std::string& password)
{
    std::array<unsigned char, kSaltLen> saltBuf{};
    if (RAND_bytes(saltBuf.data(), saltBuf.size()) != 1)
        throw std::runtime_error("RAND_bytes failed");

    const std::vector<unsigned char> salt(saltBuf.begin(), saltBuf.end());
    const auto digest = derive(password, salt, kIterations, kHashLen);

    return std::string(kAlgo) + "$" + std::to_string(kIterations)
        + "$" + Base64::encode(salt.data(), salt.size())
        + "$" + Base64::encode(digest.data(), digest.size());
}

bool PasswordHash::verify(const std::string& password, const std::string& stored)
{
    Parsed p;
    if (!parse(stored, p))
        return false;

    const auto digest = derive(password, p.salt, p.iterations, p.hash.size());
    return CRYPTO_memcmp(digest.data(), p.hash.data(), p.hash.size()) == 0;
}

bool PasswordHash::isWellFormed(const std::string& stored)
{
    Parsed p;
    return parse(stored, p);
}

bool PasswordHash::constantTimeEquals(const std::string& a, const std::string& b)
{
    if (a.size() != b.size())
        return false;
    if (a.empty())
        return true;
    return CRYPTO_memcmp(a.data(), b.data(), a.size()) == 0;
}
