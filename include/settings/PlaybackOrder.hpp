#pragma once
#include "repository/entity/Image.hpp"
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <vector>

enum class PlaybackMode
{
    Shuffle,
    Chronological,
    NewestFirst,
    Custom
};

std::string toString(PlaybackMode mode);
std::optional<PlaybackMode> playbackModeFromString(const std::string& s);

namespace playback
{
/// Play order of `images` for every mode but Shuffle. Custom keeps the ids of
/// `customIds` that still exist and appends the rest chronologically.
std::vector<int64_t> arrange(const std::vector<Image>& images, PlaybackMode mode,
                             const std::vector<int64_t>& customIds = {});

/// The id after `currentId`, wrapping around; the first one when `currentId`
/// is not in `sequence`. 0 for an empty sequence.
int64_t nextAfter(const std::vector<int64_t>& sequence, int64_t currentId);

/// Plays every picture once per cycle in random order. Pictures that arrive
/// mid-cycle still get their turn in it, and a new cycle never starts with the
/// picture that ended the last one.
class ShuffleBag
{
public:
    explicit ShuffleBag(std::mt19937::result_type seed = std::random_device{}());

    /// Drops ids that are gone and adds new ones to the part not yet played.
    void sync(const std::vector<int64_t>& ids);

    /// 0 when the bag is empty.
    int64_t next();

    [[nodiscard]] const std::vector<int64_t>& order() const { return order_; }

private:
    std::vector<int64_t> order_;
    size_t played_ = 0;
    std::mt19937 rng_;
};
} // namespace playback
