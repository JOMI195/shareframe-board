#include "net/WebsocketClient.hpp"
#include "support/FakeWsServer.hpp"
#include "support/ImageFixture.hpp"
#include "support/TokenStore.hpp"
#include "support/Wait.hpp"
#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
class WebsocketClientTest : public ::testing::Test, protected ImageFixture
{
protected:
    WebsocketClientTest()
    {
        cfg.baseUrl = server.baseUrl();
        storeValidToken(db.get());
    }

    ~WebsocketClientTest() override
    {
        client.stop();
        bus.stop();
    }

    void connect()
    {
        client.start();
        bus.start();
        ASSERT_TRUE(waitFor([&] { return server.clientCount() == 1; }, 5000ms));
    }

    FakeWsServer server;
    TokenRepository tokens{db.get()};
    HTTPClient http{5, 5};
    AuthTokenManager auth{cfg, tokens, http};
    Recorder<Topic::WS_CONNECTED> connected{bus};
    Recorder<Topic::PICTURE> pictures{bus};
    Recorder<Topic::CLEAR_IMAGES> clearImages{bus};
    Recorder<Topic::CLEAR_DISPLAY> clearDisplay{bus};
    WebsocketClient client{bus, cfg, auth};
};
}

TEST_F(WebsocketClientTest, ConnectsWithTheFrameTokenAndAnnouncesIt)
{
    ASSERT_TRUE(server.start());
    connect();
    EXPECT_EQ(server.openHeaders().at(0).at("Authorization"), "Frame-Access-Token test-token");
    EXPECT_TRUE(waitFor([&] { return connected.size() == 1; }));
}

TEST_F(WebsocketClientTest, ServerMessagesAreRoutedByType)
{
    ASSERT_TRUE(server.start());
    connect();

    server.sendToAll("not json");
    server.sendToAll(R"({"no_type":1})");
    server.sendToAll(R"({"type":"something_new"})");
    server.sendToAll(R"({"type":"picture","sent_image_id":1})");
    server.sendToAll(R"({"type":"clear_specific_sent_images","sent_image_ids":[1]})");
    server.sendToAll(R"({"type":"clear_display"})");

    ASSERT_TRUE(waitFor([&] { return clearDisplay.size() == 1; }));
    ASSERT_EQ(pictures.size(), 1u);
    EXPECT_EQ(pictures.get()[0]["sent_image_id"], 1);
    ASSERT_EQ(clearImages.size(), 1u);
    EXPECT_EQ(clearImages.get()[0]["sent_image_ids"], nlohmann::json::array({1}));
}

TEST_F(WebsocketClientTest, MessagesSentWhileOfflineAreDeliveredOnConnect)
{
    client.start();
    bus.start();
    bus.publish<Topic::WS_SEND>({{"type", "check_missing_images"}, {"sent_image_ids", {7}}});
    std::this_thread::sleep_for(300ms);

    ASSERT_TRUE(server.start());
    ASSERT_TRUE(waitFor([&] { return !server.received().empty(); }, 10000ms));
    EXPECT_EQ(nlohmann::json::parse(server.received()[0])["sent_image_ids"], nlohmann::json::array({7}));
}
