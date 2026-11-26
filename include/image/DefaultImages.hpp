#pragma once
#include <cstdint>
#include <filesystem>
#include <map>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <vector>

/// The pictures the frame shows while it has none of its own. Kept as files named after their content,
/// in a directory the owner may change; the bundled set is the factory state.
class DefaultImages
{
public:
    static constexpr size_t kMin = 3;
    static constexpr size_t kMax = 10;

    enum class AddResult
    {
        Added,
        Invalid,
        Duplicate,
        TooSimilar,
        Full,
        NoSpace,
    };

    enum class RemoveResult
    {
        Removed,
        NotFound,
        AtMinimum,
    };

    struct Entry
    {
        std::string name;
        std::filesystem::path path;
        int width = 0;
        int height = 0;
        std::uintmax_t sizeBytes = 0;
    };

    DefaultImages(std::filesystem::path dir, std::filesystem::path bundledDir, int minFreeMb);

    /// Restores the bundled set when the directory is missing or holds too few pictures.
    /// Every other call does the same first, so a set wiped while running heals itself.
    void ensureSeeded();

    /// Oldest first.
    [[nodiscard]] std::vector<Entry> list();
    [[nodiscard]] std::optional<std::filesystem::path> find(const std::string& name) const;
    [[nodiscard]] AddResult add(std::span<const uint8_t> bytes);
    [[nodiscard]] RemoveResult remove(const std::string& name);

    /// The only names that ever reach the filesystem.
    [[nodiscard]] static bool isValidName(const std::string& name);

    /// What the slideshow plays: the pictures in `dir`, or the bundled ones while `dir` holds fewer than kMin.
    [[nodiscard]] static std::vector<std::filesystem::path> playable(const std::filesystem::path& dir,
                                                                     const std::filesystem::path& bundledDir);

private:
    [[nodiscard]] static std::vector<std::filesystem::path> _pictures(const std::filesystem::path& dir);
    void _healLocked();
    /// The bundled pictures land before the others go, so the frame never runs short.
    void _restoreBundledLocked();
    [[nodiscard]] AddResult _addLocked(std::span<const uint8_t> bytes);
    /// The panel buffer of a stored picture, rendered once.
    [[nodiscard]] const std::vector<uint8_t>& _rendered(const std::filesystem::path& path);

    const std::filesystem::path dir_;
    const std::filesystem::path bundledDir_;
    const int minFreeMb_;
    mutable std::mutex mtx_;
    std::map<std::string, std::vector<uint8_t>> renderCache_;
};
