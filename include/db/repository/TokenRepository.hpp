#pragma once
#include "db/ITokenRepository.hpp"
#include <SQLiteCpp/SQLiteCpp.h>

class TokenRepository : public ITokenRepository
{
public:
    explicit TokenRepository(SQLite::Database& db);
    void                 save(const Token& token) override;
    std::optional<Token> get()                    override;
    void                 clear()                  override;

private:
    SQLite::Database& _db;
};
