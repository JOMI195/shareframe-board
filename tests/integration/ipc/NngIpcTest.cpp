#include "ipc/EventPublisher.hpp"
#include "ipc/EventSubscriber.hpp"
#include "ipc/HealthCheck.hpp"
#include "ipc/IpcClient.hpp"
#include "ipc/NngRepServer.hpp"
#include "support/TempDir.hpp"
#include "support/Wait.hpp"
#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
std::string endpoint(const TempDir& dir, const std::string& name) { return "ipc://" + (dir / name).string(); }
}

TEST(NngIpc, RequestReplyRoundTrip)
{
    TempDir dir;
    const auto url = endpoint(dir, "rep.sock");
    NngRepServer server(url, [](const IpcMessage& msg) -> nlohmann::json
    {
        if (msg.type == IpcMessageType::GetDisplayInterval)
            return {{"interval_secs", 900}};
        return {{"echo", msg.data}};
    });
    server.start();

    IpcClient client(url);
    const auto interval = client.sendAndReceive({IpcMessageType::GetDisplayInterval, {}});
    ASSERT_TRUE(interval);
    EXPECT_EQ((*interval)["interval_secs"], 900);

    const auto echo = client.sendAndReceive({IpcMessageType::UpdateDisplayInterval, {{"interval_secs", 600}}});
    ASSERT_TRUE(echo);
    EXPECT_EQ((*echo)["echo"]["interval_secs"], 600);
}

TEST(NngIpc, HealthCheckAgainstHealthResponder)
{
    TempDir dir;
    const auto url = endpoint(dir, "health.sock");
    NngRepServer server(url, health::respond);
    server.start();
    IpcClient client(url);
    EXPECT_TRUE(health::isRunning(client, 2000ms));
}

TEST(NngIpc, ThrowingHandlerStillAnswersWithEmptyObject)
{
    TempDir dir;
    const auto url = endpoint(dir, "throw.sock");
    NngRepServer server(url, [](const IpcMessage&) -> nlohmann::json { throw std::runtime_error("boom"); });
    server.start();
    IpcClient client(url);

    const auto resp = client.sendAndReceive({IpcMessageType::SkipImage, {}});
    ASSERT_TRUE(resp);
    EXPECT_TRUE(resp->is_object());
    EXPECT_TRUE(resp->empty());
    EXPECT_TRUE(client.sendAndReceive({IpcMessageType::SkipImage, {}}).has_value());
}

TEST(NngIpc, NoServerTimesOut)
{
    TempDir dir;
    IpcClient client(endpoint(dir, "nobody.sock"));
    const auto start = std::chrono::steady_clock::now();
    EXPECT_FALSE(client.sendAndReceive({IpcMessageType::GetHealth, {}}, 200ms).has_value());
    EXPECT_LT(std::chrono::steady_clock::now() - start, 2s);
    EXPECT_FALSE(health::isRunning(client, 100ms));
}

TEST(NngIpc, ClientConnectsToServerStartedLater)
{
    TempDir dir;
    const auto url = endpoint(dir, "late.sock");
    IpcClient client(url);
    EXPECT_FALSE(health::isRunning(client, 100ms));

    NngRepServer server(url, health::respond);
    server.start();
    EXPECT_TRUE(waitFor([&] { return health::isRunning(client, 200ms); }, 5000ms));
}

TEST(NngIpc, ServerRestartIsPickedUp)
{
    TempDir dir;
    const auto url = endpoint(dir, "restart.sock");
    IpcClient client(url);
    {
        NngRepServer first(url, health::respond);
        first.start();
        ASSERT_TRUE(health::isRunning(client, 2000ms));
    }
    NngRepServer second(url, health::respond);
    second.start();
    EXPECT_TRUE(waitFor([&] { return health::isRunning(client, 200ms); }, 5000ms));
}

TEST(NngIpc, PubSubDeliversTopicAndPayload)
{
    TempDir dir;
    const auto url = endpoint(dir, "pub.sock");
    EventPublisher pub(url);
    ASSERT_TRUE(pub.start());

    std::mutex m;
    std::vector<std::pair<std::string, nlohmann::json>> got;
    EventSubscriber sub(url, [&](const std::string& topic, const nlohmann::json& payload)
    {
        std::lock_guard lk(m);
        got.emplace_back(topic, payload);
    });
    sub.start();

    // PUB drops messages until the subscriber has joined, so publish until one lands.
    ASSERT_TRUE(waitFor([&]
    {
        pub.publish("image.new", {{"id", 42}});
        std::lock_guard lk(m);
        return !got.empty();
    }, 5000ms));

    std::lock_guard lk(m);
    EXPECT_EQ(got.front().first, "image.new");
    EXPECT_EQ(got.front().second["id"], 42);
}
