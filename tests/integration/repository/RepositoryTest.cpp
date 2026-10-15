#include "repository/DisplayMetricsRepository.hpp"
#include "repository/ImageRepository.hpp"
#include "repository/SettingsRepository.hpp"
#include "repository/TokenRepository.hpp"
#include "support/TestDb.hpp"
#include <algorithm>
#include <gtest/gtest.h>

namespace
{
Image img(int64_t id, int64_t expires = 100) { return {id, "alice", "/img/" + std::to_string(id) + ".jpg", expires}; }

std::vector<int64_t> sorted(std::vector<int64_t> v)
{
    std::ranges::sort(v);
    return v;
}
}

TEST(ImageRepository, SaveGetAndReplace)
{
    TestDb db;
    ImageRepository repo(db.get());
    repo.save(img(1));
    repo.save(img(2));
    repo.save({1, "bob", "/img/new.jpg", 5});

    const auto all = repo.getAll();
    ASSERT_EQ(all.size(), 2u);
    const auto one = repo.getByIds({1});
    ASSERT_EQ(one.size(), 1u);
    EXPECT_EQ(one[0].sender, "bob");
    EXPECT_EQ(one[0].imagePath, "/img/new.jpg");
    EXPECT_EQ(one[0].expiresAt, 5);
}

TEST(ImageRepository, LargeIdsSurviveRoundTrip)
{
    TestDb db;
    ImageRepository repo(db.get());
    constexpr int64_t big = 9'000'000'000;
    repo.save(img(big));
    EXPECT_EQ(repo.getAllIds(), std::vector<int64_t>{big});
}

TEST(ImageRepository, BulkRemoveAndClear)
{
    TestDb db;
    ImageRepository repo(db.get());
    for (int64_t i = 1; i <= 5; ++i)
        repo.save(img(i));

    repo.removeByIds({2, 4, 99});
    EXPECT_EQ(sorted(repo.getAllIds()), (std::vector<int64_t>{1, 3, 5}));
    repo.remove(1);
    EXPECT_EQ(sorted(repo.getAllIds()), (std::vector<int64_t>{3, 5}));
    repo.removeByIds({});
    EXPECT_EQ(repo.getByIds({}).size(), 0u);
    repo.clear();
    EXPECT_TRUE(repo.getAll().empty());
}

TEST(TokenRepository, KeepsOnlyLatestToken)
{
    TestDb db;
    TokenRepository repo(db.get());
    EXPECT_FALSE(repo.get().has_value());
    repo.save({"a", 1});
    repo.save({"b", 2});
    ASSERT_TRUE(repo.get().has_value());
    EXPECT_EQ(repo.get()->value, "b");
    EXPECT_EQ(db.get().execAndGet("SELECT COUNT(*) FROM tokens").getInt(), 1);
    repo.clear();
    EXPECT_FALSE(repo.get().has_value());
}

TEST(SettingsRepository, UpsertAndMissingKey)
{
    TestDb db;
    SettingsRepository repo(db.get());
    EXPECT_FALSE(repo.get("k").has_value());
    repo.set("k", "1");
    repo.set("k", "2");
    EXPECT_EQ(repo.get("k"), "2");
}

TEST(DisplayMetricsRepository, IncrementSetAndSetIfUnset)
{
    TestDb db;
    DisplayMetricsRepository repo(db.get());
    EXPECT_EQ(repo.get("refresh"), 0);
    repo.increment("refresh");
    repo.increment("refresh", 4);
    EXPECT_EQ(repo.get("refresh"), 5);

    repo.set("last", 10);
    repo.set("last", 20);
    EXPECT_EQ(repo.get("last"), 20);

    repo.setIfUnset("first", 1);
    repo.setIfUnset("first", 2);
    EXPECT_EQ(repo.get("first"), 1);

    EXPECT_EQ(repo.all(), (std::map<std::string, int64_t>{{"first", 1}, {"last", 20}, {"refresh", 5}}));
}
