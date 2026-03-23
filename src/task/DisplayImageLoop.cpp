#include "task/DisplayImageLoop.hpp"
#include "events/Messages.hpp"
#include <algorithm>
#include <filesystem>

DisplayImageLoop::DisplayImageLoop(EventBus& bus, const AppConfig& cfg,
                                   ImageRepository& repo, DisplayManager& display)
    : Task(bus, "DisplayImageLoop"), cfg_(cfg), repo_(repo), display_(display)
{
}

void DisplayImageLoop::start()
{
    bus_.subscribe<Topic::IMAGE_REMOVED>(
        [this](const std::vector<int64_t>& ids) { _onImageRemoved(ids); });

    logger_->info("Starting DisplayImageLoop (interval={}s, minRefresh={}s)",
                  cfg_.display.intervalSecs, cfg_.display.minRefreshSecs);
    Task::start();
}

void DisplayImageLoop::_onImageRemoved(const std::vector<int64_t>& removedIds)
{
    std::lock_guard lk(mtx_);
    if (std::ranges::find(removedIds, currentImageId_) != removedIds.end())
    {
        logger_->info("Current image {} was removed, skipping", currentImageId_);
        skipCurrent_ = true;
        cv_.notify_one();
    }
}

std::vector<std::filesystem::path> DisplayImageLoop::_loadDefaultImages() const
{
    std::vector<std::filesystem::path> paths;
    const auto& dir = cfg_.display.defaultImagesPath;

    if (!std::filesystem::exists(dir))
    {
        logger_->warn("Default images directory not found: {}", dir);
        return paths;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir))
    {
        if (entry.is_regular_file())
            paths.push_back(entry.path());
    }

    std::ranges::sort(paths);
    logger_->info("Loaded {} default images from {}", paths.size(), dir);
    return paths;
}

void DisplayImageLoop::_run(const std::stop_token st)
{
    while (!st.stop_requested())
    {
        auto ids = repo_.getAllIds();

        if (ids.empty())
        {
            // Show default images when no user images exist
            auto defaultImages = _loadDefaultImages();
            if (defaultImages.empty())
            {
                logger_->debug("No images available, waiting");
                std::unique_lock lk(mtx_);
                cv_.wait_for(lk, std::chrono::seconds(cfg_.display.intervalSecs),
                             [&st, this] { return st.stop_requested() || skipCurrent_; });
                continue;
            }

            while (!st.stop_requested())
            {
                // Check if user images appeared
                if (!repo_.getAllIds().empty())
                {
                    logger_->info("User images available, switching from defaults");
                    defaultImageIdx_ = 0;
                    break;
                }

                if (defaultImageIdx_ >= defaultImages.size())
                    defaultImageIdx_ = 0;

                const auto& path = defaultImages[defaultImageIdx_];
                if (display_.displayImage(path))
                    logger_->info("Displaying default image ({})", path.string());
                else
                    logger_->error("Failed to display default image: {}", path.string());

                ++defaultImageIdx_;

                std::unique_lock lk(mtx_);
                skipCurrent_ = false;
                cv_.wait_for(lk, std::chrono::seconds(cfg_.display.intervalSecs),
                             [&st, this] { return st.stop_requested() || skipCurrent_; });
            }
            continue;
        }

        // Find next image ID > currentImageId_, or wrap to lowest
        std::ranges::sort(ids);
        auto it = std::ranges::upper_bound(ids, currentImageId_);
        int64_t nextId = (it != ids.end()) ? *it : ids.front();

        // Fetch the image to get its file path
        auto images = repo_.getByIds({nextId});
        if (images.empty())
        {
            logger_->warn("Image {} not found in repository, skipping", nextId);
            currentImageId_ = nextId;
            continue;
        }

        const auto& image = images.front();
        if (!std::filesystem::exists(image.imagePath))
        {
            logger_->warn("Image file missing: {}, skipping", image.imagePath.string());
            currentImageId_ = nextId;
            continue;
        }

        // Display the image (DisplayManager enforces min refresh)
        if (display_.displayImage(image.imagePath))
            logger_->info("Displaying image {} ({})", nextId, image.imagePath.string());
        else
            logger_->error("Failed to display image {}", nextId);

        currentImageId_ = nextId;

        // Wait for interval or skip signal
        {
            std::unique_lock lk(mtx_);
            skipCurrent_ = false;
            cv_.wait_for(lk, std::chrono::seconds(cfg_.display.intervalSecs),
                         [&st, this] { return st.stop_requested() || skipCurrent_; });
        }
    }
}
