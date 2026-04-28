#pragma once

// ============================================================================
// 【TargetHintRuntime.hpp】目标提示运行时
// ============================================================================
// 提供生成目标提示文本所需的上下文结构体和函数声明。
//
// 主要职责：
// - 定义 TargetHintContext 结构体，描述当前高亮目标
// - 提供 BuildCurrentTargetText 函数，生成面向 HUD 的提示文本
//
// 设计原则：
// - 上下文只描述"当前高亮对象 + 相关状态"，不含业务逻辑
// - 所有数据通过只读引用传递，确保数据不被意外修改
// ============================================================================

#include "CloudSeamanor/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/Interactable.hpp"
#include "CloudSeamanor/Inventory.hpp"
#include "CloudSeamanor/TeaBush.hpp"
#include "CloudSeamanor/WorkshopSystem.hpp"

#include <string>
#include <vector>

namespace CloudSeamanor::engine {

// ============================================================================
// 【TargetHintContext】目标提示上下文
// ============================================================================
// 描述当前高亮对象及相关状态，用于生成提示文本。
//
// 用途：
// - 将当前高亮状态传递给提示生成函数
// - 支持根据不同高亮对象生成不同的提示文本
//
// 设计决策：
// - 所有数据成员使用 const 引用，确保只读访问
// - 不包含任何回调或函数指针，保持简单数据结构
// - 高亮索引使用 -1 表示"无高亮"
// ============================================================================
struct TargetHintContext {
    // ============================================================================
    // 【系统引用】
    // ============================================================================
    /** 背包系统引用：用于检查物品数量以生成准确提示 */
    const CloudSeamanor::domain::Inventory& inventory;
    
    /** 工坊系统引用：用于检查制茶机运行状态 */
    const CloudSeamanor::domain::WorkshopSystem& workshop;

    // ============================================================================
    // 【游戏对象列表】
    // ============================================================================
    /** 茶田地块列表：用于检查地块状态和作物信息 */
    const std::vector<TeaPlot>& tea_plots;
    const std::vector<CloudSeamanor::domain::TeaBush>& tea_bushes;
    
    /** NPC 列表：用于检查 NPC 名称和状态 */
    const std::vector<NpcActor>& npcs;
    
    /** 灵兽状态：用于检查灵兽交互状态 */
    const SpiritBeast& spirit_beast;
    
    /** 场景可交互对象列表：用于检查可采集节点、工作台等 */
    const std::vector<CloudSeamanor::domain::Interactable>& interactables;

    // ============================================================================
    // 【高亮状态】
    // ============================================================================
    /** 灵兽今日是否已浇水标记 */
    bool spirit_beast_watered_today = false;
    
    /** 当前高亮的茶田地块索引，-1 表示无高亮 */
    int highlighted_plot_index = -1;
    
    /** 当前高亮的 NPC 索引，-1 表示无高亮 */
    int highlighted_npc_index = -1;
    
    /** 灵兽是否处于高亮状态 */
    bool spirit_beast_highlighted = false;
    
    /** 当前高亮的一般可交互对象索引，-1 表示无高亮 */
    int highlighted_index = -1;
    int current_day = 1;
    int current_hour = 0;
    int player_gold = 0;

    // ============================================================================
    // 【建筑/设施状态】
    // ============================================================================
    /** 主屋修缮项目状态：用于判断是否可以继续修缮 */
    const RepairProject& main_house_repair;
    
    /** 制茶机状态：用于判断是否可以启动加工 */
    const TeaMachine& tea_machine;
};

// ============================================================================
// 【BuildCurrentTargetText】生成当前目标提示文本
// ============================================================================
/**
 * @brief 根据当前高亮状态生成目标提示文本
 * 
 * 提示优先级：
 * 1. 高亮地块时：显示作物状态和下一步操作
 * 2. 高亮 NPC 时：显示 NPC 名称和对话
 * 3. 高亮灵兽时：显示灵兽状态和互动提示
 * 4. 高亮交互对象时：显示交互对象类型和操作
 * 5. 无高亮时：显示通用提示
 * 
 * @param context 目标提示上下文，包含当前高亮状态
 * @return 面向 HUD/窗口标题的目标提示文本
 */
std::string BuildCurrentTargetText(const TargetHintContext& context);

} // namespace CloudSeamanor::engine
