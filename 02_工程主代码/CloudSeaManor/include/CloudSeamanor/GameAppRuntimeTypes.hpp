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

#include "CloudSeamanor/CropData.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Vector2.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::engine {

// ============================================================================
// 【PlotObstacleType】地块障碍物类型（开垦系统）
// ============================================================================
enum class PlotObstacleType : std::uint8_t {
    None = 0,
    Stone = 1,  // 3 次斧击清除
    Stump = 2,  // 2 次镐击清除
    Weed = 3,   // 1 次镰刀清除
};

// ============================================================================
// 【TeaPlot】茶田地块运行时状态
// ============================================================================
// 描述一块地从翻土到收获的全流程状态。
// 作物参数（生长时间、阶段数、品质）由 CropTable 驱动。
struct TeaPlot {
    /** 地块在世界中的可视矩形。 */
    sf::RectangleShape shape;

    /** 作物唯一标识（对应 CropTable.csv 中的 id）。 */
    std::string crop_id;

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
    sf::CircleShape shape;
    sf::Vector2f velocity;
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
    sf::CircleShape shape;
    SpiritBeastState state = SpiritBeastState::Wander;
    std::vector<sf::Vector2f> patrol_points;
    std::size_t patrol_index = 0;
    float idle_timer = 0.0f;
    float interact_timer = 0.0f;
    bool daily_interacted = false;
    int last_interaction_day = 0;
    std::string trait = "Watering Aid";
    sf::Vector2f home_position{620.0f, 250.0f};
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
    sf::RectangleShape shape;
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
    sf::Color base_outline = sf::Color(96, 96, 96);
    NpcMood mood = NpcMood::Normal;
    bool married = false;
};

} // namespace CloudSeamanor::engine
