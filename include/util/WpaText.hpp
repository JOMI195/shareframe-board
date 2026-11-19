#pragma once
#include "util/NonInstantiable.hpp"
#include <string>

class WpaText : NonInstantiable
{
public:
    /// Text as wpa_supplicant's printf_encode wrote it (\\ \" \e \n \r \t \xNN) back as its bytes.
    [[nodiscard]] static std::string decode(const std::string& text);
};
