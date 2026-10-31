#include "display/DisplayIpc.hpp"
#include "ipc/EventBridge.hpp"
#include "ipc/EventPublisher.hpp"
#include "ipc/EventSubscriber.hpp"
#include "repository/DisplayMetricsRepository.hpp"
#include "support/ImageFixture.hpp"
#include "support/Wait.hpp"
#include <gtest/gtest.h>

using namespace std::chrono_literals;

// Websocket bus → EventBridge → nng PUB/SUB → display service → display bus.
namespace
{
class DisplayEventsTest : public ::testing::Test, protected ImageFixture
{
protected:
    DisplayEventsTest()
    {
        cfg.display.mockDisplay = true;
        bridge.wire();
        wsBus.start();
        bus.start();
        publisher.start();
        subscriber.start();
    }

    ~DisplayEventsTest() override
    {
        subscriber.stop();
        bus.stop();
        wsBus.stop();
    }

    /// PUB drops messages until the subscriber has joined, so resend until the first lands.
    template <typename Pred, typename Publish>
    bool deliver(Publish&& publish, Pred&& arrived)
    {
        return waitFor([&]
        {
            publish();
            return arrived();
        }, 5000ms);
    }

    std::string url = "ipc://" + (db.dir() / "pub.sock").string();
    EventBus wsBus;
    EventPublisher publisher{url};
    EventBridge bridge{wsBus, publisher};

    SettingsRepository settingsRepo{db.get()};
    RuntimeSettings settings{settingsRepo, cfg};
    DisplayMetricsRepository metrics{db.get()};
    DisplayManager display{cfg, metrics};
    DisplayImageLoop loop{bus, cfg, repo, display, settings};
    DisplayIpc displayIpc{bus, settings, loop, repo, display};

    Recorder<Topic::IMAGE_NEW> newImages{bus};
    Recorder<Topic::IMAGE_REMOVED> removed{bus};
    Recorder<Topic::CLEAR_DISPLAY> clears{bus};
    Recorder<Topic::FRAME_REGISTRATION> registration{bus};
    EventSubscriber subscriber{url, [this](const std::string& t, const nlohmann::json& p) { displayIpc.onEvent(t, p); }};
};
}

TEST_F(DisplayEventsTest, NewImageIdArrivesExact)
{
    constexpr int64_t big = 5'000'000'000;
    ASSERT_TRUE(deliver([&] { wsBus.publish<Topic::IMAGE_NEW>(big); }, [&] { return newImages.size() > 0; }));
    EXPECT_EQ(newImages.get()[0], big);
}

TEST_F(DisplayEventsTest, RemovedIdsArrive)
{
    ASSERT_TRUE(deliver([&] { wsBus.publish<Topic::IMAGE_REMOVED>({3, 4}); }, [&] { return removed.size() > 0; }));
    EXPECT_EQ(removed.get()[0], (std::vector<int64_t>{3, 4}));
}

TEST_F(DisplayEventsTest, ServerClearReachesTheDisplay)
{
    ASSERT_TRUE(deliver([&] { wsBus.publish<Topic::CLEAR_DISPLAY>({{"type", "clear_display"}}); },
                        [&] { return clears.size() > 0; }));
}

TEST_F(DisplayEventsTest, ConnectEventsAreNotForwardedToTheDisplay)
{
    ASSERT_TRUE(deliver([&] { wsBus.publish<Topic::WS_CONNECTED>({}); wsBus.publish<Topic::IMAGE_NEW>(1); },
                        [&] { return newImages.size() > 0; }));
    EXPECT_EQ(removed.size() + clears.size(), 0u);
}

TEST_F(DisplayEventsTest, RefusedRegistrationReachesTheDisplay)
{
    ASSERT_TRUE(deliver([&] { wsBus.publish<Topic::FRAME_REGISTRATION>({false}); },
                        [&] { return registration.size() > 0; }));
    EXPECT_FALSE(registration.get()[0].registered);

    ASSERT_TRUE(deliver([&] { wsBus.publish<Topic::FRAME_REGISTRATION>({true}); },
                        [&] { return registration.get().back().registered; }));
}
