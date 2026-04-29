#pragma once

// ============================================================================
// 【GameAppRuntimeTypes】游戏 App 层运行时类型
// ============================================================================
// 定义游戏运行时使用的实体结构体、枚举和子类型。
//
// 设计原则：
// - 本头文件仅存放结构体/枚举声明，不放实现。
// - 如需跨模块共享领域类型（如 TeaPlot），优先放在对应 domain 模块。
// - 涉及多个子模块的跨域类型（如 TeaPlot 涉及农业+UI+存档），归入此处。
// ============================================================================

#include "CloudSeamanor/domain/CropData.hpp"
#include "CloudSeamanor/MathTypes.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::engine {

[[nodiscard]] constexpr std::uint32_t PackRgba(
    std::uint8_t r,
    std::uint8_t g,
    std::uint8_t b,
    std::uint8_t a = 255) noexcept {
    return (static_cast<std::uint32_t>(r) << 24)
        | (static_cast<std::uint32_t>(g) << 16)
        | (static_cast<std::uint32_t>(b) << 8)
        | static_cast<std::uint32_t>(a);
}

// ============================================================================
// 【PlotObstacleType】地块障碍物类型（开垦系统）
// ============================================================================
enum class PlotObstacleType : std::uint8_t {
    None = 0,
    Stone = 1,  // 3 次斧击清除
    Stump = 2,  // 2 次镐击清除
    Weed = 3,   // 1 次镰刀清除
};

enum class TeaPlotLayer : std::uint8_t {
    NormalFarm = 0,
    TeaGardenExclusive = 1,
};

// ============================================================================
// 【TeaPlot】茶田地块运行时状态
// ============================================================================
// 描述一块地从翻土到收获的全流程状态。
// 作物参数（生长时间、阶段数、品质）由 CropTable 驱动。
struct TeaPlot {
    /** 地块几何：世界坐标位置与尺寸（渲染 shape 由 SceneVisualStore 生成）。 */
    CloudSeamanor::domain::Vec2f position{0.0f, 0.0f};
    CloudSeamanor::domain::Vec2f size{52.0f, 52.0f};

    /** 地块视觉数据（渲染层使用）。 */
    std::uint32_t fill_rgba = PackRgba(58, 62, 66);
    std::uint32_t outline_rgba = PackRgba(34, 34, 34);
    float outline_thickness = 2.0f;

    /** 作物唯一标识（对应 CropTable.csv 中的 id）。 */
    std::string crop_id;
    TeaPlotLayer layer = TeaPlotLayer::NormalFarm;

    /** 作物展示名称。 */
    std::string crop_name;

    /** 播种时消耗的物品标识。 */
    std::string seed_item_id;

    /** 收获时产出的物品标识。 */
    std::string harvest_item_id;

    /** 单次收获产出的基础数量。 */
    int harvest_amount = 1;

    /** 作物总生长时间（秒），来自 CropTable。 */
    float growth_time = 80.0f;

    /** 作物总生长阶段数，来自 CropTable。 */
    int growth_stages = 4;

    bool tilled = false;
    bool seeded = false;
    bool watered = false;
    bool ready = false;
    float growth = 0.0f;
    int stage = 0;

    /** 是否已开垦（未开垦时不能翻土/播种）。 */
    bool cleared = true;

    /** 障碍物类型（cleared=false 时有效）。 */
    PlotObstacleType obstacle_type = PlotObstacleType::None;
    /** 障碍物剩余清除次数（cleared=false 时有效）。 */
    int obstacle_hits_left = 0;

    /** 作物品质等级，由云海状态决定。 */
    CloudSeamanor::domain::CropQuality quality =
        CloudSeamanor::domain::CropQuality::Normal;

    /** 是否已放置洒水器。 */
    bool sprinkler_installed = false;
    /** 洒水器剩余天数（<=0 表示失效）。 */
    int sprinkler_days_left = 0;

    /** 是否已施肥。 */
    bool fertilized = false;
    /** 肥料类型：none/basic/premium。 */
    std::string fertilizer_type = "none";

    /** 是否处于温室区域（MVP：按地块标记）。 */
    bool in_greenhouse = false;
    bool disease = false;
    bool pest = false;
    int disease_days = 0;

