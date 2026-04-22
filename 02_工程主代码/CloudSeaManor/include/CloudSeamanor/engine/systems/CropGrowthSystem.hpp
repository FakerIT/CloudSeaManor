#pragma once

// ============================================================================
// 【CropGrowthSystem】作物生长系统
// ============================================================================
// Responsibilities:
// - Update crop growth stages based on elapsed time
// - Apply weather/cloud multipliers to growth speed
// - Detect stage transitions
// - Generate harvest notifications
// ============================================================================

#include "CloudSeamanor/GameWorldState.hpp"
#include "CloudSeamanor/GameClock.hpp"

#include <functional>
#include <string>

namespace CloudSeamanor::engine {

class CropGrowthSystem {
public:
    using HintCallback = std::function<void(const std::string&, float)>;

    CropGrowthSystem();

    void SetHintCallback(HintCallback callback);

    void SetFertilizerMultipliers(float basic_multiplier, float premium_multiplier) {
        fertilizer_basic_multiplier_ = basic_multiplier;
        fertilizer_premium_multiplier_ = premium_multiplier;
    }

    void Update(
        GameWorldState& world_state,
        float delta_seconds
    );
    void HandleSeasonChanged(
        GameWorldState& world_state,
        CloudSeamanor::domain::Season current_season
    );
    void HandleDailyDiseaseAndPest(GameWorldState& world_state);

private:
    HintCallback hint_callback_;
    float fertilizer_basic_multiplier_ = 1.2f;
    float fertilizer_premium_multiplier_ = 1.5f;
};

}  // namespace CloudSeamanor::engine
