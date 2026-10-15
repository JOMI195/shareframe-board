#pragma once
#include "repository/TokenRepository.hpp"
#include <ctime>

/// A cached, unexpired frame token, so token auth never has to reach the server.
inline void storeValidToken(SQLite::Database& db, const std::string& value = "test-token")
{
    TokenRepository(db).save({value, std::time(nullptr) + 3600});
}
