#pragma once
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

/// Formats shared with shareframe-hardware. The producer owns the golden: the hardware
/// repo's goldens are read from its checkout, this repo's live in tests/contract/golden.
namespace golden
{
inline std::string read(const std::filesystem::path& path)
{
    std::ifstream f(path);
    return {std::istreambuf_iterator<char>(f), {}};
}

/// A golden shareframe-hardware's scripts produce; SHAREFRAME_HARDWARE_DIR overrides the
/// sibling checkout.
inline std::filesystem::path hardware(const std::string& name)
{
    const char* env = std::getenv("SHAREFRAME_HARDWARE_DIR");
    const std::filesystem::path repo = env && *env ? env : SHAREFRAME_HARDWARE_DIR;
    auto path = repo / "tests/contract/golden" / name;
    if (!std::filesystem::exists(path))
        throw std::runtime_error("missing " + path.string() +
                                 ": these tests need the shareframe-hardware checkout (SHAREFRAME_HARDWARE_DIR)");
    return path;
}

/// What this repo writes for shareframe-hardware to read must equal its golden;
/// SHAREFRAME_UPDATE_GOLDEN=1 rewrites the golden instead.
inline void expectBoard(const std::string& name, const std::string& actual)
{
    const auto path = std::filesystem::path(SHAREFRAME_BOARD_GOLDEN_DIR) / name;
    if (const char* update = std::getenv("SHAREFRAME_UPDATE_GOLDEN"); update && std::string(update) == "1")
    {
        std::ofstream(path) << actual;
        return;
    }
    EXPECT_EQ(read(path), actual) << name << " differs from its golden, which shareframe-hardware's scripts read";
}
} // namespace golden
