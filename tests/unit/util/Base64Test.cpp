#include "util/Base64.hpp"
#include <gtest/gtest.h>

namespace
{
std::string str(const std::vector<unsigned char>& v) { return {v.begin(), v.end()}; }
std::string enc(const std::string& s)
{
    return Base64::encode(reinterpret_cast<const unsigned char*>(s.data()), s.size());
}
}

TEST(Base64, KnownVectors)
{
    EXPECT_EQ(enc(""), "");
    EXPECT_EQ(enc("f"), "Zg==");
    EXPECT_EQ(enc("fo"), "Zm8=");
    EXPECT_EQ(enc("foo"), "Zm9v");
    EXPECT_EQ(str(Base64::decode("Zm9vYmFy")), "foobar");
}

TEST(Base64, RoundTripBinary)
{
    std::vector<unsigned char> data(256);
    for (int i = 0; i < 256; ++i)
        data[i] = static_cast<unsigned char>(i);
    EXPECT_EQ(Base64::decode(Base64::encode(data.data(), data.size())), data);
}

TEST(Base64, UrlDecodeHandlesUrlAlphabetAndMissingPadding)
{
    EXPECT_EQ(Base64::urlDecode("-_8"), Base64::decode("+/8="));
    EXPECT_EQ(str(Base64::urlDecode("Zg")), "f");
}
