#pragma once

// ============================================================================
// 【GameAppFarming.hpp】农业系统 UI 层
// ============================================================================
// 提供农业相关的 UI 构建、视觉刷新和统计函数。
//
// 主要职责：
// - 刷新地块视觉状态（颜色、高亮效果）
// - 构建默认茶田地块布局
// - 提供地块相关的统计函数
//
// 设计原则：
// - UI 层函数与领域逻辑分离
// - 视觉更新通过回调函数实现，便于解耦
// ============================================================================

#include "CloudSeamanor/CropData.hpp"
#include "CloudSeamanor/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/CloudSystem.hpp"
#include "CloudSeamanor/GameClock.hpp"
#include "CloudSeamanor/Inventory.hpp"
#include "CloudSeamanor/Stamina.hpp"
#include "CloudSeamanor/SkillSystem.hpp"

#include <functional>
#include <string>
#include <vector>

namespace CloudSeamanor::engine {

// ============================================================================
// 【RefreshTeaPlotVisual】刷新地块视觉状态
// ============================================================================
/**
 * @brief 根据地块当前状态刷新视觉表现
 * 
 * 视觉规则：
 * - 未翻土：显示为深绿色
 * - 已翻土未播种：显示为棕色
 * - 已播种未浇水：显示为浅棕色
 * - 已浇水未成熟：渐变绿色，根据生长阶段
 * - 已成熟：显示为亮绿色
 * 
 * @param plot        目标地块引用
 * @param highlighted 是否高亮（高亮时显示白色边框）
 */
void RefreshTeaPlotVisual(TeaPlot& plot, bool highlighted);

// ============================================================================
// 【BuildTeaPlots】构建默认地块列表
// ============================================================================
/**
 * @brief 创建游戏初始的茶田地块布局
 * 
 * 默认布局：
 * - 3 块茶田，水平排列
 * - 前 2 块为云雾茶（茶叶种子 x4 可种 2 块）
 * - 第 3 块为萝卜（萝卜种子 x2 可种 1 块）
 * 
 * @param tea_plots 输出参数，存储构建的地块列表
 */
void BuildTeaPlots(std::vector<TeaPlot>& tea_plots);

// ============================================================================
// 【UpdateCropGrowth】更新作物生长
// ============================================================================
/**
 * @brief 每帧更新所有地块的作物生长状态
 * 
 * 生长逻辑：
 * 1. 仅更新已播种且已浇水的地块
 * 2. 生长速度受云海倍率、技能加成、灵兽协助影响
 * 3. 达到生长阈值时标记为可收获
 * 
 * @param plots             地块列表引用
 * @param delta_seconds     距离上次更新的时间（秒）
 * @param cloud_multiplier  云海生长倍率
 * @param spirit_buff       技能灵气加成
 * @param beast_share       灵兽协助加成（互动过为 1.2，否则 1.0）
 * @param refresh_visual    刷新地块视觉的回调
 * @param push_hint        推送提示的回调
 * @param log_info         日志记录回调
 * @return 是否有地块从非成熟变为成熟
 */
bool UpdateCropGrowth(
    std::vector<TeaPlot>& plots,
    float delta_seconds,
    float cloud_multiplier,
    float spirit_buff,
    float beast_share,
    const std::function<void(TeaPlot&, bool)>& refresh_visual,
    const std::function<void(const std::string&, float)>& push_hint,
    const std::function<void(const std::string&)>& log_info
);

// ============================================================================
// 【HandlePlotInteraction】处理地块交互
// ============================================================================
/**
 * @brief 处理玩家与地块的交互
 * 
 * 交互状态机（按顺序检查）：
 * 1. 未翻土 -> 翻土
 * 2. 已翻土未播种 -> 消耗种子并播种
 * 3. 已播种未浇水 -> 标记为已浇水
 * 4. 已浇水已成熟 -> 收获并添加到背包
 * 5. 其他情况 -> 显示当前状态提示
 * 
 * @param plot              目标地块引用
 * @param inventory         背包引用（用于消耗种子、添加产物）
 * @param skills            技能系统引用（用于添加经验）
 * @param cloud_density     云海密度（影响经验倍率）
 * @param beast_interacted  灵兽今日是否已互动（影响经验倍率）
 * @param refresh_visual    刷新地块视觉的回调
 * @param push_hint         推送提示的回调
 * @param log_info          日志记录回调
 * @return 是否成功处理了交互
 */
bool HandlePlotInteraction(
    TeaPlot& plot,
    CloudSeamanor::domain::Inventory& inventory,
    CloudSeamanor::domain::SkillSystem& skills,
    float cloud_density,
    bool beast_interacted,
    const std::function<void(TeaPlot&, bool)>& refresh_visual,
    const std::function<void(const std::string&, float)>& push_hint,
    const std::function<void(const std::string&)>& log_info
);

// ============================================================================
// 【PlotStatusText】获取地块状态文本
// ============================================================================
/**
 * @brief 根据地块当前状态生成描述文本
 * 
 * 状态映射：
 * - 未翻土：未开垦
 * - 已翻土未播种：已开垦，等待播种
 * - 已播种未浇水：已播种，需要浇水
 * - 已浇水未成熟：生长中，第 X 阶段
 * - 已成熟：可以收获
 * 
 * @param plot 目标地块
 * @return 描述地块当前状态的文本
 */
std::string PlotStatusText(const TeaPlot& plot);

// ============================================================================
// 【CountReadyPlots】统计可收获地块数量
// ============================================================================
/**
 * @brief 统计当前有多少地块已达到可收获状态
 * 
 * @param plots 地块列表引用
 * @return 可收获地块的数量
 */
int CountReadyPlots(const std::vector<TeaPlot>& plots);

// ============================================================================
// 【CountDryPlots】统计缺水地块数量
// ============================================================================
/**
 * @brief 统计当前有多少已播种但未浇水地块
 * 
 * 用于提示玩家哪些地块需要浇水
 * 
 * @param plots 地块列表引用
 * @return 缺水地块的数量
 */
int CountDryPlots(const std::vector<TeaPlot>& plots);

// ============================================================================
// 【CountGrowingPlots】统计生长中地块数量
// ============================================================================
/**
 * @brief 统计当前有多少地块处于生长中
 * 
 * 生长中定义：已播种且已浇水，但未成熟
 * 
 * @param plots 地块列表引用
 * @return 生长中地块的数量
 */
int CountGrowingPlots(const std::vector<TeaPlot>& plots);

} // namespace CloudSeamanor::engine
