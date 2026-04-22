#pragma once

#include "CloudSeamanor/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/CloudSystem.hpp"

#include <vector>

namespace CloudSeamanor::domain {

class TeaGarden {
public:
    void UpdatePlots(
        std::vector<CloudSeamanor::engine::TeaPlot>& plots,
        float delta_seconds,
        CloudSeamanor::domain::CloudState cloud_state,
        int day_of_year);
};

} // namespace CloudSeamanor::domain
