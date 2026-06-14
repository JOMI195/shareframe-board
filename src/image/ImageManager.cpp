#include "image/ImageManager.hpp"
#include <openssl/evp.h>
#include <spdlog/spdlog.h>
#include <ctime>
#include <fstream>
#include <unordered_set>

static std::vector<uint8_t> decodeBase64(const std::string& encoded)
{
    std::vector<uint8_t> out(encoded.size() * 3 / 4);
    const int len = EVP_DecodeBlock(out.data(),
                                    reinterpret_cast<const unsigned char*>(encoded.data()),
                                    static_cast<int>(encoded.size()));
    if (len < 0)
        return {};

    // EVP_DecodeBlock doesn't account for padding — trim trailing zeros from '=' chars
    size_t padding = 0;
    if (encoded.size() >= 2 && encoded.back() == '=') ++padding;
    if (encoded.size() >= 2 && encoded[encoded.size() - 2] == '=') ++padding;
    out.resize(static_cast<size_t>(len) - padding);
    return out;
}

ImageManager::ImageManager(const AppConfig& cfg, ImageRepository& repo)
    : _cfg(cfg), _repo(repo), _savePath(cfg.image.imageSavePath)
{
    std::filesystem::create_directories(_savePath);
    removeOrphaned();
    spdlog::info("ImageManager initialized, save path: {}", _savePath.string());
}

std::optional<Image> ImageManager::saveImage(
    int64_t sentImageId,
    const std::string& sender,
    const std::string& base64Data,
    int64_t expiresAt) const
{
    try
    {
        auto decoded = decodeBase64(base64Data);
        if (decoded.empty())
        {
            spdlog::error("Failed to decode base64 image data for sent_image_id={}", sentImageId);
            return std::nullopt;
        }

        auto filename = fmt::format("{}_{}_{}_{}.jpg",
                                    sender, std::time(nullptr), expiresAt, sentImageId);
        auto filepath = _savePath / filename;

        std::ofstream file(filepath, std::ios::binary);
        if (!file)
        {
            spdlog::error("Failed to open file for writing: {}", filepath.string());
            return std::nullopt;
        }
        file.write(reinterpret_cast<const char*>(decoded.data()),
                   static_cast<std::streamsize>(decoded.size()));
        file.close();

        Image image{sentImageId, sender, filepath, expiresAt};
        _repo.save(image);
        spdlog::info("Saved image: {}", filepath.string());
        return image;
    }
    catch (const std::exception& e)
    {
        spdlog::error("Error saving image sent_image_id={}: {}", sentImageId, e.what());
        return std::nullopt;
    }
}

void ImageManager::removeImages(const std::vector<int64_t>& ids) const
{
    if (ids.empty())
        return;

    for (const auto images = _repo.getByIds(ids); const auto& img : images)
    {
        std::error_code ec;
        std::filesystem::remove(img.imagePath, ec);
        if (ec)
            spdlog::warn("Failed to delete image file {}: {}", img.imagePath.string(), ec.message());
    }
    _repo.removeByIds(ids);
}

std::vector<int64_t> ImageManager::removeExpired() const
{
    const auto now = std::time(nullptr);
    const auto allImages = _repo.getAll();
    std::vector<int64_t> expiredIds;

    for (const auto& img : allImages)
    {
        if (img.expiresAt < now)
        {
            std::error_code ec;
            std::filesystem::remove(img.imagePath, ec);
            if (ec)
                spdlog::warn("Failed to delete expired image file {}: {}",
                             img.imagePath.string(), ec.message());
            expiredIds.push_back(img.id);
        }
    }

    if (!expiredIds.empty())
    {
        _repo.removeByIds(expiredIds);
        spdlog::info("Removed {} expired images", expiredIds.size());
    }

    return expiredIds;
}

void ImageManager::removeOrphaned() const
{
    std::unordered_set<std::string> knownPaths;
    for (const auto& img : _repo.getAll())
        knownPaths.insert(img.imagePath.string());

    int removed = 0;
    for (const auto& entry : std::filesystem::directory_iterator(_savePath))
    {
        if (!entry.is_regular_file())
            continue;

        if (knownPaths.contains(entry.path().string()))
            continue;

        std::error_code ec;
        std::filesystem::remove(entry.path(), ec);
        if (ec)
            spdlog::warn("Failed to delete orphaned image {}: {}", entry.path().string(), ec.message());
        else
            ++removed;
    }

    if (removed > 0)
        spdlog::info("Removed {} orphaned image(s) from disk", removed);
}
