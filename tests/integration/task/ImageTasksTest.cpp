#include "support/ImageFixture.hpp"
#include "support/Wait.hpp"
#include "task/ExpiredImageCleanup.hpp"
#include "task/ImageUpdate.hpp"
#include "task/MissingImageCheck.hpp"
#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
class ImageUpdateTest : public ::testing::Test, protected ImageFixture
{
protected:
    void SetUp() override
    {
        task.start();
        bus.start();
    }

    void TearDown() override
    {
        task.stop();
        bus.stop();
    }

    nlohmann::json picture(const nlohmann::json& id) const
    {
        return {{"type", "picture"}, {"sent_image_id", id}, {"sender", "alice"},
                {"data", jpegBase64()}, {"expiry_unix_timestamp", std::time(nullptr) + 3600}};
    }

    ImageManager mgr{cfg, repo};
    Recorder<Topic::IMAGE_NEW> newImages{bus};
    Recorder<Topic::IMAGE_REMOVED> removed{bus};
    ImageUpdate task{bus, mgr, repo};
};
}

TEST_F(ImageUpdateTest, PictureIsStoredAndAnnounced)
{
    bus.publish<Topic::PICTURE>(picture(5));
    ASSERT_TRUE(waitFor([&] { return newImages.size() == 1; }));
    EXPECT_EQ(newImages.get()[0], 5);
    EXPECT_EQ(repo.getAllIds(), std::vector<int64_t>{5});
}

TEST_F(ImageUpdateTest, DuplicatePictureIsIgnored)
{
    bus.publish<Topic::PICTURE>(picture(5));
    bus.publish<Topic::PICTURE>(picture(5));
    bus.publish<Topic::PICTURE>(picture(6));
    ASSERT_TRUE(waitFor([&] { return repo.getAllIds().size() == 2; }));
    std::this_thread::sleep_for(100ms);
    EXPECT_EQ(newImages.size(), 2u);
}

TEST_F(ImageUpdateTest, IdAbove32BitsIsKeptExact)
{
    constexpr int64_t big = 5'000'000'000;
    bus.publish<Topic::PICTURE>(picture(big));
    ASSERT_TRUE(waitFor([&] { return newImages.size() == 1; }));
    EXPECT_EQ(newImages.get()[0], big);
    EXPECT_EQ(repo.getAllIds(), std::vector<int64_t>{big});
}

TEST_F(ImageUpdateTest, StringIdIsAccepted)
{
    bus.publish<Topic::PICTURE>(picture("12"));
    ASSERT_TRUE(waitFor([&] { return newImages.size() == 1; }));
    EXPECT_EQ(newImages.get()[0], 12);
}

TEST_F(ImageUpdateTest, IncompleteOrBrokenPicturesAreDroppedAndTaskSurvives)
{
    auto noSender = picture(1);
    noSender.erase("sender");
    auto badExpiry = picture(2);
    badExpiry["expiry_unix_timestamp"] = "soon";
    auto badData = picture(3);
    badData["data"] = "%%%";
    bus.publish<Topic::PICTURE>(noSender);
    bus.publish<Topic::PICTURE>(badExpiry);
    bus.publish<Topic::PICTURE>(badData);
    bus.publish<Topic::PICTURE>(picture(4));

    ASSERT_TRUE(waitFor([&] { return newImages.size() == 1; }));
    EXPECT_EQ(newImages.get()[0], 4);
    EXPECT_EQ(repo.getAllIds(), std::vector<int64_t>{4});
}

TEST_F(ImageUpdateTest, ClearImagesRemovesAndAnnounces)
{
    bus.publish<Topic::PICTURE>(picture(1));
    bus.publish<Topic::PICTURE>(picture(2));
    ASSERT_TRUE(waitFor([&] { return newImages.size() == 2; }));

    bus.publish<Topic::CLEAR_IMAGES>({{"sent_image_ids", {1}}});
    ASSERT_TRUE(waitFor([&] { return removed.size() == 1; }));
    EXPECT_EQ(removed.get()[0], std::vector<int64_t>{1});
    EXPECT_EQ(repo.getAllIds(), std::vector<int64_t>{2});
}

TEST(MissingImageCheck, SendsKnownIdsOnEveryConnect)
{
    ImageFixture f;
    f.repo.save({3, "a", "/x", 0});
    f.repo.save({4, "a", "/y", 0});
    Recorder<Topic::WS_SEND> sent{f.bus};
    MissingImageCheck task(f.bus, f.repo);
    task.start();
    f.bus.start();

    f.bus.publish<Topic::WS_CONNECTED>({});
    ASSERT_TRUE(waitFor([&] { return sent.size() == 1; }));
    const auto msg = sent.get()[0];
    EXPECT_EQ(msg["type"], "check_missing_images");
    auto ids = msg["sent_image_ids"].get<std::vector<int64_t>>();
    std::ranges::sort(ids);
    EXPECT_EQ(ids, (std::vector<int64_t>{3, 4}));

    f.bus.publish<Topic::WS_CONNECTED>({});
    EXPECT_TRUE(waitFor([&] { return sent.size() == 2; }));
    task.stop();
    f.bus.stop();
}

TEST(ExpiredImageCleanup, RemovesExpiredOnStartAndAnnounces)
{
    ImageFixture f;
    ImageManager mgr(f.cfg, f.repo);
    ASSERT_TRUE(mgr.saveImage(1, "a", ImageFixture::jpegBase64(), std::time(nullptr) - 1));
    ASSERT_TRUE(mgr.saveImage(2, "a", ImageFixture::jpegBase64(), std::time(nullptr) + 3600));
    Recorder<Topic::IMAGE_REMOVED> removed{f.bus};
    ExpiredImageCleanup task(f.bus, f.cfg, mgr);
    f.bus.start();
    task.start();

    ASSERT_TRUE(waitFor([&] { return removed.size() == 1; }));
    EXPECT_EQ(removed.get()[0], std::vector<int64_t>{1});
    EXPECT_EQ(f.repo.getAllIds(), std::vector<int64_t>{2});
    task.stop();
    f.bus.stop();
}
