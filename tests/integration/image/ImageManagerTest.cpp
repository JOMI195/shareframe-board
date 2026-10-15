#include "support/ImageFixture.hpp"
#include <fstream>
#include <gtest/gtest.h>

namespace
{
class ImageManagerTest : public ::testing::Test, protected ImageFixture
{
};

bool isInside(const std::filesystem::path& file, const std::filesystem::path& dir)
{
    const auto f = std::filesystem::weakly_canonical(file);
    const auto d = std::filesystem::weakly_canonical(dir);
    return std::mismatch(d.begin(), d.end(), f.begin(), f.end()).first == d.end();
}
}

TEST_F(ImageManagerTest, SaveWritesDecodedFileAndRow)
{
    ImageManager mgr(cfg, repo);
    const auto saved = mgr.saveImage(7, "alice", jpegBase64(), 123);
    ASSERT_TRUE(saved);
    EXPECT_TRUE(std::filesystem::exists(saved->imagePath));
    EXPECT_EQ(std::filesystem::file_size(saved->imagePath), 12u);
    EXPECT_EQ(repo.getAllIds(), std::vector<int64_t>{7});
}

TEST_F(ImageManagerTest, InvalidBase64IsRejected)
{
    ImageManager mgr(cfg, repo);
    EXPECT_FALSE(mgr.saveImage(7, "alice", "!!!not base64!!!", 123));
    EXPECT_TRUE(repo.getAll().empty());
}

TEST_F(ImageManagerTest, SenderCannotEscapeImageDirectory)
{
    ImageManager mgr(cfg, repo);
    const auto saved = mgr.saveImage(8, "../../escaped", jpegBase64(), 123);
    ASSERT_TRUE(saved);
    EXPECT_TRUE(isInside(saved->imagePath, cfg.image.imageSavePath)) << saved->imagePath;
    EXPECT_EQ(saved->sender, "../../escaped");
}

TEST_F(ImageManagerTest, SenderWithSlashStillSaves)
{
    ImageManager mgr(cfg, repo);
    const auto saved = mgr.saveImage(9, "a/b", jpegBase64(), 123);
    ASSERT_TRUE(saved);
    EXPECT_TRUE(std::filesystem::exists(saved->imagePath));
}

TEST_F(ImageManagerTest, RemoveImagesDeletesFilesAndRows)
{
    ImageManager mgr(cfg, repo);
    const auto a = mgr.saveImage(1, "alice", jpegBase64(), 123);
    const auto b = mgr.saveImage(2, "alice", jpegBase64(), 123);
    mgr.removeImages({1});
    EXPECT_FALSE(std::filesystem::exists(a->imagePath));
    EXPECT_TRUE(std::filesystem::exists(b->imagePath));
    EXPECT_EQ(repo.getAllIds(), std::vector<int64_t>{2});
}

TEST_F(ImageManagerTest, RemoveExpiredOnlyTouchesPastImages)
{
    ImageManager mgr(cfg, repo);
    const auto now = std::time(nullptr);
    const auto old = mgr.saveImage(1, "alice", jpegBase64(), now - 10);
    const auto fresh = mgr.saveImage(2, "alice", jpegBase64(), now + 3600);

    EXPECT_EQ(mgr.removeExpired(), std::vector<int64_t>{1});
    EXPECT_FALSE(std::filesystem::exists(old->imagePath));
    EXPECT_TRUE(std::filesystem::exists(fresh->imagePath));
}

TEST_F(ImageManagerTest, OrphanedFilesAreRemovedOnStartup)
{
    std::filesystem::create_directories(cfg.image.imageSavePath);
    const auto orphan = std::filesystem::path(cfg.image.imageSavePath) / "orphan.jpg";
    std::ofstream(orphan) << "x";
    std::filesystem::path kept;
    {
        ImageManager mgr(cfg, repo);
        kept = mgr.saveImage(1, "alice", jpegBase64(), 123)->imagePath;
    }
    ImageManager restarted(cfg, repo);
    EXPECT_FALSE(std::filesystem::exists(orphan));
    EXPECT_TRUE(std::filesystem::exists(kept));
}
