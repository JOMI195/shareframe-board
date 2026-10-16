#include "display/LogoScreenRenderer.hpp"
#include <gtest/gtest.h>

TEST(LogoScreenRenderer, ReleaseShowsItsVersion)
{
    EXPECT_EQ(LogoScreenRenderer::versionLabel("1.2.0", "abc1234def"), "v1.2.0");
}

TEST(LogoScreenRenderer, DevBuildShowsItsShortSha)
{
    EXPECT_EQ(LogoScreenRenderer::versionLabel("dev", "abc1234def"), "dev (abc1234)");
    EXPECT_EQ(LogoScreenRenderer::versionLabel("dev", "abc"), "dev (abc)");
}
