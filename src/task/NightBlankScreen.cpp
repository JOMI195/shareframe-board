#include "task/NightBlankScreen.hpp"

NightBlankScreen::NightBlankScreen(EventBus& bus, DisplayManager& display)
    : Task(bus, "NightBlankScreen"), display_(display)
{
}

void NightBlankScreen::_run(const std::stop_token st)
{
    if (!display_.isCleared())
    {
        logger_->info("Blanking the panel for the night");
        display_.clear(st);
    }

    std::unique_lock lk(mtx_);
    cv_.wait(lk, st, [] { return false; });
}
