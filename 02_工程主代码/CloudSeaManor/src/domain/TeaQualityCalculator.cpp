#include "CloudSeamanor/TeaQualityCalculator.hpp"

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

}  // namespace CloudSeamanor::engine

