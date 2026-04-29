#pragma once

#include "CloudSeamanor/engine/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/domain/GameClock.hpp"
#include "CloudSeamanor/domain/Inventory.hpp"
#include "CloudSeamanor/engine/PickupDrop.hpp"
#include "CloudSeamanor/domain/Player.hpp"
#include "CloudSeamanor/domain/SkillSystem.hpp"
#include "CloudSeamanor/domain/Stamina.hpp"
#include "CloudSeamanor/domain/FestivalSystem.hpp"
#include "CloudSeamanor/domain/DynamicLifeSystem.hpp"
#include "CloudSeamanor/domain/WorkshopSystem.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>

#include <string>
#include <vector>

namespace CloudSeamanor::engine {

/**
 * @brief HUD 文本缓存器 - 避免每帧重建字符串
 * 
 * 性能优化：只在关键数据变化时更新 HUD 文本，而不是每帧都重建
 */
class HudTextCache {
public:
    bool NeedsHudUpdate(int current_day, float current_hour, int current_cloud_state) const {
        return current_day != last_day_ ||
               std::abs(current_hour - last_hour_) >= 0.5f ||
               current_cloud_state != last_cloud_state_;
    }

    bool NeedsInventoryUpdate(int current_day) const {
        return current_day != last_inventory_day_;
    }

    void MarkHudUpdated(int day, float hour, int cloud_state) {
        last_day_ = day;
        last_hour_ = hour;
        last_cloud_state_ = cloud_state;
    }

    void MarkInventoryUpdated(int day) {
        last_inventory_day_ = day;
    }

private:
    int last_day_ = -1;
    float last_hour_ = -1.0f;
    int last_cloud_state_ = -1;
    int last_inventory_day_ = -1;
};

/**
 * @brief 刷新 HUD、背包、调试与世界提示文本。
 *
 * @param font_loaded 字体是否已经成功加载。
 * @param hud_text 左上主 HUD 文本对象。
 * @param inventory_text 背包文本对象。
 * @param hint_text 底部提示文本对象。
 * @param dialogue_overlay_text 对话面板文本对象。
 * @param debug_text 调试文本对象。
 * @param world_tip_text 世界提示文本对象。
 * @param clock 当前游戏时钟。
 * @param cloud_system 当前云海系统。
 * @param stamina 当前体力对象。
 * @param player 当前玩家对象。
 * @param inventory 当前库存对象。
 * @param tea_plots 当前茶田列表。
 * @param main_house_repair 主屋修缮状态。
 * @param tea_machine 制茶机状态。
 * @param spirit_beast 当前灵兽状态。
 * @param spirit_beast_watered_today 灵兽今日协助是否已使用。
 * @param npcs 当前 NPC 列表。
 * @param highlighted_plot_index 当前高亮茶田索引。
 * @param highlighted_npc_index 当前高亮 NPC 索引。
 * @param hint_message 当前提示文本。
 * @param session_time 当前会话总时长。
 * @param current_target_text 当前目标说明文本。
 * @param dialogue_text 当前对话文本。
 * @param pickup_count 当前掉落物数量。
 * @param skills 技能系统引用（可为空指针表示未初始化）。
 * @param festivals 节日系统引用（可为空指针表示未初始化）。
 * @param festival_notice_text 节日预告文本。
 * @param dynamic_life 动态人生系统引用（可为空指针表示未初始化）。
 * @param workshop 工坊系统引用（可为空指针表示未初始化）。
 */
void UpdateHudText(bool font_loaded,
                   sf::Text& hud_text,
                   sf::Text& inventory_text,
                   sf::Text& hint_text,
                   sf::Text& dialogue_overlay_text,
                   sf::Text& debug_text,
                   sf::Text& world_tip_text,
                   const CloudSeamanor::domain::GameClock& clock,
                   const CloudSeamanor::domain::CloudSystem& cloud_system,
                   const CloudSeamanor::domain::StaminaSystem& stamina,
                   const CloudSeamanor::domain::Player& player,
                   const CloudSeamanor::domain::Inventory& inventory,
                   const std::vector<TeaPlot>& tea_plots,
                   const RepairProject& main_house_repair,
                   const TeaMachine& tea_machine,
                   const SpiritBeast& spirit_beast,
                   bool spirit_beast_watered_today,
                   const std::vector<NpcActor>& npcs,
                   int highlighted_plot_index,
                   int highlighted_npc_index,
                   const std::string& hint_message,
                   float session_time,
                   const std::string& current_target_text,
                   const std::string& dialogue_text,
                   std::size_t pickup_count,
                   const CloudSeamanor::domain::SkillSystem* skills = nullptr,
                   const CloudSeamanor::domain::FestivalSystem* festivals = nullptr,
                   const std::string& festival_notice_text = "",
                   const CloudSeamanor::domain::DynamicLifeSystem* dynamic_life = nullptr,
                   const CloudSeamanor::domain::WorkshopSystem* workshop = nullptr);

/**
 * @brief 根据当前运行时状态刷新窗口标题文本。
 *
 * @param window 目标窗口对象。
 * @param clock 当前游戏时钟。
 * @param cloud_system 当前云海系统。
 * @param player 当前玩家对象。
 * @param stamina 当前体力对象。
 * @param main_house_repair 主屋修缮状态。
 * @param tea_machine 制茶机状态。
 * @param spirit_beast 当前灵兽状态。
 * @param npcs 当前 NPC 列表。
 * @param pickups 当前掉落物列表。
 * @param highlighted_index 当前高亮交互对象索引。
 * @param current_target_text 当前目标说明文本。
 * @param can_sleep_now 当前是否允许睡觉。
 * @param skills 技能系统引用（可为空指针表示未初始化）。
 * @param festivals 节日系统引用（可为空指针表示未初始化）。
 */
void RefreshWindowTitle(sf::RenderWindow& window,
                        const CloudSeamanor::domain::GameClock& clock,
                        const CloudSeamanor::domain::CloudSystem& cloud_system,
                        const CloudSeamanor::domain::Player& player,
                        const CloudSeamanor::domain::StaminaSystem& stamina,
                        const RepairProject& main_house_repair,
                        const TeaMachine& tea_machine,
                        const SpiritBeast& spirit_beast,
                        const std::vector<NpcActor>& npcs,
                        const std::vector<CloudSeamanor::domain::PickupDrop>& pickups,
                        int highlighted_index,
                        const std::string& current_target_text,
                        bool can_sleep_now,
                        const CloudSeamanor::domain::SkillSystem* skills = nullptr,
                        const CloudSeamanor::domain::FestivalSystem* festivals = nullptr);

} // namespace CloudSeamanor::engine
