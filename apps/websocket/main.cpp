#include "app/Bootstrap.hpp"
#include "auth/AuthTokenManager.hpp"
#include "db/Database.hpp"
#include "events/EventBus.hpp"
#include "image/ImageManager.hpp"
#include "ipc/EventBridge.hpp"
#include "ipc/EventPublisher.hpp"
#include "ipc/HealthCheck.hpp"
#include "ipc/NngRepServer.hpp"
#include "net/HTTPClient.hpp"
#include "net/WebsocketClient.hpp"
#include "repository/ImageRepository.hpp"
#include "repository/TokenRepository.hpp"
#include "task/ImageCheck.hpp"
#include "task/ImageUpdate.hpp"
#include <spdlog/spdlog.h>

// Owns network + image ingest. WebsocketClient parses server messages into the
// EventBus; ImageUpdate persists images; EventBridge forwards events over nng
// PUB for the display service; a REP socket answers health probes.
int main(int argc, char* argv[])
{
    auto [cfg, profile] = bootstrap(argc, argv);
    initLogging(cfg, cfg.websocketApplication.logFile);
    spdlog::info("shareframe-websocket v{} starting [profile: {}]", cfg.version, profileName(profile));

    // Does NOT migrate: the shareframe-migrate oneshot runs migrations first.
    Database database;
    database.init(cfg.database, false);
    TokenRepository tokenRepo(database.get());
    ImageRepository imageRepo(database.get());

    ImageManager imageManager(cfg, imageRepo);

    HTTPClient http(60, 600);
    AuthTokenManager authTokenManager(cfg, tokenRepo, http);

    EventBus eventBus;

    // Bridge subscribes the bus, so wire it before eventBus.start().
    EventPublisher publisher(cfg.ipc.wsPub);
    publisher.start();
    EventBridge bridge(eventBus, publisher);
    bridge.wire();

    // Tasks subscribe to the bus in start() — also before eventBus.start().
    WebsocketClient wsClient(eventBus, cfg, authTokenManager);
    wsClient.start();
    ImageUpdate imageUpdate(eventBus, imageManager, imageRepo);
    imageUpdate.start();
    ImageCheck imageCheck(eventBus, cfg, imageRepo);
    imageCheck.start();

    eventBus.start();

    // REP: this service only answers health probes.
    NngRepServer repServer(cfg.ipc.wsRep, health::respond);
    repServer.start();

    const int sig = waitForSignal();
    spdlog::info("Received signal {}, shutting down", sig);
    repServer.stop();
    eventBus.stop();
    imageCheck.stop();
    wsClient.stop();
    imageUpdate.stop();

    return 0;
}
