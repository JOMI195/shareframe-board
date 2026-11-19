#include "util/WpaText.hpp"
#include <gtest/gtest.h>

TEST(WpaText, PlainNamesStayAsTheyAre)
{
    EXPECT_EQ(WpaText::decode("home"), "home");
    EXPECT_EQ(WpaText::decode("my net 5"), "my net 5");
    EXPECT_EQ(WpaText::decode(""), "");
}

TEST(WpaText, EscapedBytesBecomeTheNameItself)
{
    EXPECT_EQ(WpaText::decode(R"(Caf\xc3\xa9 Wi-Fi)"), "Café Wi-Fi");
    EXPECT_EQ(WpaText::decode(R"(M\xc3\xbcller \xf0\x9f\x93\xb6)"), "Müller 📶");
    EXPECT_EQ(WpaText::decode(R"(quote\"d)"), "quote\"d");
    EXPECT_EQ(WpaText::decode(R"(back\\slash)"), "back\\slash");
    EXPECT_EQ(WpaText::decode(R"(\e\n\r\t)"), "\x1b\n\r\t");
}

TEST(WpaText, WhatIsNoEscapeIsKept)
{
    EXPECT_EQ(WpaText::decode(R"(end\)"), "end\\");
    EXPECT_EQ(WpaText::decode(R"(\xZZ)"), "\\xZZ");
    EXPECT_EQ(WpaText::decode(R"(\x4)"), "\\x4");
    EXPECT_EQ(WpaText::decode(R"(\q)"), "\\q");
}
