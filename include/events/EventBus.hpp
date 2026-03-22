#pragma once
#include "events/Messages.hpp"
#include <any>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <stop_token>
#include <unordered_map>
#include <vector>

class EventBus {
public:
    template <Topic T>
    void subscribe(std::function<void(const TopicMessage_t<T>&)> handler)
    {
        handlers_[static_cast<int>(T)].push_back(
            [h = std::move(handler)](const std::any& a) {
                h(std::any_cast<const TopicMessage_t<T>&>(a));
            }
        );
    }

    template <Topic T>
    void publish(TopicMessage_t<T> msg)
    {
        {
            std::lock_guard lk(mtx_);
            queue_.emplace_back(static_cast<int>(T), std::any(std::move(msg)));
        }
        cv_.notify_one();
    }

    void waitAndProcess(std::stop_token token)
    {
        std::vector<std::pair<int, std::any>> batch;
        {
            std::unique_lock lk(mtx_);
            cv_.wait(lk, token, [this] { return !queue_.empty(); });
            if (token.stop_requested())
                return;
            batch.swap(queue_);
        }
        for (auto& [topicId, payload] : batch) {
            if (auto it = handlers_.find(topicId); it != handlers_.end()) {
                for (auto& handler : it->second) {
                    handler(payload);
                }
            }
        }
    }

private:
    using ErasedHandler = std::function<void(const std::any&)>;

    std::mutex mtx_;
    std::condition_variable_any cv_;
    std::vector<std::pair<int, std::any>> queue_;
    std::unordered_map<int, std::vector<ErasedHandler>> handlers_;
};
