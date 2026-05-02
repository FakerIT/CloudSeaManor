#pragma once

// ============================================================================
// 【CropGrowthSystem】作物生长系统
// ============================================================================
// Responsibilities:
// - Update crop growth stages based on elapsed time
// - Apply weather/cloud multipliers to growth speed (day/night)
// - Apply fertilizer, greenhouse, sprinkler bonuses
// - Quality snapshot at planting
// - Daily accumulation of dense cloud / tide days
// - Spirit mutation trigger on harvest (dense cloud / tide)
// - Disease and pest system
// - Ecology quality bonus integration
// ============================================================================

#include "CloudSeamanor/engine/GameWorldState.hpp"
#include "CloudSeamanor/domain/GameClock.hpp"
#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/domain/ManorEcologySystem.hpp"

#include <functional>
#include <string>

namespace CloudSeamanor::engine {

class CropGrowthSystem {
public:
    using HintCallback = std::function<void(const std::string&, float)>;

    CropGrowthSystem();

    void SetHintCallback(HintCallback callback);
    void SetEcologySystem(const CloudSeamanor::domain::ManorEcologySystem* ecology);

    void SetFertilizerMultipliers(
        float basic_multiplier,
        float premium_multiplier,
        float spirit_multiplier = 1.8f,
        float cloud_multiplier = 2.0f,
        float tea_soul_multiplier = 2.2f
    ) {
        fertilizer_basic_multiplier_ = basic_multiplier;
        fertilizer_premium_multiplier_ = premium_multiplier;
        fertilizer_spirit_multiplier_ = spirit_multiplier;
        fertilizer_cloud_multiplier_ = cloud_multiplier;
        fertilizer_tea_soul_multiplier_ = tea_soul_multiplier;
    }

    // ========================================================================
    // 【Update】每帧更新作物生长
    // ========================================================================
    // @param world_state 游戏世界状态
    // @param delta_seconds 距离上次更新的时间（秒）
    // @param cloud_system 云海系统（用于密度判定）
    void Update(
        GameWorldState& world_state,
        float delta_seconds,
        const CloudSeamanor::domain::CloudSystem& cloud_system
    );

    void HandleSeasonChanged(
        GameWorldState& world_state,
        CloudSeamanor::domain::Season current_season
    );

    void HandleDailyDiseaseAndPest(GameWorldState& world_state);

    // ========================================================================
    // 【HandleDailyAccumulation】每日睡觉时调用：累积云海天数
    // ========================================================================
    void HandleDailyAccumulation(
        GameWorldState& world_state,
        const CloudSeamanor::domain::CloudSystem& cloud_system
    );

    // ========================================================================
    // 【ApplyPlantingSnapshot】播种时记录品质快照
    // ========================================================================
    void ApplyPlantingSnapshot(
        TeaPlot& plot,
        const CloudSeamanor::domain::CloudSystem& cloud_system
    );

    // ========================================================================
    // 【CalculateGrowthMultiplier】计算生长倍率
    // ========================================================================
    [[nodiscard]] float CalculateGrowthMultiplier(
        const TeaPlot& plot,
        const CloudSeamanor::domain::CloudState cloud_state
    );

    // ========================================================================
    // 【CheckSpiritMutation】检测灵化变种（成熟时调用）
    // ========================================================================
    void CheckSpiritMutation(
        TeaPlot& plot,
        const CloudSeamanor::domain::CloudSystem& cloud_system
    );

private:
    HintCallback hint_callback_;
    const CloudSeamanor::domain::ManorEcologySystem* ecology_system_ = nullptr;
    float fertilizer_basic_multiplier_ = 1.2f;
    float fertilizer_premium_multiplier_ = 1.5f;
    float fertilizer_spirit_multiplier_ = 1.8f;
    float fertilizer_cloud_multiplier_ = 2.0f;
    float fertilizer_tea_soul_multiplier_ = 2.2f;

    // P18-P003: 生长倍率缓存（基于云海状态缓存）
    struct GrowthMultiplierCache {
        CloudSeamanor::domain::CloudState cloud_state = CloudSeamanor::domain::CloudState::Clear;
        float ecology_bonus = 0.0f;
        float cached_multiplier = 1.0f;
        bool valid = false;
    };
    GrowthMultiplierCache growth_multiplier_cache_;
    int last_cache_hour_ = -1;  // 用于夜晚惩罚缓存
    float night_multiplier_cache_ = 1.0f;
};

}  // namespace CloudSeamanor::engine
