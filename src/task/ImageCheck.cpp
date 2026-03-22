#include "task/ImageCheck.hpp"
#include "events/Topic.hpp"
#include <nlohmann/json.hpp>

ImageCheck::ImageCheck(EventBus& bus, const AppConfig& cfg, ImageRepository& repo)
    : PeriodicTask(bus, cfg, "ImageCheck"), repo_(repo)
{
}

int ImageCheck::intervalSecs() const
{
    return cfg_.imageCheck.intervalSecs;
}

void ImageCheck::execute()
{
    _checkExpiry();
    _checkMissing();
}

void ImageCheck::_checkExpiry() const
{
    const auto images = repo_.getAll();
    if (images.empty())
    {
        logger_->debug("No images to check expiry for");
        return;
    }

    nlohmann::json imageArray = nlohmann::json::array();
    for (const auto& img : images)
    {
        imageArray.push_back({
            {"sent_image_id", img.id},
            {"expires_at", img.expiresAt}
        });
    }

    const nlohmann::json payload = {
        {"type", "check_sent_images_expiry"},
        {"user_frame_images", imageArray}
    };

    logger_->info("Checking expiry for {} images", images.size());
    bus_.publish<Topic::WS_SEND>(payload);
}

void ImageCheck::_checkMissing() const
{
    const auto ids = repo_.getAllIds();
    if (ids.empty())
    {
        logger_->debug("No images to check for missing");
        return;
    }

    const nlohmann::json payload = {
        {"type", "check_missing_images"},
        {"sent_image_ids", ids}
    };

    logger_->info("Checking for missing images, {} image IDs sent", ids.size());
    bus_.publish<Topic::WS_SEND>(payload);
}
