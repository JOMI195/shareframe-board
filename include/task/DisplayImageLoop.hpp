#pragma once
#include "display/DisplayManager.hpp"
#include "repository/ImageRepository.hpp"
#include "task/Task.hpp"
#include <chrono>
#include <filesystem>
#include <vector>

class DisplayImageLoop : public Task
{
public:
    DisplayImageLoop(EventBus& bus, const AppConfig& cfg,
                     ImageRepository& repo, DisplayManager& display);

    void start() override;

private:
    void _run(std::stop_token st) override;
    void _onImageRemoved(const std::vector<int64_t>& removedIds);
    [[nodiscard]] std::vector<std::filesystem::path> _loadDefaultImages() const;

    const AppConfig& cfg_;
    ImageRepository& repo_;
    DisplayManager& display_;
    int64_t currentImageId_ = 0;
    bool skipCurrent_ = false;
    size_t defaultImageIdx_ = 0;
};
