#pragma once

#include "CloudSeamanor/engine/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/domain/CloudSystem.hpp"

#include <vector>

namespace CloudSeamanor::engine {

class TeaGarden {
public:
    void UpdatePlots(
        std::vector<TeaPlot>& plots,
        float delta_seconds,
        CloudSeamanor::domain::CloudState cloud_state,
        int day_of_year);
};

} // namespace CloudSeamanor::engine

namespace CloudSeamanor::domain {
using TeaGarden = CloudSeamanor::engine::TeaGarden;
}
