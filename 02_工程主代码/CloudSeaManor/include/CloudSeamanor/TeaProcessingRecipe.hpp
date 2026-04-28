#pragma once

#include <string>

namespace CloudSeamanor::domain {

struct TeaProcessingRecipe {
    std::string id;
    std::string input_item_id;
    int input_count = 1;
    std::string output_item_id;
    int output_count = 1;
    float process_seconds = 60.0f;
    float success_rate = 1.0f;
    std::string buff_effect_id;
};

}  // namespace CloudSeamanor::domain

