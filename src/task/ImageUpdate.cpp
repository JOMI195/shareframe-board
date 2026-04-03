#include "task/ImageUpdate.hpp"
#include "events/Messages.hpp"
#include "events/Topic.hpp"

ImageUpdate::ImageUpdate(EventBus& bus, ImageManager& imgMgr, ImageRepository& repo)
    : Task(bus, "ImageUpdate"), imgMgr_(imgMgr), repo_(repo)
{
}

void ImageUpdate::start()
{
    bus_.subscribe<Topic::PICTURE>([this](const nlohmann::json& msg) { _enqueue(Topic::PICTURE, msg); });
    bus_.subscribe<Topic::CLEAR_IMAGES>([this](const nlohmann::json& msg) { _enqueue(Topic::CLEAR_IMAGES, msg); });
    bus_.subscribe<Topic::CLEAR_DISPLAY>([this](const nlohmann::json& msg) { _enqueue(Topic::CLEAR_DISPLAY, msg); });

    logger_->info("Starting ImageUpdate thread");
    Task::start();
}

void ImageUpdate::_enqueue(Topic topic, const nlohmann::json& msg)
{
    {
        std::lock_guard lk(mtx_);
        queue_.emplace(topic, msg);
    }
    cv_.notify_one();
}

void ImageUpdate::_run(const std::stop_token st)
{
    while (!st.stop_requested())
    {
        std::queue<std::pair<Topic, nlohmann::json>> batch;
        {
            std::unique_lock lk(mtx_);
            cv_.wait(lk, st, [this] { return !queue_.empty(); });
            if (st.stop_requested())
                return;
            std::swap(batch, queue_);
        }

        while (!batch.empty())
        {
            auto [topic, msg] = std::move(batch.front());
            batch.pop();

            switch (topic)
            {
            case Topic::PICTURE:
                _onPicture(msg);
                break;
            case Topic::CLEAR_IMAGES:
                _onClearImages(msg);
                break;
            case Topic::CLEAR_DISPLAY:
                _onClearDisplay(msg);
                break;
            default:
                break;
            }
        }
    }
}

void ImageUpdate::_onPicture(const nlohmann::json& msg) const
{
    try
    {
        // Server may send numeric fields as either numbers or strings
        auto jsonInt64 = [&](const char* key) -> int64_t
        {
            if (!msg.contains(key))
                return 0;
            const auto& v = msg[key];
            if (v.is_number())
                return v.get<int64_t>();
            if (v.is_string())
                return std::stoll(v.get<std::string>());
            return 0;
        };

        const auto sentImageId = msg.value("sent_image_id", 0);
        const auto sender = msg.value("sender", "");
        const auto base64Data = msg.value("data", "");
        const auto expiresAt = jsonInt64("expiry_unix_timestamp");

        if (sentImageId == 0 || sender.empty() || base64Data.empty())
        {
            logger_->warn("Received picture message with missing fields");
            return;
        }

        // Check if image already exists
        if (const auto existing = repo_.getByIds({sentImageId}); !existing.empty())
        {
            logger_->info("Image {} from {} already exists, skipping", sentImageId, sender);
            return;
        }

        if (auto saved = imgMgr_.saveImage(sentImageId, sender, base64Data, expiresAt))
            logger_->info("Saved image {} from {}", sentImageId, sender);
        else
            logger_->error("Failed to save image {} from {}", sentImageId, sender);
    }
    catch (const std::exception& e)
    {
        logger_->error("Error handling picture message: {}", e.what());
    }
}

void ImageUpdate::_onClearImages(const nlohmann::json& msg) const
{
    try
    {
        const auto ids = msg.value("sent_image_ids", std::vector<int64_t>{});
        if (ids.empty())
        {
            logger_->debug("Received clear_specific_sent_images with empty list");
            return;
        }

        logger_->info("Clearing {} specific images", ids.size());
        imgMgr_.removeImages(ids);
        bus_.publish<Topic::IMAGE_REMOVED>(ids);
    }
    catch (const std::exception& e)
    {
        logger_->error("Error handling clear images message: {}", e.what());
    }
}

void ImageUpdate::_onClearDisplay(const nlohmann::json& msg) const
{
    try
    {
        const auto allImages = repo_.getAll();
        if (allImages.empty())
        {
            logger_->debug("No images to clear");
            return;
        }

        std::vector<int64_t> allIds;
        allIds.reserve(allImages.size());
        for (const auto& img : allImages)
            allIds.push_back(img.id);

        logger_->info("Clearing all {} images", allIds.size());
        imgMgr_.removeImages(allIds);
        bus_.publish<Topic::IMAGE_REMOVED>(allIds);
    }
    catch (const std::exception& e)
    {
        logger_->error("Error handling clear display message: {}", e.what());
    }
}
