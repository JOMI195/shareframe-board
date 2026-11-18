#include "auth/AuthTokenManager.hpp"
#include "ipc/EventPublisher.hpp"
#include "ipc/EventSubscriber.hpp"
#include "ipc/EventTopics.hpp"
#include "ipc/IpcClient.hpp"
#include "repository/ImageRepository.hpp"
#include "support/FakeHttpServer.hpp"
#include "support/PathShim.hpp"
#include "support/TestDb.hpp"
#include "support/TokenStore.hpp"
#include "support/Wait.hpp"
#include "task/Heartbeat.hpp"
#include <gtest/gtest.h>

using namespace std::chrono_literals;

// The websocket service announces a (re)connect; the heartbeat service answers with a heartbeat
// at once instead of after its interval.
TEST(WsHeartbeat, AReconnectOfTheWebsocketSendsAHeartbeatAtOnce)
{
    PathShim shims;
    shims.add("shareframe-sysinfo", "exit 0");
    shims.add("shareframe-event", "exit 0");
    FakeHttpServer backend;
    TestDb db;
    AppConfig cfg;
    cfg.baseUrl = backend.baseUrl();
    cfg.heartbeat.intervalSecs = 3600;
    cfg.heartbeat.firstContactRetrySecs = 3600;
    cfg.heartbeat.eventCursorPath = (db.dir() / "event-cursor").string();
    cfg.heartbeat.backendOkPath = (db.dir() / "backend-ok").string();
    storeValidToken(db.get());
    constexpr auto kHeartbeat = "/api/frames/frame-hearbeat/";
    backend.on(std::string("POST ") + kHeartbeat, 200);
    TokenRepository tokens{db.get()};
    ImageRepository images{db.get()};
    HTTPClient http{5, 5};
    AuthTokenManager auth{cfg, tokens, http};
    EventBus bus;
    auto sock = [&](const char* n) { return "ipc://" + (db.dir() / n).string(); };
    IpcClient wsIpc{sock("ws.sock")}, displayIpc{sock("display.sock")}, dashboardIpc{sock("dash.sock")},
        updateIpc{sock("update.sock")};
    Heartbeat heartbeat{bus, cfg, http, auth, wsIpc, displayIpc, dashboardIpc, updateIpc, images};

    const auto pubUrl = sock("ws.pub.sock");
    EventPublisher wsEvents(pubUrl);
    ASSERT_TRUE(wsEvents.start());
    EventSubscriber subscriber(pubUrl, [&](const std::string& topic, const nlohmann::json&) { heartbeat.onWsEvent(topic); });
    subscriber.start();
    heartbeat.start();
    ASSERT_TRUE(waitFor([&] { return backend.requests(kHeartbeat).size() == 1; }, 5000ms));

    EXPECT_TRUE(waitFor([&]
    {
        wsEvents.publish(std::string(event_topics::WsConnected), {});
        return backend.requests(kHeartbeat).size() >= 2;
    }, 5000ms));
    subscriber.stop();
    heartbeat.stop();
}
