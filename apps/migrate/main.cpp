#include "app/Bootstrap.hpp"
#include "db/Database.hpp"
#include <spdlog/spdlog.h>
#include <exception>

// Standalone DB migration entrypoint. On the device this runs as the
// `shareframe-db-migrate` s6 oneshot, before any service starts; on dev it is a
// one-command DB setup tool: `SHAREFRAME_PROFILE=dev ./shareframe-migrate`.
//
// The migration logic itself lives in shareframe-common (Database +
// MigrationRunner) and is equally callable from tests via
// `Database::init(cfg, true)` — this binary is just a thin entrypoint, not a
// separate application. Exits non-zero on failure so the s6 oneshot fail-stops
// (dependent services won't start against an unmigrated DB).
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
