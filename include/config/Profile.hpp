#pragma once
#include <string_view>

enum class Profile { Dev, Prod, ProdLocal };

constexpr std::string_view profileName(const Profile p)
{
    switch (p)
    {
    case Profile::Dev:       return "dev";
    case Profile::Prod:      return "prod";
    case Profile::ProdLocal: return "prod.local";
    }
    __builtin_unreachable();
}
