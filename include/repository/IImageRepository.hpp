#pragma once
#include "repository/entity/Image.hpp"
#include <vector>

class IImageRepository
{
public:
    virtual ~IImageRepository() = default;
    virtual void save(const Image& image) = 0;
    virtual std::vector<Image> getAll() = 0;
    virtual std::vector<int64_t> getAllIds() = 0;
    virtual std::vector<Image> getByIds(const std::vector<int64_t>& ids) = 0;
    virtual void remove(int64_t id) = 0;
    virtual void removeByIds(const std::vector<int64_t>& ids) = 0;
    virtual void clear() = 0;
};
