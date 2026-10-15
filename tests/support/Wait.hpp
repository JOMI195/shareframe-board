#pragma once
#include <chrono>
#include <thread>

template <typename Pred>
bool waitFor(Pred&& pred, std::chrono::milliseconds timeout = std::chrono::milliseconds{3000})
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (pred())
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
    return pred();
}
