#include "CloudSeamanor/FishingSystem.hpp"
#include "CloudSeamanor/GameConstants.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <cmath>

namespace CloudSeamanor::domain {

// ============================================================================
// FishingSpot 实现
// ============================================================================
bool FishingSpot::IsAvailable(int current_game_time) const {
    int elapsed = current_game_time - last_fish_time;
    return elapsed >= cooldown_minutes;
}

// ============================================================================
// FishingState 实现
// ============================================================================
std::map<std::string, std::string> FishingState::ToMap() const {
    std::map<std::string, std::string> data;
    data["skill_level"] = std::to_string(skill_level);
    data["total_caught"] = std::to_string(total_caught);
    data["legendary_caught"] = std::to_string(legendary_caught);
    data["equipped_bait"] = equipped_bait_;
    data["bait_count"] = std::to_string(bait_count);
    return data;
}

void FishingState::FromMap(const std::map<std::string, std::string>& data) {
    auto get_int = [&](const std::string& key, int default_val) -> int {
        auto it = data.find(key);
        if (it != data.end()) {
            try { return std::stoi(it->second); } catch (...) {}
        }
        return default_val;
    };

    skill_level = get_int("skill_level", 1);
    total_caught = get_int("total_caught", 0);
    legendary_caught = get_int("legendary_caught", 0);
    equipped_bait_ = data.count("equipped_bait") ? data.at("equipped_bait") : "";
    bait_count = get_int("bait_count", 0);
}

// ============================================================================
// FishingSystem 实现
// ============================================================================
FishingSystem& FishingSystem::Instance() {
    static FishingSystem instance;
    return instance;
}

bool FishingSystem::Initialize() {
    const auto& assets = GameConstants::AssetsPath();

    LoadFishData(assets + "/data/fishing/fish_table.csv");
    LoadLocationData(assets + "/data/fishing/fishing_locations.csv");
    LoadBaitData(assets + "/data/fishing/bait_table.csv");
    LoadSkillData(assets + "/data/fishing/fishing_skills.csv");

    return !fish_data_.empty();
}

void FishingSystem::LoadFishData(const std::string& csv_path) {
    std::ifstream file(csv_path);
    if (!file.is_open()) return;

    std::string line;
    std::getline(file, line);  // 跳过表头

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        FishData fish;

        std::getline(ss, fish.id, ',');
        std::getline(ss, fish.name_zh, ',');
        std::getline(ss, fish.name_en, ',');
        {
            std::string val; std::getline(ss, val, ',');
            fish.base_price = std::stoi(val);
        }
        {
            std::string val; std::getline(ss, val, ',');
            fish.min_weight = std::stof(val);
        }
        {
            std::string val; std::getline(ss, val, ',');
            fish.max_weight = std::stof(val);
        }
        {
            std::string val; std::getline(ss, val, ',');
            fish.difficulty = std::stoi(val);
        }
        std::getline(ss, fish.season, ',');
        std::getline(ss, fish.weather, ',');
        std::getline(ss, fish.time_range, ',');
        std::getline(ss, fish.location_id, ',');
        std::getline(ss, fish.rarity, ',');
        {
            std::string val; std::getline(ss, val, ',');
            fish.tier = std::stoi(val);
        }
        std::getline(ss, fish.description_zh, ',');

        fish_data_[fish.id] = std::move(fish);
    }
}

void FishingSystem::LoadLocationData(const std::string& csv_path) {
    std::ifstream file(csv_path);
    if (!file.is_open()) return;

    std::string line;
    std::getline(file, line);  // 跳过表头

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        FishingLocation loc;

        std::getline(ss, loc.id, ',');
        std::getline(ss, loc.name_zh, ',');
        std::getline(ss, loc.description, ',');
        std::getline(ss, loc.required_item, ',');
        {
            std::string val; std::getline(ss, val, ',');
            loc.min_day = std::stoi(val);
        }
        std::getline(ss, loc.available_seasons, ',');
        {
            std::string val; std::getline(ss, val, ',');
            loc.difficulty_modifier = std::stoi(val);
        }

        location_data_[loc.id] = std::move(loc);
    }
}

