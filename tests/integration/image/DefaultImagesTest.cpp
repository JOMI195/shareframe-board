#include "image/DefaultImages.hpp"
#include "support/PngWriter.hpp"
#include "support/TempDir.hpp"
#include <chrono>
#include <fstream>
#include <functional>
#include <gtest/gtest.h>
#include <set>

namespace
{
const std::filesystem::path kBundled = std::filesystem::path(SHAREFRAME_ASSETS_DIR) / "image/default";

std::vector<uint8_t> png(const TempDir& tmp, const std::function<uint8_t(int x, int y)>& shade)
{
    const auto path = tmp / "upload.png";
    writeGreyPng(path, 800, 480, shade);
    std::ifstream f(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), {}};
}

std::set<std::string> names(const std::vector<DefaultImages::Entry>& entries)
{
    std::set<std::string> out;
    for (const auto& e : entries)
        out.insert(e.name);
    return out;
}

// Large flat shapes, far apart on the panel from each other and from the bundled photos.
const std::vector<std::function<uint8_t(int, int)>> kShapes = {
    [](int x, int) { return x < 400 ? 0 : 255; },
    [](int, int y) { return y < 240 ? 0 : 255; },
    [](int x, int y) { return (x < 400) == (y < 240) ? 0 : 255; },
    [](int x, int y) { return x * 480 < y * 800 ? 0 : 255; },
    [](int x, int y) { return std::abs(x - 400) < 150 && std::abs(y - 240) < 120 ? 0 : 255; },
    [](int x, int) { return x < 200 || x >= 600 ? 0 : 255; },
    [](int, int y) { return y < 120 || y >= 360 ? 0 : 255; },
    [](int x, int y) { return (x / 200 + y / 160) % 2 ? 0 : 255; },
};
} // namespace

TEST(DefaultImages, AFreshFrameStartsWithTheBundledSet)
{
    TempDir tmp;
    DefaultImages pictures(tmp / "default-images", kBundled, 0);

    pictures.ensureSeeded();

    const auto entries = pictures.list();
    ASSERT_EQ(entries.size(), 3u);
    for (const auto& e : entries)
        EXPECT_TRUE(DefaultImages::isValidName(e.name)) << e.name;
    EXPECT_EQ(DefaultImages::playable(tmp / "default-images", kBundled).front().parent_path(), tmp / "default-images");
}

TEST(DefaultImages, AShortOwnSetPlaysTheBundledOneAndHalfWrittenFilesNeverPlay)
{
    TempDir tmp;
    const auto own = tmp / "default-images";
    std::filesystem::create_directories(own);
    for (const auto* name : {"a.png", "b.png"})
        writeGreyPng(own / name, 8, 8, [](int, int) { return 0; });
    writeGreyPng(own / "c.png.part", 8, 8, [](int, int) { return 0; });

    const auto played = DefaultImages::playable(own, kBundled);

    ASSERT_EQ(played.size(), 3u);
    EXPECT_EQ(played.front().parent_path(), kBundled);
}

TEST(DefaultImages, AnUploadThatLooksLikeAStoredOneOnThePanelIsRefused)
{
    TempDir tmp;
    DefaultImages pictures(tmp / "default-images", kBundled, 0);
    pictures.ensureSeeded();
    const auto shape = [](int x, int) -> uint8_t { return x < 400 ? 10 : 240; };

    ASSERT_EQ(pictures.add(png(tmp, shape)), DefaultImages::AddResult::Added);
    EXPECT_EQ(pictures.add(png(tmp, shape)), DefaultImages::AddResult::Duplicate);
    EXPECT_EQ(pictures.add(png(tmp, [&](int x, int y) -> uint8_t { return shape(x, y) + 5; })),
              DefaultImages::AddResult::TooSimilar);
    EXPECT_EQ(pictures.add(std::vector<uint8_t>{'G', 'I', 'F', '8', '9', 'a'}), DefaultImages::AddResult::Invalid);
    EXPECT_EQ(pictures.list().size(), 4u);
    for (const auto& entry : std::filesystem::directory_iterator(tmp / "default-images"))
        EXPECT_NE(entry.path().extension(), ".part");
}

TEST(DefaultImages, TheSetStaysBetweenThreeAndTenPictures)
{
    TempDir tmp;
    DefaultImages pictures(tmp / "default-images", kBundled, 0);
    pictures.ensureSeeded();
    const auto bundled = pictures.list();

    EXPECT_EQ(pictures.remove(bundled.front().name), DefaultImages::RemoveResult::AtMinimum);
    for (size_t i = 0; i + 1 < kShapes.size(); ++i)
        ASSERT_EQ(pictures.add(png(tmp, kShapes[i])), DefaultImages::AddResult::Added) << "shape " << i;
    ASSERT_EQ(pictures.list().size(), DefaultImages::kMax);
    EXPECT_EQ(pictures.add(png(tmp, kShapes.back())), DefaultImages::AddResult::Full);

    EXPECT_EQ(pictures.remove(bundled.front().name), DefaultImages::RemoveResult::Removed);
    EXPECT_EQ(pictures.remove("../" + bundled.back().name), DefaultImages::RemoveResult::NotFound);
    EXPECT_EQ(pictures.list().size(), DefaultImages::kMax - 1);
}

TEST(DefaultImages, ASetWipedWhileRunningComesBackOnTheNextRequest)
{
    TempDir tmp;
    DefaultImages pictures(tmp / "default-images", kBundled, 0);
    pictures.ensureSeeded();
    const auto bundled = names(pictures.list());

    std::filesystem::remove_all(tmp / "default-images");

    EXPECT_EQ(pictures.add(png(tmp, kShapes[0])), DefaultImages::AddResult::Added);
    EXPECT_EQ(pictures.list().size(), bundled.size() + 1);
}

// Names are content hashes, so sorting by name would put an upload anywhere.
TEST(DefaultImages, ListsAndPlaysThePicturesInTheOrderTheyWereAdded)
{
    TempDir tmp;
    const auto dir = tmp / "default-images";
    DefaultImages pictures(dir, kBundled, 0);
    pictures.ensureSeeded();
    ASSERT_EQ(pictures.add(png(tmp, kShapes[0])), DefaultImages::AddResult::Added);
    ASSERT_EQ(pictures.add(png(tmp, kShapes[1])), DefaultImages::AddResult::Added);
    auto entries = pictures.list();
    const auto first = entries[3].path, second = entries[4].path;
    // Makes the later upload the older file, whatever the file system's time resolution.
    std::filesystem::last_write_time(second, std::filesystem::last_write_time(first) - std::chrono::hours(1));

    entries = pictures.list();
    const auto played = DefaultImages::playable(dir, kBundled);

    EXPECT_EQ(entries.front().path, second);
    EXPECT_EQ(entries.back().path, first);
    EXPECT_EQ(played.front(), second);
}
