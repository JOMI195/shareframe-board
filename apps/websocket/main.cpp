#include "app/Bootstrap.hpp"
#include "auth/AuthTokenManager.hpp"
#include "db/Database.hpp"
#include "events/EventBus.hpp"
#include "image/ImageManager.hpp"
#include "ipc/EventBridge.hpp"
#include "ipc/EventPublisher.hpp"
#include "ipc/IpcClient.hpp"
#include "ipc/NngRepServer.hpp"
#include "net/DiagnosticsUploader.hpp"
#include "net/HTTPClient.hpp"
#include "net/WebsocketClient.hpp"
#include "net/WsIpc.hpp"
#include "repository/ImageRepository.hpp"
#include "repository/SettingsRepository.hpp"
#include "repository/TokenRepository.hpp"
#include "task/ExpiredImageCleanup.hpp"
#include "task/ImageUpdate.hpp"
#include "task/MissingImageCheck.hpp"
#include "task/RemoteCommand.hpp"
#include "Version.hpp"
#include <spdlog/spdlog.h>

// Owns network + image ingest. WebsocketClient parses server messages into the
// EventBus; ImageUpdate persists images; EventBridge forwards events over nng
// PUB for the display service; a REP socket answers health probes and the
// server link state.
int main(int argc, char* argv[])
{
    auto [cfg, profile] = bootstrap(argc, argv);
    initLogging(cfg, cfg.websocketApplication.logFile);
    spdlog::info("shareframe-websocket v{} ({}) starting [profile: {}]", cfg.version, shareframe::kAppSha, profileName(profile));

    // Does NOT migrate: the shareframe-migrate oneshot runs migrations first.
    Database database;
    database.init(cfg.database, false);
    TokenRepository tokenRepo(database.get());
    ImageRepository imageRepo(database.get());
    SettingsRepository settingsRepo(database.get());
    ServerContact serverContact(settingsRepo);

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
    WebsocketClient wsClient(eventBus, cfg, authTokenManager, serverContact);
    wsClient.start();
    ImageUpdate imageUpdate(eventBus, imageManager, imageRepo);
    imageUpdate.start();
    MissingImageCheck missingImageCheck(eventBus, imageRepo);
    missingImageCheck.start();
    ExpiredImageCleanup expiredImageCleanup(eventBus, cfg, imageManager);
    expiredImageCleanup.start();
    IpcClient displayIpc(cfg.ipc.displayRep);
    IpcClient updateIpc(cfg.ipc.updateRep);
    DiagnosticsUploader diagnostics(cfg, http, authTokenManager);
    RemoteCommand remoteCommand(eventBus, displayIpc, updateIpc, diagnostics);
    remoteCommand.start();

    eventBus.start();

    WsIpc wsIpc(serverContact);
    NngRepServer repServer(cfg.ipc.wsRep, [&](const IpcMessage& msg) { return wsIpc.handle(msg); });
    repServer.start();

    const int sig = waitForSignal();
    spdlog::info("Received signal {}, shutting down", sig);
    repServer.stop();
    eventBus.stop();
    missingImageCheck.stop();
    remoteCommand.stop();
    displayIpc.disconnect();
    updateIpc.disconnect();
    expiredImageCleanup.stop();
    wsClient.stop();
    imageUpdate.stop();

    return 0;
}
