#pragma once

#include "CloudSeamanor/CloudSystem.hpp"
#include "CloudSeamanor/DynamicLifeSystem.hpp"
#include "CloudSeamanor/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/GameClock.hpp"
#include "CloudSeamanor/HungerSystem.hpp"
#include "CloudSeamanor/Inventory.hpp"
#include "CloudSeamanor/Player.hpp"
#include "CloudSeamanor/SkillSystem.hpp"
#include "CloudSeamanor/Stamina.hpp"
#include "CloudSeamanor/FestivalSystem.hpp"
#include "CloudSeamanor/WorkshopSystem.hpp"
#include "CloudSeamanor/RelationshipSystem.hpp"

#include <SFML/Graphics/RectangleShape.hpp>

#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::engine {

class NpcDialogueManager;
struct TutorialState;

/**
 * @brief 将当前运行时状态写入存档文件。
 *
 * @param save_path 目标存档路径。
 * @param clock 当前游戏时钟。
 * @param cloud_system 当前云海系统状态。
 * @param player 当前玩家状态。
 * @param stamina 当前体力状态。
 * @param main_house_repair 主屋修缮状态。
 * @param tea_machine 制茶机状态。
 * @param spirit_beast 灵兽运行时状态。
 * @param spirit_beast_watered_today 灵兽今日是否已触发协助。
 * @param tea_plots 当前所有茶田地块。
 * @param inventory 当前库存。
 * @param npcs 当前 NPC 列表。
 * @param push_hint 用于推送提示文本的回调。
 * @param skills 技能系统状态。
 * @param festivals 节日系统状态。
 * @param dynamic_life 动态人生系统状态。
 * @param workshop 工坊系统状态。
 * @return `true` 表示存档成功写入。
 */
bool SaveGameState(const std::filesystem::path& save_path,
                   const CloudSeamanor::domain::GameClock& clock,
                   const CloudSeamanor::domain::CloudSystem& cloud_system,
                   const CloudSeamanor::domain::Player& player,
                   const CloudSeamanor::domain::StaminaSystem& stamina,
                   const CloudSeamanor::domain::HungerSystem& hunger,
                   const RepairProject& main_house_repair,
                   const TeaMachine& tea_machine,
                   const SpiritBeast& spirit_beast,
                   bool spirit_beast_watered_today,
                   const std::vector<TeaPlot>& tea_plots,
                   int gold,
                   const std::vector<PriceTableEntry>& price_table,
                   const std::vector<MailOrderEntry>& mail_orders,
                   const CloudSeamanor::domain::Inventory& inventory,
                   const std::vector<NpcActor>& npcs,
                   const std::function<void(const std::string&, float)>& push_hint,
                   const CloudSeamanor::domain::SkillSystem* skills = nullptr,
                   const CloudSeamanor::domain::FestivalSystem* festivals = nullptr,
                   const CloudSeamanor::domain::DynamicLifeSystem* dynamic_life = nullptr,
                   const CloudSeamanor::domain::WorkshopSystem* workshop = nullptr,
                   const CloudSeamanor::domain::RelationshipState* relationship = nullptr,
                   const int* decoration_score = nullptr,
                   const std::string* pet_type = nullptr,
                   const bool* pet_adopted = nullptr,
                   const std::unordered_map<std::string, bool>* achievements = nullptr,
                   const std::unordered_map<std::string, int>* weekly_buy_count = nullptr,
                   const std::unordered_map<std::string, int>* weekly_sell_count = nullptr,
                   const std::vector<InnOrderEntry>* inn_orders = nullptr,
                   const int* inn_visitors_today = nullptr,
                   const int* inn_income_today = nullptr,
                   const int* inn_reputation = nullptr,
                   const int* coop_fed_today = nullptr,
                   const int* livestock_eggs_today = nullptr,
                   const int* livestock_milk_today = nullptr,
                   const int* spirit_realm_daily_max = nullptr,
                   const int* spirit_realm_daily_remaining = nullptr,
                   const bool* in_battle_mode = nullptr,
                   const int* battle_state = nullptr,
                   const bool* battle_available = nullptr,
                   const std::vector<std::string>* battle_active_partners = nullptr,
                   const std::string* equipped_weapon_id = nullptr,
                   const TutorialState* tutorial = nullptr,
                   const std::vector<DiaryEntryState>* diary_entries = nullptr,
                   const std::unordered_map<std::string, bool>* recipe_unlocks = nullptr,
                   const std::unordered_map<std::string, std::string>* skill_branches = nullptr,
                   const std::vector<std::string>* pending_skill_branches = nullptr,
                   const std::vector<PlacedObject>* placed_objects = nullptr,
                   const int* purify_return_days = nullptr,
                   const int* purify_return_spirits = nullptr,
                   const int* fishing_attempts = nullptr,
                   const std::string* last_fish_catch = nullptr);

// ============================================================================
// 【J18+ 存档预留：装饰实体摆放】
// ============================================================================
// 目前装饰系统仅持久化 `decor|<score>`（装饰评分）。
// 为避免未来加入“家具摆放实体”时破坏存档格式，预留行格式如下（当前版本可写可不写，读档会忽略未知行）：
//
// - `decor_item|<item_id>|<x>|<y>|<rot>|<room>`
//   - item_id: 家具/装饰物 id（如 furniture_table）
//   - x,y: 世界坐标或房间内坐标（float 或 int，建议统一 float）
//   - rot: 旋转/朝向（0/90/180/270 或枚举）
//   - room: 区域标识（如 main_house / inn / yard）

