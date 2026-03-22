#pragma once
#include "repository/ImageRepository.hpp"
#include "task/PeriodicTask.hpp"

class ImageCheck : public PeriodicTask
{
public:
    ImageCheck(EventBus& bus, const AppConfig& cfg, ImageRepository& repo);

protected:
    [[nodiscard]] int intervalSecs() const override;
    void execute() override;

private:
    void _checkExpiry() const;
    void _checkMissing() const;

    ImageRepository& repo_;
};