void FishingSystem::LoadBaitData(const std::string& csv_path) {
    std::ifstream file(csv_path);
    if (!file.is_open()) return;

    std::string line;
    std::getline(file, line);  // 跳过表头

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        BaitData bait;

        std::getline(ss, bait.id, ',');
        std::getline(ss, bait.name_zh, ',');
        std::getline(ss, bait.name_en, ',');
        {
            std::string val; std::getline(ss, val, ',');
            bait.price = std::stoi(val);
        }
        std::getline(ss, bait.effect_type, ',');
        {
            std::string val; std::getline(ss, val, ',');
            bait.effect_value = std::stoi(val);
        }
        {
            std::string val; std::getline(ss, val, ',');
            bait.stack_size = std::stoi(val);
        }
        std::getline(ss, bait.unlock_condition, ',');

        bait_data_[bait.id] = std::move(bait);
    }
}

void FishingSystem::LoadSkillData(const std::string& csv_path) {
    std::ifstream file(csv_path);
    if (!file.is_open()) return;

    std::string line;
    std::getline(file, line);  // 跳过表头

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        FishingSkill skill;

        {
            std::string val; std::getline(ss, val, ',');
            skill.level = std::stoi(val);
        }
        std::getline(ss, skill.name_zh, ',');
        std::getline(ss, skill.name_en, ',');
        std::getline(ss, skill.new_unlocks_zh, ',');

        // 解析加值
        std::string& bonus = skill.new_unlocks_zh;
        if (bonus.find('+') == 0) {
            skill.power_bonus = std::stoi(bonus.substr(1));
        }

        skill_data_[skill.level] = std::move(skill);
    }
}

// ============================================================================
// 查询接口
// ============================================================================
const FishData* FishingSystem::GetFishData(const std::string& fish_id) const {
    auto it = fish_data_.find(fish_id);
    return it != fish_data_.end() ? &it->second : nullptr;
}

const FishingLocation* FishingSystem::GetLocationData(const std::string& location_id) const {
    auto it = location_data_.find(location_id);
    return it != location_data_.end() ? &it->second : nullptr;
}

const BaitData* FishingSystem::GetBaitData(const std::string& bait_id) const {
    auto it = bait_data_.find(bait_id);
    return it != bait_data_.end() ? &it->second : nullptr;
}

const FishingSkill* FishingSystem::GetSkillData(int level) const {
    auto it = skill_data_.find(level);
    return it != skill_data_.end() ? &it->second : nullptr;
}

std::vector<const FishData*> FishingSystem::GetAvailableFish(
    const std::string& season,
    const std::string& weather,
    int game_time_minutes,
    const std::string& location_id
) const {
    std::vector<const FishData*> available;

    for (const auto& [id, fish] : fish_data_) {
        // 检查季节
        if (!IsSeasonMatch(fish.season, season)) continue;

        // 检查天气
        if (!IsWeatherMatch(fish.weather, weather)) continue;

        // 检查时间
        if (!IsInTimeRange(game_time_minutes, fish.time_range)) continue;

        // 检查地点
        if (fish.location_id != location_id &&
            fish.location_id != "all" &&
            fish.location_id.find(location_id) == std::string::npos) continue;

        available.push_back(&fish);
    }

    return available;
}