/**
 * @brief 从存档文件恢复当前运行时状态。
 *
 * @param save_path 目标存档路径。
 * @param clock 待恢复的游戏时钟。
 * @param cloud_system 待恢复的云海系统状态。
 * @param player 待恢复的玩家状态。
 * @param stamina 待恢复的体力状态。
 * @param main_house_repair 待恢复的主屋修缮状态。
 * @param tea_machine 待恢复的制茶机状态。
 * @param spirit_beast 待恢复的灵兽状态。
 * @param spirit_beast_watered_today 待恢复的灵兽每日协助标记。
 * @param tea_plots 待恢复的茶田地块列表。
 * @param inventory 待恢复的库存对象。
 * @param npcs 待恢复的 NPC 列表。
 * @param obstacle_shapes 当前场景障碍形状列表，用于同步主屋外观。
 * @param last_cloud_state 待恢复的云海缓存状态。
 * @param refresh_spirit_beast_visual 用于刷新灵兽表现的回调。
 * @param refresh_tea_plot_visual 用于刷新地块表现的回调。
 * @param update_highlighted_interactable 用于重算交互高亮的回调。
 * @param update_hud_text 用于刷新 HUD 的回调。
 * @param push_hint 用于推送提示文本的回调。
 * @param skills 技能系统状态（可为空）。
 * @param festivals 节日系统状态（可为空）。
 * @param dynamic_life 动态人生系统状态（可为空）。
 * @param workshop 工坊系统状态（可为空）。
 * @return `true` 表示读档成功。
 */
bool LoadGameState(const std::filesystem::path& save_path,
                   CloudSeamanor::domain::GameClock& clock,
                   CloudSeamanor::domain::CloudSystem& cloud_system,
                   CloudSeamanor::domain::Player& player,
                   CloudSeamanor::domain::StaminaSystem& stamina,
                   CloudSeamanor::domain::HungerSystem& hunger,
                   RepairProject& main_house_repair,
                   TeaMachine& tea_machine,
                   SpiritBeast& spirit_beast,
                   bool& spirit_beast_watered_today,
                   std::vector<TeaPlot>& tea_plots,
                   int& gold,
                   std::vector<PriceTableEntry>& price_table,
                   std::vector<MailOrderEntry>& mail_orders,
                   CloudSeamanor::domain::Inventory& inventory,
                   std::vector<NpcActor>& npcs,
                   std::vector<sf::RectangleShape>& obstacle_shapes,
                   CloudSeamanor::domain::CloudState& last_cloud_state,
                   const std::function<void()>& refresh_spirit_beast_visual,
                   const std::function<void(TeaPlot&, bool)>& refresh_tea_plot_visual,
                   const std::function<void()>& update_highlighted_interactable,
                   const std::function<void()>& update_hud_text,
                   const std::function<void(const std::string&, float)>& push_hint,
                   CloudSeamanor::domain::SkillSystem* skills = nullptr,
                   CloudSeamanor::domain::FestivalSystem* festivals = nullptr,
                   CloudSeamanor::domain::DynamicLifeSystem* dynamic_life = nullptr,
                   CloudSeamanor::domain::WorkshopSystem* workshop = nullptr,
                   CloudSeamanor::domain::RelationshipState* relationship = nullptr,
                   NpcDialogueManager* dialogue_manager = nullptr,
                   int* decoration_score = nullptr,
                   std::string* pet_type = nullptr,
                   bool* pet_adopted = nullptr,
                   std::unordered_map<std::string, bool>* achievements = nullptr,
                   std::unordered_map<std::string, int>* weekly_buy_count = nullptr,
                   std::unordered_map<std::string, int>* weekly_sell_count = nullptr,
                   std::vector<InnOrderEntry>* inn_orders = nullptr,
                   int* inn_visitors_today = nullptr,
                   int* inn_income_today = nullptr,
                   int* inn_reputation = nullptr,
                   int* coop_fed_today = nullptr,
                   int* livestock_eggs_today = nullptr,
                   int* livestock_milk_today = nullptr,
                   int* spirit_realm_daily_max = nullptr,
                   int* spirit_realm_daily_remaining = nullptr,
                   bool* in_battle_mode = nullptr,
                   int* battle_state = nullptr,
                   bool* battle_available = nullptr,
                   std::vector<std::string>* battle_active_partners = nullptr,
                   std::string* equipped_weapon_id = nullptr,
                   TutorialState* tutorial = nullptr,
                   std::vector<DiaryEntryState>* diary_entries = nullptr,
                   std::unordered_map<std::string, bool>* recipe_unlocks = nullptr,
                   std::unordered_map<std::string, std::string>* skill_branches = nullptr,
                   std::vector<std::string>* pending_skill_branches = nullptr,
                   std::vector<PlacedObject>* placed_objects = nullptr,
                   int* purify_return_days = nullptr,
                   int* purify_return_spirits = nullptr,
                   int* fishing_attempts = nullptr,
                   std::string* last_fish_catch = nullptr);

} // namespace CloudSeamanor::engine
