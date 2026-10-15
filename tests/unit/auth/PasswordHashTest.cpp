#include "auth/PasswordHash.hpp"
#include <gtest/gtest.h>

TEST(PasswordHash, HashVerifiesOnlyMatchingPassword)
{
    const auto stored = PasswordHash::hash("s3cret-pass");
    EXPECT_TRUE(PasswordHash::isWellFormed(stored));
    EXPECT_TRUE(PasswordHash::verify("s3cret-pass", stored));
    EXPECT_FALSE(PasswordHash::verify("s3cret-pasS", stored));
    EXPECT_FALSE(PasswordHash::verify("", stored));
}

TEST(PasswordHash, SaltMakesHashesDiffer)
{
    EXPECT_NE(PasswordHash::hash("same"), PasswordHash::hash("same"));
}

TEST(PasswordHash, MalformedStoredValuesNeverVerify)
{
    for (const std::string bad : {"", "plain", "pbkdf2-sha256$", "pbkdf2-sha256$abc$x$y",
                                  "md5$1000$c2FsdA==$aGFzaA==", "pbkdf2-sha256$0$c2FsdA==$aGFzaA=="})
    {
        EXPECT_FALSE(PasswordHash::isWellFormed(bad)) << bad;
        EXPECT_FALSE(PasswordHash::verify("x", bad)) << bad;
    }
}

TEST(PasswordHash, ConstantTimeEquals)
{
    EXPECT_TRUE(PasswordHash::constantTimeEquals("abc", "abc"));
    EXPECT_FALSE(PasswordHash::constantTimeEquals("abc", "abd"));
    EXPECT_FALSE(PasswordHash::constantTimeEquals("abc", "abcd"));
    EXPECT_TRUE(PasswordHash::constantTimeEquals("", ""));
}
