#pragma once
#include "db/Token.hpp"
#include <optional>

class ITokenRepository
{
public:
    virtual ~ITokenRepository() = default;
    virtual void                  save(const Token& token) = 0;
    virtual std::optional<Token>  get()                    = 0;
    virtual void                  clear()                  = 0;
};
