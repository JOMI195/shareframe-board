#include "util/SysInfo.hpp"
#include <gtest/gtest.h>

TEST(SysInfo, IdentifiersKeepTheirTextEvenWhenTheyLookLikeNumbers)
{
    const auto info = SysInfo::parse("serial=000123ab\nos_sha=1234567\nversion=1.10\nhostname=1234\nboot_slot=A\n");
    EXPECT_EQ(info.at("serial"), "000123ab");
    EXPECT_EQ(info.at("os_sha"), "1234567");
    EXPECT_EQ(info.at("version"), "1.10");
    EXPECT_EQ(info.at("hostname"), "1234");
}

TEST(SysInfo, MetricsAndCountersAreNumbers)
{
    const auto info = SysInfo::parse(
        "cpu_temp_celsius=48.3\nuptime_seconds=3720\nram_total_bytes=448790528\nload_1=0.25\n"
        "boot_count=12\ncrash_count=0\nwlan_signal_dbm=-61\nproc_display_threads=3\ndata_written_kb=1024\n");
    EXPECT_DOUBLE_EQ(info.at("cpu_temp_celsius").get<double>(), 48.3);
    EXPECT_EQ(info.at("uptime_seconds"), 3720);
    EXPECT_EQ(info.at("ram_total_bytes"), 448790528);
    EXPECT_DOUBLE_EQ(info.at("load_1").get<double>(), 0.25);
    EXPECT_EQ(info.at("boot_count"), 12);
    EXPECT_EQ(info.at("crash_count"), 0);
    EXPECT_EQ(info.at("wlan_signal_dbm"), -61);
    EXPECT_EQ(info.at("proc_display_threads"), 3);
    EXPECT_EQ(info.at("data_written_kb"), 1024);
}

TEST(SysInfo, FlagsAreBooleansAndAMetricThatIsNoNumberStaysText)
{
    const auto info = SysInfo::parse("wlan_connected=true\nslot_trial=false\ncpu_temp_celsius=n/a\n");
    EXPECT_EQ(info.at("wlan_connected"), true);
    EXPECT_EQ(info.at("slot_trial"), false);
    EXPECT_EQ(info.at("cpu_temp_celsius"), "n/a");
}

TEST(SysInfo, ValuesMayContainEqualsAndLinesWithoutKeyAreSkipped)
{
    const auto info = SysInfo::parse("sd_card=SD16G mfr=0x03 date=2024\n=orphan\nnot a pair\nkernel=Linux 6.18.12\n");
    EXPECT_EQ(info.at("sd_card"), "SD16G mfr=0x03 date=2024");
    EXPECT_EQ(info.at("kernel"), "Linux 6.18.12");
    EXPECT_EQ(info.size(), 2u);
}
