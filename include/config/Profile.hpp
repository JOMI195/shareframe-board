#pragma once
#include <array>
#include <string_view>

enum class Profile { Dev, Prod, ProdLocal, Sim };

inline constexpr std::array kProfiles = {Profile::Dev, Profile::Prod, Profile::ProdLocal, Profile::Sim};

constexpr std::string_view profileName(const Profile p)
{
    switch (p)
    {
    case Profile::Dev:       return "dev";
    case Profile::Prod:      return "prod";
    case Profile::ProdLocal: return "prod.local";
    case Profile::Sim:       return "sim";
    }
    __builtin_unreachable();
}
