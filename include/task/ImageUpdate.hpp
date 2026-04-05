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

protected:
    void _run(std::stop_token st) override;

private:
    void _enqueue(Topic topic, const nlohmann::json& msg);

    void _onPicture(const nlohmann::json& msg) const;
    void _onClearImages(const nlohmann::json& msg) const;

    ImageManager& imgMgr_;
    ImageRepository& repo_;
    std::queue<std::pair<Topic, nlohmann::json>> queue_;
};
