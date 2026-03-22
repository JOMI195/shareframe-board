#pragma once
#include "config/AppConfig.hpp"
#include "repository/ImageRepository.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

class ImageManager
{
public:
    explicit ImageManager(const AppConfig& cfg, ImageRepository& repo);

    /// Decodes base64 image data, saves to disk, persists in DB.
    /// Returns the saved Image on success, nullopt on failure.
    [[nodiscard]] std::optional<Image> saveImage(int64_t sentImageId,
                                                 const std::string& sender,
                                                 const std::string& base64Data,
                                                 int64_t expiresAt) const;

    /// Removes specific images from DB and disk.
    void removeImages(const std::vector<int64_t>& ids) const;

    /// Removes images that have expired (expiresAt < now) from DB and disk.
    void removeExpired() const;

private:
    void removeOrphaned() const;

    const AppConfig& _cfg;
    ImageRepository& _repo;
    std::filesystem::path _savePath;
};
