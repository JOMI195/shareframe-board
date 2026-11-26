#include "image/DefaultImages.hpp"
#include "display/PhotoRenderer.hpp"
#include "image/PanelSimilarity.hpp"
#include "image/PictureCheck.hpp"
#include "util/FileHash.hpp"
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>

namespace
{
const std::shared_ptr<spdlog::logger>& logger()
{
    static const auto instance = spdlog::default_logger()->clone("DefaultImages");
    return instance;
}

bool hasPictureExtension(const std::filesystem::path& path)
{
    auto ext = path.extension().string();
    std::ranges::transform(ext, ext.begin(), [](const unsigned char c) { return std::tolower(c); });
    return ext == ".jpg" || ext == ".jpeg" || ext == ".png";
}

std::optional<std::vector<uint8_t>> readBytes(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return std::nullopt;
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {});
}

bool writeFile(const std::filesystem::path& path, const std::span<const uint8_t> bytes)
{
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    file.close();
    if (file)
        return true;
    std::error_code ec;
    std::filesystem::remove(path, ec);
    return false;
}

// Named .part until complete, so the slideshow never picks up half a picture.
std::filesystem::path partOf(const std::filesystem::path& target)
{
    return target.string() + ".part";
}

bool writeAtomically(const std::filesystem::path& target, const std::span<const uint8_t> bytes)
{
    const auto part = partOf(target);
    if (!writeFile(part, bytes))
        return false;
    std::error_code ec;
    std::filesystem::rename(part, target, ec);
    if (ec)
        std::filesystem::remove(part, ec);
    return !ec;
}

std::string_view describe(const DefaultImages::AddResult result)
{
    using enum DefaultImages::AddResult;
    switch (result)
    {
    case Added: return "added";
    case Invalid: return "not a usable picture";
    case Duplicate: return "already there";
    case TooSimilar: return "too similar to another one";
    case Full: return "the set is full";
    case NoSpace: return "no space to store it";
    }
    return "unknown";
}

std::string nameFor(const std::span<const uint8_t> bytes)
{
    const auto hash = FileHash::sha256({reinterpret_cast<const char*>(bytes.data()), bytes.size()});
    return hash.empty() ? "" : hash + std::string(picture::extension(bytes));
}
} // namespace

DefaultImages::DefaultImages(std::filesystem::path dir, std::filesystem::path bundledDir, const int minFreeMb)
    : dir_(std::move(dir)), bundledDir_(std::move(bundledDir)), minFreeMb_(minFreeMb)
{
}

bool DefaultImages::isValidName(const std::string& name)
{
    constexpr size_t hashLength = 64;
    if (name.size() != hashLength + 4)
        return false;
    const auto ext = std::string_view(name).substr(hashLength);
    if (ext != ".jpg" && ext != ".png")
        return false;
    return std::ranges::all_of(std::string_view(name).substr(0, hashLength),
                               [](const char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); });
}

std::vector<std::filesystem::path> DefaultImages::_pictures(const std::filesystem::path& dir)
{
    std::vector<std::filesystem::path> paths;
    std::error_code ec;
    for (std::filesystem::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec))
        if (it->is_regular_file() && hasPictureExtension(it->path()))
            paths.push_back(it->path());
    // Oldest first, so an upload joins at the end instead of wherever its hash name sorts.
    const auto written = [](const std::filesystem::path& path)
    {
        std::error_code ec;
        return std::filesystem::last_write_time(path, ec);
    };
    std::ranges::sort(paths, [&](const auto& a, const auto& b)
    {
        const auto ta = written(a), tb = written(b);
        return ta != tb ? ta < tb : a < b;
    });
    return paths;
}

std::vector<std::filesystem::path> DefaultImages::playable(const std::filesystem::path& dir,
                                                              const std::filesystem::path& bundledDir)
{
    if (auto own = _pictures(dir); own.size() >= kMin)
        return own;
    return _pictures(bundledDir);
}

void DefaultImages::ensureSeeded()
{
    std::lock_guard lk(mtx_);
    _healLocked();
}

void DefaultImages::_healLocked()
{
    std::error_code ec;
    std::filesystem::create_directories(dir_, ec);
    if (ec)
    {
        logger()->error("Cannot create {}: {}", dir_.string(), ec.message());
        return;
    }
    std::vector<std::filesystem::path> leftovers;
    for (std::filesystem::directory_iterator it(dir_, ec), end; !ec && it != end; it.increment(ec))
        if (it->path().extension() == ".part")
            leftovers.push_back(it->path());
    for (const auto& part : leftovers)
        std::filesystem::remove(part, ec);

    if (const auto count = _pictures(dir_).size(); count < kMin)
    {
        logger()->info("{} default image(s) in {}, restoring the bundled set", count, dir_.string());
        _restoreBundledLocked();
    }
}

