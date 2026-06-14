#pragma once
#include "repository/ImageRepository.hpp"
#include "task/Task.hpp"

// Sends a "check_missing_images" reconciliation to the server once each time the
// WebSocket (re)connects, catching up on images missed while disconnected.
class MissingImageCheck : public Task
{
public:
    MissingImageCheck(EventBus& bus, ImageRepository& repo);

    void start() override;

protected:
    void _run(std::stop_token st) override;

private:
    void _checkMissing() const;

    ImageRepository& repo_;
    bool checkRequested_ = false;
};
