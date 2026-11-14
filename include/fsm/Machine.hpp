#pragma once
#include <atomic>
#include <memory>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string_view>

namespace fsm
{
struct ForbiddenMove : std::logic_error
{
    using std::logic_error::logic_error;
};

/// Holds the state of one machine. The state type brings its rules as two free functions
/// next to the enum: which moves are allowed, and a name per state. One thread moves it,
/// any reads it.
template <typename S>
class Machine
{
public:
    Machine(const S initial, std::shared_ptr<spdlog::logger> logger,
            const spdlog::level::level_enum level = spdlog::level::info)
        : state_(initial), logger_(std::move(logger)), level_(level)
    {
    }

    [[nodiscard]] S state() const { return state_.load(); }

    /// Throws on a move the rules forbid and stays put; the current state as target is no move.
    void moveTo(const S next, const std::string_view why = {})
    {
        const S current = state_.load();
        if (next == current)
            return;
        if (!canMove(current, next))
        {
            const auto message = fmt::format("Forbidden move {} -> {}", name(current), name(next));
            logger_->error(message);
            throw ForbiddenMove(message);
        }
        if (why.empty())
            logger_->log(level_, "State {} -> {}", name(current), name(next));
        else
            logger_->log(level_, "State {} -> {} ({})", name(current), name(next), why);
        state_.store(next);
    }

private:
    std::atomic<S> state_;
    std::shared_ptr<spdlog::logger> logger_;
    spdlog::level::level_enum level_;
};
} // namespace fsm
