#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>

namespace CloudSeamanor::domain {

// ============================================================================
// 【FishData】单条鱼类数据
// ============================================================================
struct FishData {
    std::string id;
    std::string name_zh;
    std::string name_en;
    int base_price = 30;
    float min_weight = 0.5f;
    float max_weight = 2.0f;
    int difficulty = 1;          // 1-5 难度等级
    std::string season;         // spring|summer|autumn|winter|all
    std::string weather;        // any|clear|lightMist|heavyCloud|heavyRain|snow|tide
    std::string time_range;     // HHMM-HHMM 格式
    std::string location_id;
    std::string rarity;         // common|uncommon|rare|tide|legendary
    int tier = 1;
    std::string description_zh;
};

// ============================================================================
// 【FishingLocation】钓鱼地点数据
// ============================================================================
struct FishingLocation {
    std::string id;
    std::string name_zh;
    std::string description;
    std::string required_item;
    int min_day = 1;
    std::string available_seasons;
    int difficulty_modifier = 0;
};

// ============================================================================
// 【BaitData】鱼饵数据
// ============================================================================
struct BaitData {
    std::string id;
    std::string name_zh;
    int price = 2;
    std::string effect_type;     // chanceIncrease|spiritChance|rareChance|tideFishChance
    int effect_value = 5;
    int stack_size = 10;
    std::string unlock_condition;
};

// ============================================================================
// 【FishingSkill】钓鱼技能等级
// ============================================================================
struct FishingSkill {
    int level = 1;
    std::string name_zh;
    int power_bonus = 0;
    std::string new_unlocks_zh;
};

// ============================================================================
// 【FishingResult】钓鱼结果
// ============================================================================
struct FishingResult {
    bool success = false;
    std::string fish_id;
    float weight = 0.0f;
    int quality = 1;            // 1-5 品质等级
    int sell_price = 0;
    std::string message;
};

// ============================================================================
// 【FishingSpot】钓鱼点状态
// ============================================================================
struct FishingSpot {
    std::string location_id;
    int cooldown_minutes = 60;  // 冷却时间（游戏分钟）
    int last_fish_time = 0;    // 上次钓鱼的游戏时间（分钟）
    bool IsAvailable(int current_game_time) const;
};

// ============================================================================
// 【FishingState】玩家钓鱼状态
// ============================================================================
struct FishingState {
    int skill_level = 1;
    int total_caught = 0;
    int legendary_caught = 0;
    std::map<std::string, int> fish_count;      // 每种鱼的数量
    std::map<std::string, int> fish_best_weight; // 每种鱼的最佳重量
    std::string equipped_bait;
    int bait_count = 0;
    std::vector<FishingSpot> fishing_spots;

    // 存档序列化
    std::map<std::string, std::string> ToMap() const;
    void FromMap(const std::map<std::string, std::string>& data);
};

// ============================================================================
// 【FishingSystem】钓鱼系统核心
// ============================================================================
class FishingSystem {
public:
    // ========================================================================
    // 初始化
    // ========================================================================
    static FishingSystem& Instance();

    bool Initialize();
    void LoadFishData(const std::string& csv_path);
    void LoadLocationData(const std::string& csv_path);
    void LoadBaitData(const std::string& csv_path);
    void LoadSkillData(const std::string& csv_path);

    // ========================================================================
    // 查询接口
    // ========================================================================
    const FishData* GetFishData(const std::string& fish_id) const;
    const FishingLocation* GetLocationData(const std::string& location_id) const;
    const BaitData* GetBaitData(const std::string& bait_id) const;
    const FishingSkill* GetSkillData(int level) const;

    std::vector<const FishData*> GetAvailableFish(
        const std::string& season,
        const std::string& weather,
        int game_time_minutes,
        const std::string& location_id
    ) const;

    // ========================================================================
    // 钓鱼操作
    // ========================================================================
    FishingResult TryFish(
        const std::string& location_id,
        const std::string& bait_id,
        int current_game_time,
        int fishing_power,
        float luck_modifier
    );

    // ========================================================================
    // 技能
    // ========================================================================
    bool CanLevelUp() const;
    void AddExperience(int exp);
    int GetExperienceForNextLevel() const;
    int GetCurrentExperience() const { return experience_; }

    // ========================================================================
    // 鱼饵管理
    // ========================================================================
    bool EquipBait(const std::string& bait_id, int count);
    void UnequipBait();
    bool HasBait() const { return !equipped_bait_.empty() && bait_count_ > 0; }
    const std::string& GetEquippedBait() const { return equipped_bait_; }
    int GetBaitCount() const { return bait_count_; }

    // ========================================================================
    // 地点冷却
    // ========================================================================
    bool IsLocationAvailable(const std::string& location_id, int current_game_time);
    void RecordFishing(const std::string& location_id, int current_game_time);

    // ========================================================================
    // 统计
    // ========================================================================
    int GetTotalCaught() const { return state_.total_caught; }
    int GetCaughtCount(const std::string& fish_id) const;
    int GetBestWeight(const std::string& fish_id) const;
    int GetSkillLevel() const { return state_.skill_level; }

    // ========================================================================
    // 存档
    // ========================================================================
    void LoadState(const FishingState& state) { state_ = state; }
    const FishingState& GetState() const { return state_; }

    // ========================================================================
    // 难度计算
    // ========================================================================
    int CalculateFinalDifficulty(int base_difficulty, const std::string& location_id) const;

private:
    FishingSystem() = default;
    ~FishingSystem() = default;
    FishingSystem(const FishingSystem&) = delete;
    FishingSystem& operator=(const FishingSystem&) = delete;

    // 数据存储
    std::map<std::string, FishData> fish_data_;
    std::map<std::string, FishingLocation> location_data_;
    std::map<std::string, BaitData> bait_data_;
    std::map<int, FishingSkill> skill_data_;

    // 玩家状态
    FishingState state_;
    int experience_ = 0;

    // 当前装备
    std::string equipped_bait_;
    int bait_count_ = 0;

    // 私有辅助方法
    bool ParseTimeRange(const std::string& range, int& start_minutes, int& end_minutes) const;
    bool IsInTimeRange(int current_minutes, const std::string& range) const;
    bool IsSeasonMatch(const std::string& fish_season, const std::string& current_season) const;
    bool IsWeatherMatch(const std::string& fish_weather, const std::string& current_weather) const;
    float CalculateCatchChance(int difficulty, int fishing_power, float luck_modifier, const BaitData* bait) const;
    int CalculateQuality(float weight, float max_weight, int fishing_power) const;
    int CalculateSellPrice(const FishData& fish, int quality, float weight) const;
};

}  // namespace CloudSeamanor::domain
