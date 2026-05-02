#pragma once

#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/domain/CropData.hpp"
#include "CloudSeamanor/MathTypes.hpp"

#include <string>

namespace CloudSeamanor::domain {

struct TeaBush {
    std::string id;
    std::string name;
    std::string tea_id;
    int age_days = 0;
    int harvest_interval_days = 5;
    int last_harvest_day = -9999;
    bool alive = true;
    CloudSeamanor::domain::Vec2f position{0.0f, 0.0f};
    CloudSeamanor::domain::Vec2f size{26.0f, 26.0f};

    [[nodiscard]] bool CanHarvest(int current_day) const noexcept;
    [[nodiscard]] int AgeQualityBonus() const noexcept;
    [[nodiscard]] int DaysUntilHarvest(int current_day) const noexcept;
};

[[nodiscard]] CropQuality CalculateTeaBushQuality(
    const TeaBush& bush,
    const CloudSystem& cloud,
    CropQuality base_quality) noexcept;

}  // namespace CloudSeamanor::domain
