#include "task/ExpiredImageCleanup.hpp"
#include "events/Topic.hpp"

ExpiredImageCleanup::ExpiredImageCleanup(EventBus& bus, const AppConfig& cfg,
                                         ImageManager& imageManager)
    : PeriodicTask(bus, cfg, "ExpiredImageCleanup"), imageManager_(imageManager)
{
}

int ExpiredImageCleanup::intervalSecs() const
{
    return cfg_.expiryCleanup.intervalSecs;
}

void ExpiredImageCleanup::execute()
{
    if (const auto removed = imageManager_.removeExpired(); !removed.empty())
        bus_.publish<Topic::IMAGE_REMOVED>(removed);
}
