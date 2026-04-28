#include "CloudSeamanor/FestivalRuntimeData.hpp"

#include <algorithm>

namespace CloudSeamanor::engine {

void FestivalRuntimeData::Reset() {
    *this = FestivalRuntimeData{};
}

void FestivalRuntimeData::ClearDayScopedVisuals_(int new_game_day) {
    (void)new_game_day;
    flower_bloom_visual_day = -1;
    winter_solstice_polar_anchor_day = -1;
}

void FestivalRuntimeData::OnBeginNewDay(int new_game_day) {
    ClearDayScopedVisuals_(new_game_day);
    if (qingming_double_spirit_gather_day >= 0 && qingming_double_spirit_gather_day < new_game_day) {
        qingming_double_spirit_gather_day = -1;
    }
    if (double_ninth_chrysanthemum_day >= 0 && double_ninth_chrysanthemum_day < new_game_day) {
        double_ninth_chrysanthemum_day = -1;
    }
    if (harvest_festival_sell_bonus_day >= 0 && harvest_festival_sell_bonus_day < new_game_day) {
        harvest_festival_sell_bonus_day = -1;
    }
    if (tea_culture_contest_day >= 0 && tea_culture_contest_day < new_game_day) {
        tea_culture_contest_day = -1;
    }
    if (mid_autumn_regen_bonus_until_day >= 0 && mid_autumn_regen_bonus_until_day < new_game_day) {
        mid_autumn_regen_bonus_until_day = -1;
    }
}

void FestivalRuntimeData::AppendSaveLines(std::vector<std::string>& lines) const {
    for (const auto& [fid, d] : mvp_auto_reward_last_day_by_festival) {
        lines.push_back("fest_rt|mvp_day|" + fid + "|" + std::to_string(d));
    }
    auto push_int = [&lines](const char* key, int v) {
        if (v >= 0) {
            lines.push_back(std::string("fest_rt|int|") + key + "|" + std::to_string(v));
        }
    };
    push_int("qingming_double", qingming_double_spirit_gather_day);
    push_int("flower_visual", flower_bloom_visual_day);
    push_int("mid_autumn_regen_until", mid_autumn_regen_bonus_until_day);
    push_int("double_ninth", double_ninth_chrysanthemum_day);
    push_int("harvest_sell", harvest_festival_sell_bonus_day);
    push_int("tea_contest", tea_culture_contest_day);
    push_int("winter_polar", winter_solstice_polar_anchor_day);
}

bool FestivalRuntimeData::TryConsumeSaveLine(const std::string& tag, const std::vector<std::string>& fields) {
    if (tag != "fest_rt" || fields.size() < 2) {
        return false;
    }
    const std::string& sub = fields[1];
    if (sub == "mvp_day" && fields.size() >= 4) {
        try {
            mvp_auto_reward_last_day_by_festival[fields[2]] = std::stoi(fields[3]);
        } catch (...) {
            return false;
        }
        return true;
    }
    if (sub == "int" && fields.size() >= 4) {
        int v = -1;
        try {
            v = std::stoi(fields[3]);
        } catch (...) {
            return false;
        }
        if (fields[2] == "qingming_double") {
            qingming_double_spirit_gather_day = v;
        } else if (fields[2] == "flower_visual") {
            flower_bloom_visual_day = v;
        } else if (fields[2] == "mid_autumn_regen_until") {
            mid_autumn_regen_bonus_until_day = v;
        } else if (fields[2] == "double_ninth") {
            double_ninth_chrysanthemum_day = v;
        } else if (fields[2] == "harvest_sell") {
            harvest_festival_sell_bonus_day = v;
        } else if (fields[2] == "tea_contest") {
            tea_culture_contest_day = v;
        } else if (fields[2] == "winter_polar") {
            winter_solstice_polar_anchor_day = v;
        }
        return true;
    }
    return false;
}

}  // namespace CloudSeamanor::engine