// ============================================================================
// 钓鱼操作
// ============================================================================
FishingResult FishingSystem::TryFish(
    const std::string& location_id,
    const std::string& bait_id,
    int current_game_time,
    int fishing_power,
    float luck_modifier
) {
    FishingResult result;

    // 检查地点冷却
    if (!IsLocationAvailable(location_id, current_game_time)) {
        result.message = "这里刚刚钓过鱼，需要等一会儿。";
        return result;
    }

    // 获取地点数据
    const auto* location = GetLocationData(location_id);
    if (!location) {
        result.message = "未知的钓鱼地点。";
        return result;
    }

    // 获取可钓的鱼
    const auto* game_clock = GetGameClockInstance();
    std::string current_season = game_clock ? game_clock->GetSeason() : "spring";
    std::string current_weather = game_clock ? game_clock->GetWeather() : "clear";

    auto available_fish = GetAvailableFish(
        current_season,
        current_weather,
        current_game_time,
        location_id
    );

    if (available_fish.empty()) {
        result.message = "这里目前没有鱼可以钓。";
        return result;
    }

    // 计算最终难度
    int final_difficulty = CalculateFinalDifficulty(1, location_id);

    // 计算钓鱼成功率
    const BaitData* bait = bait_id.empty() ? nullptr : GetBaitData(bait_id);
    float catch_chance = CalculateCatchChance(final_difficulty, fishing_power, luck_modifier, bait);

    // 随机判定
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    if (dis(gen) > catch_chance) {
        result.message = "狡猾的鱼溜走了...";
        RecordFishing(location_id, current_game_time);
        return result;
    }

    // 选择鱼（根据权重）
    std::uniform_int_distribution<> fish_dis(0, static_cast<int>(available_fish.size()) - 1);
    const FishData* caught_fish = available_fish[fish_dis(gen)];

    // 生成重量
    std::uniform_real_distribution<> weight_dis(
        caught_fish->min_weight,
        caught_fish->max_weight
    );
    float weight = weight_dis(gen);

    // 计算品质
    int quality = CalculateQuality(weight, caught_fish->max_weight, fishing_power);

    // 计算售价
    int price = CalculateSellPrice(*caught_fish, quality, weight);

    // 更新状态
    state_.total_caught++;
    state_.fish_count[caught_fish->id]++;
    if (weight > state_.fish_best_weight[caught_fish->id]) {
        state_.fish_best_weight[caught_fish->id] = static_cast<int>(weight * 100);
    }
    if (caught_fish->rarity == "legendary") {
        state_.legendary_caught++;
    }

    // 消耗鱼饵
    if (!bait_id.empty() && bait_count_ > 0) {
        bait_count_--;
        if (bait_count_ <= 0) {
            equipped_bait_.clear();
        }
    }

    // 增加经验
    experience_ += caught_fish->difficulty * 10;

    // 记录钓鱼
    RecordFishing(location_id, current_game_time);

    // 构建结果
    result.success = true;
    result.fish_id = caught_fish->id;
    result.weight = weight;
    result.quality = quality;
    result.sell_price = price;
    result.message = "钓到了 " + caught_fish->name_zh + "！";

    return result;
}

// ============================================================================
// 技能系统
// ============================================================================
bool FishingSystem::CanLevelUp() const {
    return GetCurrentExperience() >= GetExperienceForNextLevel();
}

void FishingSystem::AddExperience(int exp) {
    experience_ += exp;

    while (CanLevelUp() && state_.skill_level < 5) {
        experience_ -= GetExperienceForNextLevel();
        state_.skill_level++;
    }
}

int FishingSystem::GetExperienceForNextLevel() const {
    // 升级经验需求：100, 200, 400, 800, 1600
    return 100 * static_cast<int>(std::pow(2, state_.skill_level - 1));
}

// ============================================================================
// 鱼饵管理
// ============================================================================
bool FishingSystem::EquipBait(const std::string& bait_id, int count) {
    const auto* bait = GetBaitData(bait_id);
    if (!bait) return false;

    equipped_bait_ = bait_id;
    bait_count_ = count;
    state_.equipped_bait_ = bait_id;
    state_.bait_count = count;
    return true;
}

void FishingSystem::UnequipBait() {
    equipped_bait_.clear();
    bait_count_ = 0;
    state_.equipped_bait_.clear();
    state_.bait_count = 0;
}

// ============================================================================
// 地点冷却
// ============================================================================
bool FishingSystem::IsLocationAvailable(const std::string& location_id, int current_game_time) {
    for (auto& spot : state_.fishing_spots) {
        if (spot.location_id == location_id) {
            return spot.IsAvailable(current_game_time);
        }
    }
    return true;  // 新地点首次可用
}

void FishingSystem::RecordFishing(const std::string& location_id, int current_game_time) {
    for (auto& spot : state_.fishing_spots) {
        if (spot.location_id == location_id) {
            spot.last_fish_time = current_game_time;
            return;
        }
    }
    // 新增地点
    FishingSpot spot;
    spot.location_id = location_id;
    spot.last_fish_time = current_game_time;
    state_.fishing_spots.push_back(spot);
}

