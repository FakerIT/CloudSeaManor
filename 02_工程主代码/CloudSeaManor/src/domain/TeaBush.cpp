#include "CloudSeamanor/TeaBush.hpp"

#include <algorithm>

namespace CloudSeamanor::domain {

bool TeaBush::CanHarvest(const int current_day) const noexcept {
    if (!alive) {
        return false;
    }
    return (current_day - last_harvest_day) >= harvest_interval_days;
}

int TeaBush::AgeQualityBonus() const noexcept {
    return std::max(0, age_days / 30);
}

int TeaBush::DaysUntilHarvest(const int current_day) const noexcept {
    if (!alive) {
        return 9999;
    }
    const int elapsed = current_day - last_harvest_day;
    return std::max(0, harvest_interval_days - elapsed);
}

CropQuality CalculateTeaBushQuality(
    const TeaBush& bush,
    const CloudSystem&,
    const CropQuality base_quality) noexcept {
    int quality = static_cast<int>(base_quality);
    quality += bush.AgeQualityBonus();
    quality = std::clamp(quality, 0, 4);
    return static_cast<CropQuality>(quality);
}

}  // namespace CloudSeamanor::domain
