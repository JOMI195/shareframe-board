#include "settings/PlaybackOrder.hpp"
#include <algorithm>
#include <unordered_set>

std::string toString(const PlaybackMode mode)
{
    switch (mode)
    {
    case PlaybackMode::Shuffle:       return "shuffle";
    case PlaybackMode::Chronological: return "chronological";
    case PlaybackMode::NewestFirst:   return "newest_first";
    case PlaybackMode::Custom:        return "custom";
    }
    return "chronological";
}

std::optional<PlaybackMode> playbackModeFromString(const std::string& s)
{
    if (s == "shuffle")       return PlaybackMode::Shuffle;
    if (s == "chronological") return PlaybackMode::Chronological;
    if (s == "newest_first")  return PlaybackMode::NewestFirst;
    if (s == "custom")        return PlaybackMode::Custom;
    return std::nullopt;
}

namespace playback
{
namespace
{
std::vector<int64_t> chronological(std::vector<Image> images)
{
    std::ranges::sort(images, [](const Image& a, const Image& b)
    {
        return a.receivedAt != b.receivedAt ? a.receivedAt < b.receivedAt : a.id < b.id;
    });
    std::vector<int64_t> ids;
    ids.reserve(images.size());
    for (const auto& img : images)
        ids.push_back(img.id);
    return ids;
}
} // namespace

std::vector<int64_t> arrange(const std::vector<Image>& images, const PlaybackMode mode,
                             const std::vector<int64_t>& customIds)
{
    auto ids = chronological(images);
    if (mode == PlaybackMode::NewestFirst)
        std::ranges::reverse(ids);
    if (mode != PlaybackMode::Custom)
        return ids;

    const std::unordered_set<int64_t> existing(ids.begin(), ids.end());
    std::vector<int64_t> ordered;
    std::unordered_set<int64_t> placed;
    for (const int64_t id : customIds)
        if (existing.contains(id) && placed.insert(id).second)
            ordered.push_back(id);
    for (const int64_t id : ids)
        if (!placed.contains(id))
            ordered.push_back(id);
    return ordered;
}

int64_t nextAfter(const std::vector<int64_t>& sequence, const int64_t currentId)
{
    if (sequence.empty())
        return 0;
    const auto it = std::ranges::find(sequence, currentId);
    if (it == sequence.end() || std::next(it) == sequence.end())
        return sequence.front();
    return *std::next(it);
}

ShuffleBag::ShuffleBag(const std::mt19937::result_type seed) : rng_(seed)
{
}

void ShuffleBag::sync(const std::vector<int64_t>& ids)
{
    const std::unordered_set<int64_t> wanted(ids.begin(), ids.end());
    std::vector<int64_t> kept;
    size_t played = 0;
    for (size_t i = 0; i < order_.size(); ++i)
    {
        if (!wanted.contains(order_[i]))
            continue;
        kept.push_back(order_[i]);
        if (i < played_)
            ++played;
    }
    order_ = std::move(kept);
    played_ = played;

    const std::unordered_set<int64_t> known(order_.begin(), order_.end());
    for (const int64_t id : ids)
    {
        if (known.contains(id))
            continue;
        std::uniform_int_distribution<size_t> pos(played_, order_.size());
        order_.insert(order_.begin() + static_cast<std::ptrdiff_t>(pos(rng_)), id);
    }
}

int64_t ShuffleBag::next()
{
    if (order_.empty())
        return 0;
    if (played_ >= order_.size())
    {
        const int64_t last = order_.back();
        std::ranges::shuffle(order_, rng_);
        if (order_.size() > 1 && order_.front() == last)
        {
            std::uniform_int_distribution<size_t> other(1, order_.size() - 1);
            std::swap(order_.front(), order_[other(rng_)]);
        }
        played_ = 0;
    }
    return order_[played_++];
}
} // namespace playback
