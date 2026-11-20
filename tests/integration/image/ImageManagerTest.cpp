#include "support/ImageFixture.hpp"
#include "util/Base64.hpp"
#include <csignal>
#include <fstream>
#include <limits>
#include <sys/resource.h>
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
    EXPECT_EQ(std::filesystem::file_size(saved->imagePath), jpegBytes().size());
    EXPECT_EQ(repo.getAllIds(), std::vector<int64_t>{7});
}

TEST_F(ImageManagerTest, OnlySmallEnoughJpegsAndPngsAreStored)
{
    ImageManager mgr(cfg, repo);
    // A 1x1 GIF, then a PNG whose header claims 5000x5000 pixels: refused from the header, never decoded.
    EXPECT_FALSE(mgr.saveImage(1, "a", "R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7", 123));
    const unsigned char hugePng[] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 'I', 'H', 'D', 'R',
                                     0x00, 0x00, 0x13, 0x88, 0x00, 0x00, 0x13, 0x88, 0x08, 0x02, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00};
    EXPECT_FALSE(mgr.saveImage(2, "a", Base64::encode(hugePng, sizeof hugePng), 123));
    EXPECT_TRUE(repo.getAll().empty());
    EXPECT_TRUE(std::filesystem::is_empty(cfg.image.imageSavePath));
}

TEST_F(ImageManagerTest, APictureThatWouldEatTheFreeSpaceReserveIsRefused)
{
    cfg.image.minFreeMb = std::numeric_limits<int>::max();
    ImageManager mgr(cfg, repo);
    EXPECT_FALSE(mgr.saveImage(1, "a", jpegBase64(), 123));
    EXPECT_TRUE(repo.getAll().empty());
}

TEST_F(ImageManagerTest, AFailedWriteRegistersNothingAndLeavesNoFile)
{
    ImageManager mgr(cfg, repo);
    // A full disk, as the file-size limit: the write fails part way, like with no space left.
    struct FileSizeLimit
    {
        rlimit old{};
        FileSizeLimit(const rlim_t bytes)
        {
            std::signal(SIGXFSZ, SIG_IGN);
            getrlimit(RLIMIT_FSIZE, &old);
            const rlimit limit{bytes, old.rlim_max};
            setrlimit(RLIMIT_FSIZE, &limit);
        }
        ~FileSizeLimit() { setrlimit(RLIMIT_FSIZE, &old); std::signal(SIGXFSZ, SIG_DFL); }
    };
    {
        FileSizeLimit limit(64);
        EXPECT_FALSE(mgr.saveImage(1, "a", jpegBase64(), 123));
    }
    EXPECT_TRUE(repo.getAll().empty());
    EXPECT_TRUE(std::filesystem::is_empty(cfg.image.imageSavePath));
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
