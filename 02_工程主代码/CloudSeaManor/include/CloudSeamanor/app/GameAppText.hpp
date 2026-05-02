#pragma once

#include "CloudSeamanor/engine/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/domain/GameClock.hpp"
#include "CloudSeamanor/domain/Inventory.hpp"

#include <SFML/Graphics/Color.hpp>

#include <string>
#include <vector>

namespace CloudSeamanor::engine {

/**
 * @brief 返回灵兽状态的存档整数值。
 *
 * @param state 灵兽当前状态。
 * @return 存档使用的稳定整数编号。
 */
int SpiritBeastStateToInt(SpiritBeastState state);

/**
 * @brief 从存档整数值恢复灵兽状态。
 *
 * @param value 存档中的状态编号。
 * @return 对应的灵兽状态；未知值回退到 `Wander`。
 */
SpiritBeastState SpiritBeastStateFromInt(int value);

/**
 * @brief 构建底部控制提示文本。
 *
 * @return 当前原型支持的按键说明字符串。
 */
std::string BuildControlsHint();
std::string BuildNpcInteractionHint();

/**
 * @brief 构建当日目标文本。
 *
 * @param main_house_repair 主屋修缮状态。
 * @param inventory 当前库存引用。
 * @param tea_machine 制茶机状态。
 * @param spirit_beast 灵兽状态。
 * @param npcs 当前 NPC 列表。
 * @return 适合直接显示到 HUD 的目标文本。
 */
std::string BuildDailyGoalText(const RepairProject& main_house_repair,
                               const CloudSeamanor::domain::Inventory& inventory,
                               const TeaMachine& tea_machine,
                               const SpiritBeast& spirit_beast,
                               const std::vector<NpcActor>& npcs);

/**
 * @brief 构建"每日推荐三件事"文本列表（完全数据驱动版）。
 *
 * @param clock 当前游戏时钟。
 * @param cloud_state 当前云海状态。
 * @param cloud_system 云海系统（用于动态感知大潮预报、NPC 好感等）。
 * @param main_house_repair 主屋修缮状态。
 * @param inventory 当前库存引用。
 * @param tea_machine 制茶机状态。
 * @param spirit_beast 灵兽状态。
 * @param spirit_beast_watered_today 灵兽今日协助是否已使用。
 * @param tea_plots 当前茶田列表。
 * @param npcs 当前 NPC 列表。
 * @return 固定返回 3 条推荐：生产 / 社交 / 养成。
 *
 * @note 完全数据驱动：
 * - 生产推荐根据云海状态（大潮优先收获）、地块成熟度、库存和制茶机状态动态生成
 * - 社交推荐根据未送礼 NPC 优先级、库存茶包数量和时段动态生成
 * - 养成推荐根据灵兽互动状态和时段动态生成
 */
std::vector<std::string> BuildDailyRecommendations(
    const CloudSeamanor::domain::GameClock& clock,
    CloudSeamanor::domain::CloudState cloud_state,
    const CloudSeamanor::domain::CloudSystem& cloud_system,
    const RepairProject& main_house_repair,
    const CloudSeamanor::domain::Inventory& inventory,
    const TeaMachine& tea_machine,
    const SpiritBeast& spirit_beast,
    bool spirit_beast_watered_today,
    const std::vector<TeaPlot>& tea_plots,
    const std::vector<NpcActor>& npcs);

/**
 * @brief 根据云海状态构建天气建议文本。
 *
 * @param state 当前云海状态。
 * @param forecast_visible 次日预报当前是否已经公开。
 * @return 对应天气建议说明。
 */
std::string BuildWeatherAdviceText(CloudSeamanor::domain::CloudState state,
                                   bool forecast_visible = true);

/**
 * @brief 返回昼夜阶段文本。
 *
 * @param phase 当前昼夜阶段。
 * @return 阶段中文名称。
 */
std::string PhaseText(CloudSeamanor::domain::DayPhase phase);

/**
 * @brief 返回昼夜阶段的环境色调。
 *
 * @param phase 当前昼夜阶段。
 * @return 叠加到画面上的阶段色调。
 */
sf::Color PhaseTint(CloudSeamanor::domain::DayPhase phase);

/**
 * @brief 返回不同物品的掉落物颜色。
 *
 * @param item_id 物品标识。
 * @return 该物品应使用的颜色。
 */
sf::Color PickupColorFor(const std::string& item_id);

/**
 * @brief 把内部物品标识转换成显示名称。
 *
 * @param item_id 物品标识。
 * @return 用于 HUD 和提示文本的显示名称。
 */
std::string ItemDisplayName(const std::string& item_id);

/**
 * @brief 返回灵兽状态的调试文本。
 *
 * @param state 灵兽当前状态。
 * @return 状态对应的中文文本。
 */
const char* SpiritBeastStateText(SpiritBeastState state);
const char* SpiritBeastPersonalityText(SpiritBeastPersonality personality);
int NpcHeartLevelFromFavor(int favor);
std::string NpcHeartText(int heart_level);
std::string NpcCloudStageText(int heart_level);

/**
 * @brief 叠加两个颜色得到环境覆盖色。
 *
 * @param base 基础颜色。
 * @param tint 附加色调。
 * @return 混合后的颜色结果。
 */
sf::Color CombineOverlayColor(sf::Color base, sf::Color tint);

/**
 * @brief 返回背景基础颜色。
 *
 * @param state 当前云海状态。
 * @return 背景应使用的主色。
 */
sf::Color BackgroundColorFor(CloudSeamanor::domain::CloudState state);

/**
 * @brief 返回云海状态带来的作物生长倍率。
 *
 * @param state 当前云海状态。
 * @return 生长倍率。
 */
float CloudGrowthMultiplier(CloudSeamanor::domain::CloudState state);

/**
 * @brief 返回灵气覆盖层颜色。
 *
 * @param state 当前云海状态。
 * @return 覆盖层颜色。
 */
sf::Color AuraColorFor(CloudSeamanor::domain::CloudState state);

} // namespace CloudSeamanor::engine
