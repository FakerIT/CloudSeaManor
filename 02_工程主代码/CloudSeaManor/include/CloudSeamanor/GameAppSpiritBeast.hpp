#pragma once

#include "CloudSeamanor/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/GameClock.hpp"
#include "CloudSeamanor/Stamina.hpp"

#include <SFML/System/Vector2.hpp>

#include <functional>
#include <string>
#include <vector>

namespace CloudSeamanor::engine {

/**
 * @brief 根据灵兽当前状态刷新灵兽外观。
 *
 * @param spirit_beast 目标灵兽实体。
 * @param highlighted 灵兽当前是否被高亮。
 */
void RefreshSpiritBeastVisual(SpiritBeast& spirit_beast, bool highlighted);

/**
 * @brief 构建原型场景中的默认灵兽状态。
 *
 * @param spirit_beast 输出灵兽实体。
 * @param clock 当前游戏时钟，用于记录初始日期。
 * @param spirit_beast_watered_today 输出今日协助是否已使用。
 * @param spirit_beast_highlighted 输出高亮状态。
 */
void BuildSpiritBeast(SpiritBeast& spirit_beast,
                      const CloudSeamanor::domain::GameClock& clock,
                      bool& spirit_beast_watered_today,
                      bool& spirit_beast_highlighted);

/**
 * @brief 在指定位置生成一组爱心粒子。
 *
 * @param center 粒子发射中心点。
 * @param heart_particles 输出粒子列表。
 */
void SpawnHeartParticles(sf::Vector2f center, std::vector<HeartParticle>& heart_particles);

/**
 * @brief 触发灵兽的每日协助能力。
 *
 * @param spirit_beast 当前灵兽实体。
 * @param spirit_beast_watered_today 今日是否已经使用过协助。
 * @param tea_plots 当前茶田列表。
 * @param stamina 玩家体力对象。
 * @param refresh_tea_plot_visual 用于刷新地块外观的回调。
 * @param push_hint 用于推送提示文本的回调。
 * @param log_info 用于记录普通日志的回调。
 */
void TrySpiritBeastWateringAid(SpiritBeast& spirit_beast,
                               bool& spirit_beast_watered_today,
                               std::vector<TeaPlot>& tea_plots,
                               CloudSeamanor::domain::StaminaSystem& stamina,
                               const std::function<void(TeaPlot&, bool)>& refresh_tea_plot_visual,
                               const std::function<void(const std::string&, float)>& push_hint,
                               const std::function<void(const std::string&)>& log_info);

} // namespace CloudSeamanor::engine
