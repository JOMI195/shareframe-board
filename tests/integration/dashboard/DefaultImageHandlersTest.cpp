#include "dashboard/DefaultImageHandlers.hpp"
#include "support/Http.hpp"
#include "support/PngWriter.hpp"
#include "support/TempDir.hpp"
#include <fstream>
#include <gtest/gtest.h>

namespace
{
std::string pngBody(const TempDir& tmp)
{
    writeGreyPng(tmp / "upload.png", 800, 480, [](int x, int) -> uint8_t { return x < 400 ? 0 : 255; });
    std::ifstream f(tmp / "upload.png", std::ios::binary);
    return {std::istreambuf_iterator<char>(f), {}};
}
} // namespace

TEST(DefaultImageHandlers, AnUploadedPictureCanBeFetchedAndRemovedButNoOtherFile)
{
    TempDir tmp;
    DefaultImages images(tmp / "default-images", std::filesystem::path(SHAREFRAME_ASSETS_DIR) / "image/default", 0);
    images.ensureSeeded();
    DefaultImageHandlers handlers(images);
    std::ofstream(tmp / "secret.txt") << "not a picture";

    const auto upload = std::make_shared<ix::HttpRequest>("/api/frame/default-images", "POST", "HTTP/1.1", pngBody(tmp));
    const auto added = result(handlers.handleUpload(upload));
    ASSERT_EQ(added.status, 201) << added.body.dump();
    ASSERT_EQ(added.body["data"]["images"].size(), 4u);
    EXPECT_EQ(result(handlers.handleUpload(upload)).body["message"], "Picture already there");

    std::string name;
    for (const auto& image : added.body["data"]["images"])
        if (image["width"] == 800)
            name = image["name"];
    ASSERT_FALSE(name.empty());
    const auto file = handlers.handleFile(httpRequest("GET", "/"), {{"name", name}});
    EXPECT_EQ(file->statusCode, 200);
    EXPECT_EQ(file->headers["Content-Type"], "image/png");
    EXPECT_EQ(handlers.handleFile(httpRequest("GET", "/"), {{"name", "../secret.txt"}})->statusCode, 400);

    EXPECT_EQ(result(handlers.handleRemove(httpRequest("POST", "/", {{"name", name}}))).status, 200);
    const auto atMinimum = result(handlers.handleRemove(
        httpRequest("POST", "/", {{"name", images.list().front().name}})));
    EXPECT_EQ(atMinimum.status, 409);
    EXPECT_EQ(atMinimum.body["message"], "Too few default images");
}
