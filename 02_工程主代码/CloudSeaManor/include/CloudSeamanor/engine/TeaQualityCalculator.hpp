#pragma once

#include "CloudSeamanor/domain/CropData.hpp"
#include "CloudSeamanor/engine/GameAppRuntimeTypes.hpp"

namespace CloudSeamanor::engine {

class TeaQualityCalculator {
public:
    [[nodiscard]] static CloudSeamanor::domain::CropQuality Calculate(
        const TeaPlot& plot,
        bool is_legendary = false);

    // 使用 TeaPlot 中的 ecology_bonus_at_planting 快照计算品质
    [[nodiscard]] static CloudSeamanor::domain::CropQuality CalculateWithSnapshot(
        const TeaPlot& plot,
        bool is_legendary = false);
};

}  // namespace CloudSeamanor::engine

