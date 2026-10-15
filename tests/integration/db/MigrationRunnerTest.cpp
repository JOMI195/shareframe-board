#include "db/MigrationRunner.hpp"
#include "support/TempDir.hpp"
#include "support/TestDb.hpp"
#include <fstream>
#include <gtest/gtest.h>

namespace
{
int count(SQLite::Database& db, const std::string& sql)
{
    return db.execAndGet(sql).getInt();
}
}

TEST(MigrationRunner, AppliesAllProjectMigrations)
{
    TestDb db;
    for (const auto* table : {"tokens", "images", "settings", "display_metrics"})
        EXPECT_TRUE(db.get().tableExists(table)) << table;
    EXPECT_EQ(count(db.get(), "SELECT COUNT(*) FROM schema_migrations"), 5);
}

TEST(MigrationRunner, RerunIsIdempotent)
{
    TestDb db;
    MigrationRunner(db.get(), SHAREFRAME_MIGRATIONS_DIR).run();
    EXPECT_EQ(count(db.get(), "SELECT COUNT(*) FROM schema_migrations"), 5);
}

TEST(MigrationRunner, AppliesInFilenameOrderAndSkipsNonSql)
{
    TempDir dir;
    std::filesystem::create_directories(dir / "m");
    std::ofstream(dir / "m" / "002_b.sql") << "INSERT INTO t VALUES (2);";
    std::ofstream(dir / "m" / "001_a.sql") << "CREATE TABLE t (v INTEGER);";
    std::ofstream(dir / "m" / "README.md") << "not sql";

    SQLite::Database db((dir / "x.db").string(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    MigrationRunner(db, dir / "m").run();
    EXPECT_EQ(count(db, "SELECT v FROM t"), 2);
}

TEST(MigrationRunner, FailingMigrationRollsBackAndIsNotRecorded)
{
    TempDir dir;
    std::filesystem::create_directories(dir / "m");
    std::ofstream(dir / "m" / "001_bad.sql") << "CREATE TABLE ok (v INTEGER); SELEKT nonsense;";

    SQLite::Database db((dir / "x.db").string(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    EXPECT_ANY_THROW(MigrationRunner(db, dir / "m").run());
    EXPECT_FALSE(db.tableExists("ok"));
    EXPECT_EQ(count(db, "SELECT COUNT(*) FROM schema_migrations"), 0);
}