// ============================================================================
// 统计
// ============================================================================
int FishingSystem::GetCaughtCount(const std::string& fish_id) const {
    auto it = state_.fish_count.find(fish_id);
    return it != state_.fish_count.end() ? it->second : 0;
}

int FishingSystem::GetBestWeight(const std::string& fish_id) const {
    auto it = state_.fish_best_weight.find(fish_id);
    return it != state_.fish_best_weight.end() ? it->second : 0;
}

// ============================================================================
// 难度计算
// ============================================================================
int FishingSystem::CalculateFinalDifficulty(int base_difficulty, const std::string& location_id) const {
    const auto* location = GetLocationData(location_id);
    if (!location) return base_difficulty;
    return base_difficulty + location->difficulty_modifier;
}

// ============================================================================
// 私有辅助方法
// ============================================================================
bool FishingSystem::ParseTimeRange(const std::string& range, int& start_minutes, int& end_minutes) const {
    auto dash_pos = range.find('-');
    if (dash_pos == std::string::npos) return false;

    try {
        std::string start_str = range.substr(0, dash_pos);
        std::string end_str = range.substr(dash_pos + 1);

        start_minutes = std::stoi(start_str) / 100 * 60 + std::stoi(start_str) % 100;
        end_minutes = std::stoi(end_str) / 100 * 60 + std::stoi(end_str) % 100;
        return true;
    } catch (...) {
        return false;
    }
}

bool FishingSystem::IsInTimeRange(int current_minutes, const std::string& range) const {
    if (range == "any" || range.empty()) return true;

    int start, end;
    if (!ParseTimeRange(range, start, end)) return true;

    // 处理跨夜情况 (如 1800-0600)
    if (end < start) {
        return current_minutes >= start || current_minutes <= end;
    }
    return current_minutes >= start && current_minutes <= end;
}

bool FishingSystem::IsSeasonMatch(const std::string& fish_season, const std::string& current_season) const {
    if (fish_season == "all" || fish_season.empty()) return true;

    // 支持多季节 (spring|summer)
    size_t pos = fish_season.find(current_season);
    return pos != std::string::npos;
}

bool FishingSystem::IsWeatherMatch(const std::string& fish_weather, const std::string& current_weather) const {
    if (fish_weather == "any" || fish_weather.empty()) return true;
    return fish_weather == current_weather;
}

float FishingSystem::CalculateCatchChance(
    int difficulty,
    int fishing_power,
    float luck_modifier,
    const BaitData* bait
) const {
    // 基础成功率 = 100% - (难度 * 10%) + 钓鱼技能修正 + 幸运修正
    float base_chance = 1.0f - difficulty * 0.1f;
    float skill_bonus = fishing_power * 0.02f;
    float luck_bonus = luck_modifier * 0.01f;
    float bait_bonus = 0.0f;

    if (bait) {
        if (bait->effect_type == "chanceIncrease") {
            bait_bonus = bait->effect_value * 0.01f;
        }
    }

    float final_chance = base_chance + skill_bonus + luck_bonus + bait_bonus;
    return std::clamp(final_chance, 0.1f, 0.95f);
}

int FishingSystem::CalculateQuality(float weight, float max_weight, int fishing_power) const {
    float ratio = weight / max_weight;

    // 品质基于重量比例和钓鱼技能
    if (ratio > 0.9f && fishing_power >= 40) return 5;  // 传说品质
    if (ratio > 0.8f && fishing_power >= 30) return 4;  // 史诗品质
    if (ratio > 0.6f && fishing_power >= 20) return 3;   // 稀有品质
    if (ratio > 0.4f) return 2;                          // 优秀品质
    return 1;                                             // 普通品质
}

int FishingSystem::CalculateSellPrice(const FishData& fish, int quality, float weight) const {
    // 基础价格 * 重量 * 品质系数
    static const float quality_multiplier[] = {1.0f, 1.5f, 2.0f, 3.0f, 5.0f, 8.0f};
    float mult = quality >= 0 && quality <= 5 ? quality_multiplier[quality] : 1.0f;
    return static_cast<int>(fish.base_price * weight * mult);
}

// ============================================================================
// 全局辅助函数声明
// ============================================================================
class GameClockHelper {
public:
    static const GameClock* GetGameClockInstance();
};

}  // namespace CloudSeamanor::domain