    // ========== 品质快照系统 ==========
    /** 播种时的云海密度（快照，用于品质判定） */
    float cloud_density_at_planting = 0.0f;
    /** 播种时的灵气值（快照） */
    int aura_at_planting = 0;
    /** 播种时的天气状态（快照） */
    CloudSeamanor::domain::CloudState weather_at_planting = CloudSeamanor::domain::CloudState::Clear;
    /** 累积浓云海天数（每日睡觉时更新） */
    int dense_cloud_days_accumulated = 0;
    /** 累积大潮天数（每日睡觉时更新） */
    int tide_days_accumulated = 0;
    /** 是否处于茶魂花地块（提升品质） */
    bool tea_soul_flower_nearby = false;
    /** 施肥类型（影响品质） */
    std::string fertilizer_type_for_quality = "none";
    /** 是否灵化变种（浓云海/大潮触发） */
    bool spirit_mutated = false;
    /** 收获时的饱食恢复值 */
    int hunger_restore = 0;
    /** 播种时的生态加成（快照，用于品质判定） */
    float ecology_bonus_at_planting = 0.0f;
};

struct PriceTableEntry {
    std::string item_id;
    int buy_price = 0;
    int sell_price = 0;
    std::string buy_from;
    std::string category;
};

struct MailOrderEntry {
    std::string item_id;
    int count = 1;
    int deliver_day = 1;
    bool claimed = false;
    bool opened = false;
    bool receipt_sent = false;
    std::string source_rule_id;
    std::string sender;
    std::string subject;
    std::string body;
    std::string secondary_item_id;
    int secondary_count = 0;
};

struct InnOrderEntry {
    std::string order_id;
    std::string item_id;
    int required_count = 1;
    int reward_gold = 0;
    float npc_visit_weight = 1.0f;
    bool fulfilled = false;
};

struct MailTriggerRule {
    std::string id;
    std::string trigger_type;
    std::string trigger_arg;
    std::string item_id;
    int count = 1;
    int delay_days = 1;
    std::string hint_text;
    bool enabled = true;
    std::string sender_template;
    std::string subject_template;
    std::string body_template;
    std::string cooldown_policy = "daily";
};

// ============================================================================
// 【RepairProject】主屋修缮工程状态
// ============================================================================
struct RepairProject {
    bool completed = false;
    int level = 1;
    int build_days_left = 0;
    int wood_cost = 4;
    int turnip_cost = 2;
    int gold_cost = 0;
};

struct MainHouseUpgradeCost {
    int next_level = 0;
    int wood_cost = 0;
    int turnip_cost = 0;
    int gold_cost = 0;
    int workshop_level = 0;
    int workshop_slots = 0;
    bool unlock_greenhouse = false;
};

constexpr int kMainHouseMaxLevel = 4;

inline MainHouseUpgradeCost QueryMainHouseUpgradeCost(int current_level) {
    switch (current_level + 1) {
    case 2:
        return MainHouseUpgradeCost{
            .next_level = 2,
            .wood_cost = 10,
            .turnip_cost = 2,
            .gold_cost = 500,
            .workshop_level = 2,
            .workshop_slots = 2,
            .unlock_greenhouse = false};
    case 3:
        return MainHouseUpgradeCost{
            .next_level = 3,
            .wood_cost = 20,
            .turnip_cost = 4,
            .gold_cost = 2000,
            .workshop_level = 3,
            .workshop_slots = 4,
            .unlock_greenhouse = true};
    case 4:
        return MainHouseUpgradeCost{
            .next_level = 4,
            .wood_cost = 36,
            .turnip_cost = 8,
            .gold_cost = 6000,
            .workshop_level = 4,
            .workshop_slots = 6,
            .unlock_greenhouse = true};
    default:
        return MainHouseUpgradeCost{};
    }
}

// ============================================================================
// 【TeaMachine】制茶机运行时状态
// ============================================================================
struct TeaMachine {
    bool running = false;
    float progress = 0.0f;
    float duration = 20.0f;
    int queued_output = 0;
};

// ============================================================================
// 【PlacedObject】DIY/摆放对象（P3-002）
// ============================================================================
struct PlacedObject {
    std::string object_id;
    int tile_x = 0;
    int tile_y = 0;
    int rotation = 0;  // 0/90/180/270
    std::string room;  // main_house / tea_room / yard / workshop / tea_garden ...
    std::string custom_data;
};

// ============================================================================
// 【DiaryEntryState】日记条目状态（P2-001）
// ============================================================================
struct DiaryEntryState {
    std::string entry_id;
    int day_unlocked = 0;
    bool has_been_read = false;
};

