#include "db/MigrationRunner.hpp"
#include "settings/RuntimeSettings.hpp"
#include "support/TestDb.hpp"
#include <gtest/gtest.h>

namespace
{
AppConfig config()
{
    AppConfig cfg;
    cfg.display.intervalSecs = 900;
    cfg.display.nightModeEnabled = false;
    cfg.display.nightStartHour = 2;
    cfg.display.nightEndHour = 5;
    cfg.display.nightIntervalSecs = 3600;
    cfg.display.nightBlank = false;
    return cfg;
}
}

TEST(RuntimeSettings, ChangesSurviveRestart)
{
    TestDb db;
    SettingsRepository repo(db.get());
    {
        RuntimeSettings s(repo, config());
        s.setDisplayInterval(1200);
        s.setSlideshowActive(false);
        s.setNightMode(true, 22, 6, 7200, true);
        s.setImageQuality(ImageQuality::Gray4);
        s.setPlaybackOrder(PlaybackMode::Custom, {3, 1, 2});
    }
    RuntimeSettings s(repo, config());
    EXPECT_EQ(s.getDisplayInterval(), 1200);
    EXPECT_FALSE(s.isSlideshowActive());
    EXPECT_TRUE(s.isNightModeEnabled());
    EXPECT_EQ(s.getNightStartHour(), 22);
    EXPECT_EQ(s.getNightEndHour(), 6);
    EXPECT_EQ(s.getNightInterval(), 7200);
    EXPECT_TRUE(s.isNightBlank());
    EXPECT_EQ(s.getImageQuality(), ImageQuality::Gray4);
    EXPECT_EQ(s.getPlaybackMode(), PlaybackMode::Custom);
    EXPECT_EQ(s.getCustomOrder(), (std::vector<int64_t>{3, 1, 2}));
}

TEST(RuntimeSettings, MigrationBlanksNightsOnceAndTheUserChoiceSticks)
{
    TestDb db;
    SettingsRepository repo(db.get());
    {
        RuntimeSettings s(repo, config());
        EXPECT_TRUE(s.isNightModeEnabled());
        EXPECT_TRUE(s.isNightBlank());
        EXPECT_EQ(s.getNightStartHour(), 1);
        EXPECT_EQ(s.getNightEndHour(), 6);
        s.setNightMode(false, 22, 5, 7200, false);
    }
    MigrationRunner(db.get(), SHAREFRAME_MIGRATIONS_DIR).run();
    RuntimeSettings s(repo, config());
    EXPECT_FALSE(s.isNightModeEnabled());
    EXPECT_FALSE(s.isNightBlank());
    EXPECT_EQ(s.getNightStartHour(), 22);
}

TEST(RuntimeSettings, ImageQualityDefaultsToMono)
{
    TestDb db;
    SettingsRepository repo(db.get());
    EXPECT_EQ(RuntimeSettings(repo, config()).getImageQuality(), ImageQuality::Mono);
}

TEST(RuntimeSettings, GarbageInDbFallsBackToConfig)
{
    TestDb db;
    SettingsRepository repo(db.get());
    repo.set("display_interval", "abc");
    repo.set("night_start_hour", "99");
    repo.set("night_end_hour", "-1");
    repo.set("night_interval", "0");
    repo.set("image_quality", "sepia");
    repo.set("playback_mode", "backwards");
    repo.set("playback_custom_order", "[1, \"x\"");
    RuntimeSettings s(repo, config());
    EXPECT_EQ(s.getDisplayInterval(), 900);
    EXPECT_EQ(s.getNightStartHour(), 2);
    EXPECT_EQ(s.getNightEndHour(), 5);
    EXPECT_EQ(s.getNightInterval(), 3600);
    EXPECT_EQ(s.getImageQuality(), ImageQuality::Mono);
    EXPECT_EQ(s.getPlaybackMode(), PlaybackMode::Chronological);
    EXPECT_TRUE(s.getCustomOrder().empty());
}

TEST(RuntimeSettings, InsideNightWindowEffectiveIntervalClampsToWindowEnd)
{
    TestDb db;
    SettingsRepository repo(db.get());
    RuntimeSettings s(repo, config());

    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    const int start = local.tm_hour;
    const int end = (local.tm_hour + 1) % 24;
    s.setNightMode(true, start, end, 86400, false);

    EXPECT_TRUE(s.isNightNow());
    EXPECT_GT(s.getEffectiveInterval(), 0);
    EXPECT_LE(s.getEffectiveInterval(), 3600);
}

TEST(RuntimeSettings, RestoredHoursThatCollapseTheWindowNeverMeanNightAllDay)
{
    TestDb db;
    SettingsRepository repo(db.get());
    repo.set("night_mode_enabled", "1");
    repo.set("night_start_hour", "5");
    repo.set("night_end_hour", "5");
    RuntimeSettings s(repo, config());
    EXPECT_FALSE(s.isNightNow());
    EXPECT_EQ(s.getEffectiveInterval(), 900);
}

TEST(RuntimeSettings, BlankNightReportsWhenItNextStartsOrEnds)
{
    TestDb db;
    SettingsRepository repo(db.get());
    RuntimeSettings s(repo, config());

    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    const int hour = local.tm_hour;

    s.setNightMode(true, hour, (hour + 2) % 24, 600, true);
    EXPECT_TRUE(s.isNightBlankNow());
    EXPECT_GT(s.secsUntilNightBlankChange(), 3600);

    s.setNightMode(true, (hour + 2) % 24, (hour + 3) % 24, 600, true);
    EXPECT_FALSE(s.isNightBlankNow());
    EXPECT_GT(s.secsUntilNightBlankChange(), 3600);
    EXPECT_LE(s.secsUntilNightBlankChange(), 2 * 3600);

    s.setNightMode(true, hour, (hour + 2) % 24, 600, false);
    EXPECT_FALSE(s.isNightBlankNow());
    EXPECT_EQ(s.secsUntilNightBlankChange(), -1);
}
