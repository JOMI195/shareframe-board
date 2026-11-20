#pragma once
#include "config/AppConfig.hpp"
#include "repository/ImageRepository.hpp"
#include <cstdint>
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

    /// Removes images that have expired from DB and disk; pictures without an expiry stay.
    /// Returns the ids of the removed images.
    [[nodiscard]] std::vector<int64_t> removeExpired() const;

private:
    void removeOrphaned() const;
    [[nodiscard]] bool _acceptable(int64_t sentImageId, const std::vector<uint8_t>& bytes) const;

    const AppConfig& _cfg;
    ImageRepository& _repo;
    std::filesystem::path _savePath;
};
