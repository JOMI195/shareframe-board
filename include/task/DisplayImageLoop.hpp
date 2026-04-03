#pragma once
#include "display/DisplayManager.hpp"
#include "repository/ImageRepository.hpp"
#include "task/Task.hpp"
#include <filesystem>
#include <vector>

class RuntimeSettings;

class DisplayImageLoop : public Task
{
public:
    DisplayImageLoop(EventBus& bus, const AppConfig& cfg,
                     ImageRepository& repo, DisplayManager& display,
                     RuntimeSettings& settings);

    void start() override;

protected:
    void _run(std::stop_token st) override;

private:
    void _onImageRemoved(const std::vector<int64_t>& removedIds);
    void _onSkipImage();
    void _onUpdateInterval(const UpdateDisplayIntervalEvent& evt) const;
    [[nodiscard]] std::vector<std::filesystem::path> _loadDefaultImages() const;

    const AppConfig& cfg_;
    ImageRepository& repo_;
    DisplayManager& display_;
    RuntimeSettings& settings_;
    int64_t currentImageId_ = 0;
    bool skipCurrent_ = false;
    size_t defaultImageIdx_ = 0;
};
