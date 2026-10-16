#include "display/LogoScreenRenderer.hpp"
#include "display/PanelText.hpp"
#include "display/PhotoRenderer.hpp"
#include <spdlog/spdlog.h>

namespace
{
using namespace panel_text;
constexpr int kCentreX = DisplayManager::WIDTH / 2;
constexpr int kTitleBaseline = 359;
constexpr int kTitlePx = 64;
constexpr int kMinTitlePx = 32;
constexpr int kDetailBaseline = 400;
constexpr int kDetailPx = 24;

const std::shared_ptr<spdlog::logger>& logger()
{
    static const auto instance = spdlog::default_logger()->clone("LogoScreen");
    return instance;
}

void drawCentred(Canvas& canvas, const std::string& path, const std::string& text, int px, const int minPx,
                 const int baseline)
{
    Font font;
    if (!font.load(path))
    {
        logger()->error("Failed to load font {}, skipping \"{}\"", path, text);
        return;
    }
    const auto line = decodeUtf8(text);
    const int maxWidth = SAFE_AREA.x1 - SAFE_AREA.x0;
    while (px > minPx && font.width(line, px) > maxWidth)
        px -= 2;
    font.draw(canvas, line, px, kCentreX - static_cast<int>(font.width(line, px) / 2), baseline);
}
} // namespace

std::string LogoScreenRenderer::versionLabel(const std::string& version, const std::string& sha)
{
    if (version == "dev")
        return "dev (" + sha.substr(0, 7) + ")";
    return "v" + version;
}

std::vector<uint8_t> LogoScreenRenderer::render(const std::filesystem::path& imagePath, const std::string& title,
                                                const std::string& detail, const std::string& boldFontPath,
                                                const std::string& regularFontPath)
{
    auto buffer = PhotoRenderer::render(imagePath);
    if (buffer.empty())
    {
        logger()->error("Failed to render {}, showing the text on white", imagePath.string());
        buffer.assign(DisplayManager::BUFFER_SIZE, 0xFF);
    }

    Canvas canvas(std::move(buffer));
    drawCentred(canvas, boldFontPath, title, kTitlePx, kMinTitlePx, kTitleBaseline);
    if (!detail.empty())
        drawCentred(canvas, regularFontPath, detail, kDetailPx, kDetailPx, kDetailBaseline);
    return canvas.take();
}
