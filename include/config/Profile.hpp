#pragma once
#include <string_view>

enum class Profile { Dev, Prod };

constexpr std::string_view profileName(const Profile p)
{
    switch (p)
    {
    case Profile::Dev: return "dev";
    case Profile::Prod: return "prod";
    }
    __builtin_unreachable();
}
