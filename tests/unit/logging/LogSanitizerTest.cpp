#include "logging/LogSanitizer.hpp"
#include <gtest/gtest.h>

TEST(LogSanitizer, ShortPayloadPassesThrough)
{
    EXPECT_EQ(logging::summarizePayloadForLog("hello"), "hello");
}

TEST(LogSanitizer, LongPayloadIsSummarized)
{
    const std::string payload(2000, 'x');
    EXPECT_EQ(logging::summarizePayloadForLog(payload, "image"), "<image truncated: 2000 bytes>");
    EXPECT_EQ(logging::summarizePayloadForLog(payload, ""), "<truncated: 2000 bytes>");
}

TEST(LogSanitizer, BoundaryIsInclusive)
{
    EXPECT_EQ(logging::summarizePayloadForLog("abcd", "p", 4), "abcd");
    EXPECT_EQ(logging::summarizePayloadForLog("abcde", "p", 4), "<p truncated: 5 bytes>");
}
