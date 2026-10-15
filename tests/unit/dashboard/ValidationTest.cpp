#include "dashboard/Validation.hpp"
#include <gtest/gtest.h>

using namespace dashboard::Validation;

TEST(Validation, SsidLengthAndControlChars)
{
    EXPECT_TRUE(isValidSsid("Home WiFi"));
    EXPECT_TRUE(isValidSsid(std::string(MAX_SSID_LENGTH, 'a')));
    EXPECT_FALSE(isValidSsid(""));
    EXPECT_FALSE(isValidSsid(std::string(MAX_SSID_LENGTH + 1, 'a')));
    EXPECT_FALSE(isValidSsid("bad\nssid"));
}

TEST(Validation, PasswordBounds)
{
    EXPECT_FALSE(isValidPassword(std::string(MIN_PASSWORD_LENGTH - 1, 'a')));
    EXPECT_TRUE(isValidPassword(std::string(MIN_PASSWORD_LENGTH, 'a')));
    EXPECT_TRUE(isValidPassword(std::string(MAX_PASSWORD_LENGTH, 'a')));
    EXPECT_FALSE(isValidPassword(std::string(MAX_PASSWORD_LENGTH + 1, 'a')));
    EXPECT_FALSE(isValidPassword("password\t1"));
}

TEST(Validation, WpaPassphraseRejectsNonPrintableAscii)
{
    EXPECT_TRUE(isValidWpaPassphrase("correct horse ~"));
    EXPECT_FALSE(isValidWpaPassphrase("pässwört12"));
    EXPECT_FALSE(isValidWpaPassphrase(std::string("abcdefgh") + '\x7f'));
}

TEST(Validation, ClampLogLines)
{
    EXPECT_EQ(clampLogLines(0), DEFAULT_LOG_LINES);
    EXPECT_EQ(clampLogLines(-5), DEFAULT_LOG_LINES);
    EXPECT_EQ(clampLogLines(42), 42);
    EXPECT_EQ(clampLogLines(MAX_LOG_LINES + 1), MAX_LOG_LINES);
}

TEST(Validation, IntervalAndHourBounds)
{
    EXPECT_FALSE(isValidIntervalSecs(MIN_INTERVAL_SECS - 1));
    EXPECT_TRUE(isValidIntervalSecs(MIN_INTERVAL_SECS));
    EXPECT_TRUE(isValidIntervalSecs(MAX_INTERVAL_SECS));
    EXPECT_FALSE(isValidIntervalSecs(MAX_INTERVAL_SECS + 1));
    EXPECT_TRUE(isValidHour(0));
    EXPECT_TRUE(isValidHour(23));
    EXPECT_FALSE(isValidHour(24));
    EXPECT_FALSE(isValidHour(-1));
}
