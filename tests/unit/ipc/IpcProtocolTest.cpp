#include "ipc/IpcProtocol.hpp"
#include <gtest/gtest.h>

TEST(IpcProtocol, EveryTypeRoundTripsThroughString)
{
    for (int i = 0; i <= static_cast<int>(IpcMessageType::HideStatusScreen); ++i)
    {
        const auto t = static_cast<IpcMessageType>(i);
        EXPECT_EQ(ipcMessageTypeFromString(ipcMessageTypeToString(t)), t) << i;
    }
}

TEST(IpcProtocol, MessageRoundTripKeepsData)
{
    const IpcMessage msg{IpcMessageType::UpdateNightMode, {{"enabled", true}, {"start_hour", 22}}};
    const auto parsed = parseIpcMessage(toJson(msg));
    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed->type, IpcMessageType::UpdateNightMode);
    EXPECT_EQ(parsed->data["start_hour"], 22);
}

TEST(IpcProtocol, MessageWithoutDataOmitsField)
{
    EXPECT_EQ(toJson({IpcMessageType::SkipImage, {}}), "{\"type\":\"skip_image\"}\n");
    const auto parsed = parseIpcMessage("{\"type\":\"skip_image\"}");
    ASSERT_TRUE(parsed);
    EXPECT_TRUE(parsed->data.is_null());
}

TEST(IpcProtocol, MalformedInputIsRejectedWithoutThrowing)
{
    for (const std::string bad : {"", "not json", "{}", "{\"type\":42}", "{\"type\":\"nope\"}", "[1,2]", "null"})
        EXPECT_FALSE(parseIpcMessage(bad).has_value()) << bad;
}
