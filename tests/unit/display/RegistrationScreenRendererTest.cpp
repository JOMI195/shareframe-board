#include "display/RegistrationScreenRenderer.hpp"
#include <gtest/gtest.h>

TEST(RegistrationScreenRenderer, LinkOpensTheRegisterDialogWithTheCode)
{
    EXPECT_EQ(RegistrationScreenRenderer::registerUrl("https://www.shareframe.de", "/bilderrahmen?code=", "K7M2QX9P"),
              "https://www.shareframe.de/bilderrahmen?code=K7M2QX9P");
}

TEST(RegistrationScreenRenderer, CodeIsGroupedLikeTheRegisterDialog)
{
    EXPECT_EQ(RegistrationScreenRenderer::formatClaimCode("K7M2QX9P"), "K7M2-QX9P");
    EXPECT_EQ(RegistrationScreenRenderer::formatClaimCode(""), "");
}
