#include "image/PictureCheck.hpp"
#include "image/PictureLimits.hpp"
#include <algorithm>
#include <stb_image.h>

namespace picture
{
Verdict check(const std::span<const uint8_t> bytes, const std::filesystem::path& storage, const int minFreeMb)
{
    if (bytes.size() > kMaxFileBytes)
        return Verdict::TooLarge;
    int width = 0, height = 0, comp = 0;
    if (!stbi_info_from_memory(bytes.data(), static_cast<int>(bytes.size()), &width, &height, &comp))
        return Verdict::NotAPicture;
    if (int64_t{width} * height > kMaxPixels)
        return Verdict::TooManyPixels;
    std::error_code ec;
    const auto space = std::filesystem::space(storage, ec);
    const auto reserve = static_cast<std::uintmax_t>(minFreeMb) * 1024 * 1024;
    if (!ec && space.available < bytes.size() + reserve)
        return Verdict::NoSpace;
    return Verdict::Ok;
}

std::string_view describe(const Verdict verdict)
{
    switch (verdict)
    {
    case Verdict::Ok: return "ok";
    case Verdict::TooLarge: return "over the size limit";
    case Verdict::NotAPicture: return "not a JPEG or PNG";
    case Verdict::TooManyPixels: return "over the pixel limit";
    case Verdict::NoSpace: return "not enough free space on the picture storage";
    }
    return "unknown";
}

std::string_view extension(const std::span<const uint8_t> bytes)
{
    static constexpr uint8_t png[] = {0x89, 'P', 'N', 'G'};
    if (bytes.size() >= 4 && std::equal(std::begin(png), std::end(png), bytes.begin()))
        return ".png";
    return ".jpg";
}
} // namespace picture
