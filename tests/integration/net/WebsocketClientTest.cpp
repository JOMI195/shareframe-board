#include "net/WebsocketClient.hpp"
#include "support/FakeWsServer.hpp"
#include "support/ImageFixture.hpp"
#include "support/TokenStore.hpp"
#include "support/Wait.hpp"
#include <arpa/inet.h>
#include <atomic>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <optional>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using namespace std::chrono_literals;

namespace
{
/// Accepts every connection and hangs up at once, so each reconnect attempt fails and is counted.
class HangUpListener
{
public:
    explicit HangUpListener(const int port)
    {
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        constexpr int on = 1;
        ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(static_cast<uint16_t>(port));
        if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0 || ::listen(fd_, 8) != 0)
            throw std::runtime_error("HangUpListener: cannot listen");
        thread_ = std::thread([this]
        {
            while (!stop_)
            {
                pollfd p{fd_, POLLIN, 0};
                if (::poll(&p, 1, 20) <= 0)
                    continue;
                if (const int conn = ::accept(fd_, nullptr, nullptr); conn >= 0)
                {
                    ::close(conn);
                    ++accepted_;
                }
            }
        });
    }

    ~HangUpListener()
    {
        stop_ = true;
        thread_.join();
        ::close(fd_);
    }

    HangUpListener(const HangUpListener&) = delete;
    HangUpListener& operator=(const HangUpListener&) = delete;

    [[nodiscard]] int accepted() const { return accepted_.load(); }

private:
    int fd_ = -1;
    std::atomic<bool> stop_{false};
    std::atomic<int> accepted_{0};
    std::thread thread_;
};

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
    Recorder<Topic::REMOTE_COMMAND> commands{bus};
    SettingsRepository settingsRepo{db.get()};
    ServerContact contact{settingsRepo};
    WebsocketClient client{bus, cfg, auth, contact};
};
}

TEST_F(WebsocketClientTest, ConnectsWithTheFrameTokenAndAnnouncesIt)
{
    ASSERT_TRUE(server.start());
    connect();
    EXPECT_EQ(server.openHeaders().at(0).at("Authorization"), "Frame-Access-Token test-token");
    EXPECT_TRUE(waitFor([&] { return connected.size() == 1; }));
    EXPECT_TRUE(contact.snapshot().connected);
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
    server.sendToAll(R"({"type":"command_result","id":1})");
    server.sendToAll(R"({"type":"command","id":5,"command":"reboot"})");
    server.sendToAll(R"({"type":"clear_display"})");

    ASSERT_TRUE(waitFor([&] { return clearDisplay.size() == 1; }));
    ASSERT_EQ(commands.size(), 1u);
    EXPECT_EQ(commands.get()[0]["id"], 5);
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

TEST_F(WebsocketClientTest, AnOutageKeepsTheTokenAndReconnectNowSkipsThePendingWait)
{
    std::optional<HangUpListener> down(std::in_place, server.port());
    client.start();
    bus.start();
    // After six failures the next attempt is 3.2s away.
    ASSERT_TRUE(waitFor([&] { return down->accepted() >= 6; }, 15000ms));
    ASSERT_TRUE(tokens.get().has_value()) << "an unreachable server must not wipe the token";
    EXPECT_EQ(tokens.get()->value, "test-token");

    down.reset();
    ASSERT_TRUE(server.start());
    client.reconnectNow();
    EXPECT_TRUE(waitFor([&] { return server.clientCount() == 1; }, 1500ms));
    EXPECT_EQ(server.openHeaders().at(0).at("Authorization"), "Frame-Access-Token test-token");
}

TEST_F(WebsocketClientTest, ReconnectNowLeavesAnOpenSocketAlone)
{
    ASSERT_TRUE(server.start());
    connect();

    client.reconnectNow();
    std::this_thread::sleep_for(300ms);
    EXPECT_EQ(server.openHeaders().size(), 1u);
}