// ============================================================================
// 【SkillBranchChoice】技能分支选择（P2-002）
// ============================================================================
struct SkillBranchChoice {
    std::string skill_id;
    std::string branch_id;  // A / B / custom id
};

// ============================================================================
// 【SpiritBeastState】灵兽行为状态
// ============================================================================
enum class SpiritBeastState {
    Idle,
    Wander,
    Follow,
    Interact,
};

enum class SpiritBeastPersonality : std::uint8_t {
    Lively = 0,
    Lazy = 1,
    Curious = 2
};

// ============================================================================
// 【HeartParticle】爱心粒子
// ============================================================================
struct HeartParticle {
    CloudSeamanor::domain::Vec2f position;
    CloudSeamanor::domain::Vec2f velocity;
    float radius = 3.0f;
    std::uint32_t color_rgba = PackRgba(255, 120, 170, 220);
    float lifetime = 0.0f;
};

// ============================================================================
// 【SpiritBeast】灵兽实体
// ============================================================================
struct SpiritBeast {
    std::string custom_name = "灵团";
    SpiritBeastPersonality personality = SpiritBeastPersonality::Lively;
    int favor = 0;
    bool dispatched_for_pest_control = false;
    /** 灵兽几何与视觉（渲染 shape 由 SceneVisualStore 生成）。 */
    CloudSeamanor::domain::Vec2f position{620.0f, 250.0f};
    float radius = 22.0f;
    int point_count = 18;
    std::uint32_t fill_rgba = PackRgba(196, 235, 255);
    std::uint32_t outline_rgba = PackRgba(86, 114, 148);
    float outline_thickness = 2.0f;
    SpiritBeastState state = SpiritBeastState::Wander;
    std::vector<CloudSeamanor::domain::Vec2f> patrol_points;
    std::size_t patrol_index = 0;
    float idle_timer = 0.0f;
    float interact_timer = 0.0f;
    bool daily_interacted = false;
    int last_interaction_day = 0;
    std::string trait = "Watering Aid";
    CloudSeamanor::domain::Vec2f home_position{620.0f, 250.0f};
};

// ============================================================================
// 【NpcScheduleEntry】NPC 日程条目
// ============================================================================
struct NpcScheduleEntry {
    int start_minutes = 0;
    int end_minutes = 0;
    std::string location;
    std::string activity;
};

// ============================================================================
// 【NpcGiftPrefs】NPC 礼物偏好
// ============================================================================
struct NpcGiftPrefs {
    std::vector<std::string> loved;
    std::vector<std::string> liked;
    std::vector<std::string> disliked;
};

// ============================================================================
// 【NpcTextMappings】NPC 文本映射表
// ============================================================================
struct NpcTextMappings {
    std::unordered_map<std::string, std::string> names;
    std::unordered_map<std::string, std::string> locations;
    std::unordered_map<std::string, std::string> activities;
};

// ============================================================================
// 【NpcState】NPC 运行状态
// ============================================================================
enum class NpcState {
    Patrol,
    Talking,
};

enum class NpcMood : std::uint8_t {
    Happy,
    Normal,
    Sad,
    Angry
};

// ============================================================================
// 【NpcActor】NPC 角色实体
// ============================================================================
struct NpcActor {
    std::string id;
    std::string display_name;
    /** NPC 几何与视觉（渲染 shape 由 SceneVisualStore 生成）。 */
    CloudSeamanor::domain::Vec2f position{0.0f, 0.0f};
    CloudSeamanor::domain::Vec2f size{24.0f, 38.0f};
    std::uint32_t fill_rgba = PackRgba(180, 180, 180);
    std::uint32_t outline_rgba = PackRgba(96, 96, 96);
    float outline_thickness = 2.0f;
    std::vector<NpcScheduleEntry> schedule;
    NpcGiftPrefs prefs;
    NpcState state = NpcState::Patrol;
    std::string current_location;
    std::string current_activity;
    int favor = 0;
    int heart_level = 0;
    int daily_favor_gain = 0;
    bool daily_gifted = false;
    int last_gift_day = 0;
    bool daily_talked = false;
    int last_talk_day = 0;
    bool visible = true;
    std::uint32_t base_outline_rgba = PackRgba(96, 96, 96);
    NpcMood mood = NpcMood::Normal;
    bool married = false;
    std::string memory_tag;
    int memory_until_day = 0;
    int development_stage = 0;
};

} // namespace CloudSeamanor::engine
