#include "app/Bootstrap.hpp"
#include "db/Database.hpp"
#include <spdlog/spdlog.h>
#include <exception>

// Standalone DB migration entrypoint: runs as the s6 oneshot before any service,
// and doubles as a dev DB-setup tool. The logic lives in shareframe-common; this
// is a thin wrapper. Exits non-zero on failure so the oneshot fail-stops.
int main(int argc, char* argv[])
{
    auto [cfg, profile] = bootstrap(argc, argv);
    initLogging(cfg, "shareframe-migrate.log");
    spdlog::info("shareframe-migrate v{} starting [profile: {}]", cfg.version, profileName(profile));

    try
    {
        Database database;
        database.init(cfg.database, true); // open + run pending migrations (idempotent)
    }
    catch (const std::exception& e)
    {
        spdlog::critical("Migration failed: {}", e.what());
        return 1;
    }

    spdlog::info("Migrations complete");
    return 0;
}
