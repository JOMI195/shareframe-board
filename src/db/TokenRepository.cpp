#include "db/TokenRepository.hpp"

TokenRepository::TokenRepository(SQLite::Database& db) : _db(db)
{
    _db.exec(
        "CREATE TABLE IF NOT EXISTS tokens ("
        "  value      TEXT    NOT NULL,"
        "  expires_at INTEGER NOT NULL"
        ")"
    );
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
    SQLite::Statement query(_db, "SELECT value, expires_at FROM tokens LIMIT 1");
    if (query.executeStep())
        return Token{ query.getColumn(0).getString(), query.getColumn(1).getInt64() };
    return std::nullopt;
}

void TokenRepository::clear()
{
    _db.exec("DELETE FROM tokens");
}
