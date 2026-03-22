#include "repository/ImageRepository.hpp"

ImageRepository::ImageRepository(SQLite::Database& db) : _db(db)
{
}

void ImageRepository::save(const Image& image)
{
    SQLite::Statement stmt(
        _db,
        "INSERT OR REPLACE INTO images (id, sender, image_path, expires_at) "
        "VALUES (?, ?, ?, ?)");
    stmt.bind(1, image.id);
    stmt.bind(2, image.sender);
    stmt.bind(3, image.imagePath.string());
    stmt.bind(4, image.expiresAt);
    stmt.exec();
}

std::vector<Image> ImageRepository::getAll()
{
    std::vector<Image> images;
    SQLite::Statement query(
        _db,
        "SELECT id, sender, image_path, expires_at FROM images");
    while (query.executeStep())
    {
        images.push_back({
            .id = query.getColumn(0).getInt64(),
            .sender = query.getColumn(1).getString(),
            .imagePath = query.getColumn(2).getString(),
            .expiresAt = query.getColumn(3).getInt64(),
        });
    }
    return images;
}

std::vector<int64_t> ImageRepository::getAllIds()
{
    std::vector<int64_t> ids;
    SQLite::Statement query(_db, "SELECT id FROM images");
    while (query.executeStep())
        ids.push_back(query.getColumn(0).getInt64());
    return ids;
}

std::vector<Image> ImageRepository::getByIds(const std::vector<int64_t>& ids)
{
    if (ids.empty())
        return {};

    std::string sql = "SELECT id, sender, image_path, expires_at FROM images WHERE id IN (";
    for (size_t i = 0; i < ids.size(); ++i)
    {
        if (i > 0)
            sql += ", ";
        sql += "?";
    }
    sql += ")";

    SQLite::Statement query(_db, sql);
    for (size_t i = 0; i < ids.size(); ++i)
        query.bind(static_cast<int>(i + 1), ids[i]);

    std::vector<Image> images;
    while (query.executeStep())
    {
        images.push_back({
            .id = query.getColumn(0).getInt64(),
            .sender = query.getColumn(1).getString(),
            .imagePath = query.getColumn(2).getString(),
            .expiresAt = query.getColumn(3).getInt64(),
        });
    }
    return images;
}

void ImageRepository::remove(const int64_t id)
{
    SQLite::Statement stmt(_db, "DELETE FROM images WHERE id = ?");
    stmt.bind(1, id);
    stmt.exec();
}

void ImageRepository::removeByIds(const std::vector<int64_t>& ids)
{
    if (ids.empty())
        return;

    std::string sql = "DELETE FROM images WHERE id IN (";
    for (size_t i = 0; i < ids.size(); ++i)
    {
        if (i > 0)
            sql += ", ";
        sql += "?";
    }
    sql += ")";

    SQLite::Statement stmt(_db, sql);
    for (size_t i = 0; i < ids.size(); ++i)
        stmt.bind(static_cast<int>(i + 1), ids[i]);
    stmt.exec();
}

void ImageRepository::clear()
{
    _db.exec("DELETE FROM images");
}