std::vector<DefaultImages::Entry> DefaultImages::list()
{
    std::lock_guard lk(mtx_);
    _healLocked();
    std::vector<Entry> entries;
    for (const auto& path : _pictures(dir_))
    {
        Entry entry{path.filename().string(), path};
        int comp = 0;
        if (!stbi_info(path.c_str(), &entry.width, &entry.height, &comp))
            entry.width = entry.height = 0;
        std::error_code ec;
        entry.sizeBytes = std::filesystem::file_size(path, ec);
        if (ec)
            entry.sizeBytes = 0;
        entries.push_back(std::move(entry));
    }
    return entries;
}

std::optional<std::filesystem::path> DefaultImages::find(const std::string& name) const
{
    if (!isValidName(name))
        return std::nullopt;
    auto path = dir_ / name;
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec))
        return std::nullopt;
    return path;
}

DefaultImages::AddResult DefaultImages::add(const std::span<const uint8_t> bytes)
{
    std::lock_guard lk(mtx_);
    _healLocked();
    const auto result = _addLocked(bytes);
    if (result != AddResult::Added)
        logger()->info("Default image refused ({} bytes): {}", bytes.size(), describe(result));
    return result;
}

DefaultImages::AddResult DefaultImages::_addLocked(const std::span<const uint8_t> bytes)
{
    const auto existing = _pictures(dir_);
    if (existing.size() >= kMax)
        return AddResult::Full;

    switch (const auto verdict = picture::check(bytes, dir_, minFreeMb_))
    {
    case picture::Verdict::Ok: break;
    case picture::Verdict::NoSpace: return AddResult::NoSpace;
    default:
        logger()->warn("Uploaded default image is {}", picture::describe(verdict));
        return AddResult::Invalid;
    }

    const auto name = nameFor(bytes);
    if (name.empty())
        return AddResult::Invalid;
    const auto target = dir_ / name;
    if (std::filesystem::exists(target))
        return AddResult::Duplicate;

    const auto part = partOf(target);
    if (!writeFile(part, bytes))
    {
        logger()->error("Failed to write {}", part.string());
        return AddResult::NoSpace;
    }
    auto rendered = PhotoRenderer::render(part);
    std::error_code ec;
    if (rendered.empty())
    {
        std::filesystem::remove(part, ec);
        return AddResult::Invalid;
    }
    for (const auto& path : existing)
    {
        if (const auto diff = panel_similarity::difference(_rendered(path), rendered);
            diff < panel_similarity::kMinDifference)
        {
            logger()->info("Uploaded default image looks like {} on the panel (difference {:.3f})",
                           path.filename().string(), diff);
            std::filesystem::remove(part, ec);
            return AddResult::TooSimilar;
        }
    }
    std::filesystem::rename(part, target, ec);
    if (ec)
    {
        logger()->error("Failed to store {}: {}", target.string(), ec.message());
        std::filesystem::remove(part, ec);
        return AddResult::NoSpace;
    }
    renderCache_[name] = std::move(rendered);
    logger()->info("Added default image {}", name);
    return AddResult::Added;
}

DefaultImages::RemoveResult DefaultImages::remove(const std::string& name)
{
    if (!isValidName(name))
        return RemoveResult::NotFound;
    std::lock_guard lk(mtx_);
    _healLocked();
    const auto path = dir_ / name;
    if (!std::filesystem::exists(path))
        return RemoveResult::NotFound;
    if (_pictures(dir_).size() <= kMin)
        return RemoveResult::AtMinimum;
    std::error_code ec;
    if (!std::filesystem::remove(path, ec))
    {
        logger()->error("Failed to remove {}: {}", path.string(), ec.message());
        return RemoveResult::NotFound;
    }
    renderCache_.erase(name);
    logger()->info("Removed default image {}", name);
    return RemoveResult::Removed;
}

void DefaultImages::_restoreBundledLocked()
{
    const auto bundled = _pictures(bundledDir_);
    std::set<std::string> keep;
    for (const auto& source : bundled)
    {
        const auto bytes = readBytes(source);
        const auto name = bytes ? nameFor(*bytes) : "";
        if (name.empty())
        {
            logger()->error("Cannot read bundled picture {}", source.string());
            continue;
        }
        const auto target = dir_ / name;
        if (!std::filesystem::exists(target) && !writeAtomically(target, *bytes))
        {
            logger()->error("Failed to copy bundled picture {}", source.string());
            continue;
        }
        keep.insert(name);
    }
    if (keep.size() < kMin)
    {
        logger()->error("Only {} usable bundled picture(s) in {}", keep.size(), bundledDir_.string());
        return;
    }

    for (const auto& path : _pictures(dir_))
    {
        if (keep.contains(path.filename().string()))
            continue;
        std::error_code ec;
        std::filesystem::remove(path, ec);
        renderCache_.erase(path.filename().string());
    }
    logger()->info("Default images restored to the {} bundled ones", keep.size());
}

const std::vector<uint8_t>& DefaultImages::_rendered(const std::filesystem::path& path)
{
    const auto name = path.filename().string();
    if (const auto it = renderCache_.find(name); it != renderCache_.end())
        return it->second;
    return renderCache_[name] = PhotoRenderer::render(path);
}
