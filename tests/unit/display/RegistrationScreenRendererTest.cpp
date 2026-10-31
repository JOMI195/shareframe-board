#include "display/RegistrationScreenRenderer.hpp"
#include <gtest/gtest.h>

TEST(RegistrationScreenRenderer, LinkOpensTheRegisterDialogWithTheSerial)
{
    EXPECT_EQ(RegistrationScreenRenderer::registerUrl("https://www.shareframe.de", "/bilderrahmen?register=",
                                                      "EOJ8-WXBV-Y86U-3LL3"),
              "https://www.shareframe.de/bilderrahmen?register=EOJ8-WXBV-Y86U-3LL3");
}
