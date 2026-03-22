#pragma once
#include "repository/IImageRepository.hpp"
#include <SQLiteCpp/SQLiteCpp.h>

class ImageRepository : public IImageRepository
{
public:
    explicit ImageRepository(SQLite::Database& db);
    void                  save(const Image& image)                     override;
    std::vector<Image>    getAll()                                     override;
    std::vector<int64_t>  getAllIds()                                   override;
    std::vector<Image>    getByIds(const std::vector<int64_t>& ids)     override;
    void                  remove(int64_t id)                           override;
    void                  removeByIds(const std::vector<int64_t>& ids) override;
    void                  clear()                                      override;

private:
    SQLite::Database& _db;
};
