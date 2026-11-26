#include "image/PanelSimilarity.hpp"
#include "display/DisplayManager.hpp"
#include <bit>
#include <cmath>
#include <vector>

namespace
{
constexpr int kBlock = 16;
constexpr int kBlocksX = DisplayManager::WIDTH / kBlock;
constexpr int kBlocksY = DisplayManager::HEIGHT / kBlock;
constexpr int kBytesPerBlockRow = kBlock / 8;

std::vector<int> blackPerBlock(const std::span<const uint8_t> buffer)
{
    std::vector<int> blocks(kBlocksX * kBlocksY, 0);
    constexpr int bytesPerRow = DisplayManager::WIDTH / 8;
    for (int y = 0; y < DisplayManager::HEIGHT; ++y)
        for (int byte = 0; byte < bytesPerRow; ++byte)
        {
            const auto white = std::popcount(static_cast<unsigned>(buffer[y * bytesPerRow + byte]));
            blocks[(y / kBlock) * kBlocksX + byte / kBytesPerBlockRow] += 8 - white;
        }
    return blocks;
}
} // namespace

namespace panel_similarity
{
double difference(const std::span<const uint8_t> a, const std::span<const uint8_t> b)
{
    if (a.size() != DisplayManager::BUFFER_SIZE || b.size() != DisplayManager::BUFFER_SIZE)
        return 1.0;
    const auto blocksA = blackPerBlock(a);
    const auto blocksB = blackPerBlock(b);
    double sum = 0;
    for (size_t i = 0; i < blocksA.size(); ++i)
        sum += std::abs(blocksA[i] - blocksB[i]);
    return sum / (static_cast<double>(blocksA.size()) * kBlock * kBlock);
}
} // namespace panel_similarity
