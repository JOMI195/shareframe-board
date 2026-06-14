#pragma once
#include "image/ImageManager.hpp"
#include "task/PeriodicTask.hpp"

// Periodically removes locally-expired images from DB and disk, notifying the
// display so a currently-shown expired image is dropped.
class ExpiredImageCleanup : public PeriodicTask
{
public:
    ExpiredImageCleanup(EventBus& bus, const AppConfig& cfg, ImageManager& imageManager);

protected:
    [[nodiscard]] int intervalSecs() const override;
    void execute() override;

private:
    ImageManager& imageManager_;
};
