#include "task/PeriodicTask.hpp"

PeriodicTask::PeriodicTask(EventBus& bus, const AppConfig& cfg, std::string name)
    : bus_(bus), cfg_(cfg), logger_(spdlog::default_logger()->clone(name)), name_(std::move(name))
{
}

PeriodicTask::~PeriodicTask()
{
    stop();
}

void PeriodicTask::start()
{
    logger_->info("Starting {} thread (interval={}s)", name_, intervalSecs());
    thread_ = std::jthread([this](std::stop_token st) { _run(std::move(st)); });
}

void PeriodicTask::stop()
{
    if (!thread_.joinable())
        return;

    logger_->info("Stopping {} thread", name_);
    thread_.request_stop();
    cv_.notify_all();
    thread_.join();
    logger_->info("{} thread stopped", name_);
}

void PeriodicTask::_run(std::stop_token st)
{
    while (!st.stop_requested())
    {
        execute();

        std::unique_lock lock(mtx_);
        cv_.wait_for(lock, std::chrono::seconds(intervalSecs()), [&st]
        {
            return st.stop_requested();
        });
    }
}
