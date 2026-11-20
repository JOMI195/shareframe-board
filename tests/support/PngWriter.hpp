#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <vector>
#include <zlib.h>

/// Writes an 8-bit greyscale PNG: the board only decodes JPEG and PNG, so test pictures are real PNGs.
inline void writeGreyPng(const std::filesystem::path& path, const int width, const int height,
                         const std::function<uint8_t(int x, int y)>& shade)
{
    std::vector<unsigned char> raw;
    raw.reserve(static_cast<size_t>(height) * (width + 1));
    for (int y = 0; y < height; ++y)
    {
        raw.push_back(0);
        for (int x = 0; x < width; ++x)
            raw.push_back(shade(x, y));
    }
    uLongf packedSize = compressBound(raw.size());
    std::vector<unsigned char> packed(packedSize);
    compress(packed.data(), &packedSize, raw.data(), raw.size());
    packed.resize(packedSize);

    std::ofstream f(path, std::ios::binary);
    const auto u32 = [&](const uint32_t v)
    {
        const unsigned char b[] = {static_cast<unsigned char>(v >> 24), static_cast<unsigned char>(v >> 16),
                                   static_cast<unsigned char>(v >> 8), static_cast<unsigned char>(v)};
        f.write(reinterpret_cast<const char*>(b), 4);
    };
    const auto chunk = [&](const std::string& type, const std::vector<unsigned char>& data)
    {
        u32(static_cast<uint32_t>(data.size()));
        f.write(type.data(), 4);
        f.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        uLong crc = crc32(0, reinterpret_cast<const Bytef*>(type.data()), 4);
        crc = crc32(crc, data.data(), static_cast<uInt>(data.size()));
        u32(static_cast<uint32_t>(crc));
    };
    f.write("\x89PNG\r\n\x1a\n", 8);
    const auto w = static_cast<uint32_t>(width), h = static_cast<uint32_t>(height);
    chunk("IHDR", {static_cast<unsigned char>(w >> 24), static_cast<unsigned char>(w >> 16),
                   static_cast<unsigned char>(w >> 8), static_cast<unsigned char>(w),
                   static_cast<unsigned char>(h >> 24), static_cast<unsigned char>(h >> 16),
                   static_cast<unsigned char>(h >> 8), static_cast<unsigned char>(h), 8, 0, 0, 0, 0});
    chunk("IDAT", packed);
    chunk("IEND", {});
}
