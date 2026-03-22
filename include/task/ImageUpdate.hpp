#pragma once
#include "image/ImageManager.hpp"
#include "task/Task.hpp"
#include <nlohmann/json.hpp>
#include <queue>

class ImageUpdate : public Task
{
public:
    ImageUpdate(EventBus& bus, ImageManager& imgMgr, ImageRepository& repo);

    void start() override;

private:
    void _enqueue(const std::string& topic, const nlohmann::json& msg);
    void _run(std::stop_token st) override;

    void _onPicture(const nlohmann::json& msg) const;
    void _onClearImages(const nlohmann::json& msg) const;
    void _onClearDisplay(const nlohmann::json& msg) const;

    ImageManager& imgMgr_;
    ImageRepository& repo_;
    std::queue<std::pair<std::string, nlohmann::json>> queue_;
};
