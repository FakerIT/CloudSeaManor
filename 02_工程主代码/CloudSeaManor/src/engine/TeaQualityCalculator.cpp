#include "CloudSeamanor/engine/TeaQualityCalculator.hpp"

namespace CloudSeamanor::engine {

CloudSeamanor::domain::CropQuality TeaQualityCalculator::Calculate(
    const TeaPlot& plot,
    const bool is_legendary) {
    return CloudSeamanor::domain::CropTable::CalculateFinalQuality(
        plot.weather_at_planting,
        plot.cloud_density_at_planting,
        plot.aura_at_planting,
        plot.dense_cloud_days_accumulated,
        plot.tide_days_accumulated,
        plot.tea_soul_flower_nearby,
        plot.fertilizer_type_for_quality,
        is_legendary);
}

CloudSeamanor::domain::CropQuality TeaQualityCalculator::CalculateWithSnapshot(
    const TeaPlot& plot,
    const bool is_legendary) {
    // 先计算基础品质
    auto quality = Calculate(plot, is_legendary);

    // 生态加成：使用播种时快照的生态加成
    if (plot.ecology_bonus_at_planting > 0.0f) {
        const int boost = static_cast<int>(plot.ecology_bonus_at_planting);
        const auto boosted = static_cast<CloudSeamanor::domain::CropQuality>(
            static_cast<int>(quality) + boost);
        // 上限为圣品
        if (boosted > CloudSeamanor::domain::CropQuality::Holy) {
            return CloudSeamanor::domain::CropQuality::Holy;
        }
        return boosted;
    }

    return quality;
}

}  // namespace CloudSeamanor::engine

