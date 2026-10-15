#include "settings/PlaybackOrder.hpp"
#include <algorithm>
#include <gtest/gtest.h>

namespace
{
Image img(const int64_t id, const int64_t receivedAt) { return {id, "alice", "", 0, receivedAt}; }

// Received in the order 30, 10, 20; 10 and 40 share a timestamp.
const std::vector<Image> kImages = {img(10, 200), img(20, 300), img(30, 100), img(40, 200)};

std::vector<int64_t> sorted(std::vector<int64_t> v)
{
    std::ranges::sort(v);
    return v;
}
}

TEST(PlaybackOrder, ChronologicalFollowsArrivalNotIdAndBreaksTiesById)
{
    EXPECT_EQ(playback::arrange(kImages, PlaybackMode::Chronological), (std::vector<int64_t>{30, 10, 40, 20}));
    EXPECT_EQ(playback::arrange(kImages, PlaybackMode::NewestFirst), (std::vector<int64_t>{20, 40, 10, 30}));
}

TEST(PlaybackOrder, CustomDropsRemovedPicturesAndAppendsNewOnesChronologically)
{
    EXPECT_EQ(playback::arrange(kImages, PlaybackMode::Custom, {20, 99, 30, 20}),
              (std::vector<int64_t>{20, 30, 10, 40}));
}

TEST(PlaybackOrder, NextWrapsAroundAndRestartsWhenTheCurrentPictureIsGone)
{
    const std::vector<int64_t> seq = {30, 10, 20};
    EXPECT_EQ(playback::nextAfter(seq, 30), 10);
    EXPECT_EQ(playback::nextAfter(seq, 20), 30);
    EXPECT_EQ(playback::nextAfter(seq, 77), 30);
    EXPECT_EQ(playback::nextAfter({}, 1), 0);
}

TEST(PlaybackOrder, ShuffleShowsEveryPictureOncePerCycleAndNeverTwiceInARow)
{
    const std::vector<int64_t> ids = {1, 2, 3, 4, 5};
    for (unsigned seed = 0; seed < 50; ++seed)
    {
        playback::ShuffleBag bag(seed);
        bag.sync(ids);
        int64_t last = 0;
        for (int cycle = 0; cycle < 4; ++cycle)
        {
            std::vector<int64_t> shown;
            for (size_t i = 0; i < ids.size(); ++i)
            {
                shown.push_back(bag.next());
                ASSERT_NE(shown.back(), last) << "seed " << seed;
                last = shown.back();
            }
            EXPECT_EQ(sorted(shown), ids) << "seed " << seed << " cycle " << cycle;
        }
    }
}

TEST(PlaybackOrder, ShufflePlaysAPictureThatArrivesMidCycleBeforeRepeatingAny)
{
    for (unsigned seed = 0; seed < 50; ++seed)
    {
        playback::ShuffleBag bag(seed);
        bag.sync({1, 2, 3});
        std::vector<int64_t> shown = {bag.next(), bag.next()};
        bag.sync({1, 2, 3, 4});
        shown.push_back(bag.next());
        shown.push_back(bag.next());
        EXPECT_EQ(sorted(shown), (std::vector<int64_t>{1, 2, 3, 4})) << "seed " << seed;
    }
}

TEST(PlaybackOrder, ShuffleSkipsRemovedPicturesWithoutRestartingTheCycle)
{
    playback::ShuffleBag bag(7);
    bag.sync({1, 2, 3, 4});
    const int64_t first = bag.next();
    std::vector<int64_t> rest;
    for (const int64_t id : bag.order())
        if (id != first)
            rest.push_back(id);
    bag.sync({rest[0], rest[2], first});
    EXPECT_EQ(bag.next(), rest[0]);
    EXPECT_EQ(bag.next(), rest[2]);
}
