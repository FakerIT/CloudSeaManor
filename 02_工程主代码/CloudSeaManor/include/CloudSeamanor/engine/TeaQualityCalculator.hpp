#pragma once

#include "CloudSeamanor/domain/CropData.hpp"
#include "CloudSeamanor/engine/GameAppRuntimeTypes.hpp"

namespace CloudSeamanor::engine {

class TeaQualityCalculator {
public:
    [[nodiscard]] static CloudSeamanor::domain::CropQuality Calculate(
        const TeaPlot& plot,
        bool is_legendary = false);
};

}  // namespace CloudSeamanor::engine

