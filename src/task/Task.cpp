#include "task/Task.hpp"

Task::Task(EventBus& bus, std::string name)
    : bus_(bus), logger_(spdlog::default_logger()->clone(name)), name_(std::move(name))
{
}

Task::~Task()
{
    stop();
}

void Task::start()
{
    thread_ = std::jthread([this](std::stop_token st) { _run(std::move(st)); });
}

void Task::stop()
{
    if (!thread_.joinable())
        return;

    logger_->info("Stopping {} thread", name_);
    thread_.request_stop();
    cv_.notify_all();
    thread_.join();
    logger_->info("{} thread stopped", name_);
}
