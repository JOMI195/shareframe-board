#include "repository/TokenRepository.hpp"

TokenRepository::TokenRepository(SQLite::Database& db) : _db(db)
{
}

void TokenRepository::save(const Token& token)
{
    _db.exec("DELETE FROM tokens");
    SQLite::Statement insert(_db, "INSERT INTO tokens (value, expires_at) VALUES (?, ?)");
    insert.bind(1, token.value);
    insert.bind(2, token.expiresAt);
    insert.exec();
}

std::optional<Token> TokenRepository::get()
{
    if (SQLite::Statement query(_db, "SELECT value, expires_at FROM tokens LIMIT 1"); query.executeStep())
        return Token{query.getColumn(0).getString(), query.getColumn(1).getInt64()};
    return std::nullopt;
}

void TokenRepository::clear()
{
    _db.exec("DELETE FROM tokens");
}
