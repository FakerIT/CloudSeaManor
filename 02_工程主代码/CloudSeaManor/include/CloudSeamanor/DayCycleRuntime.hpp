#pragma once

// ============================================================================
// 【DayCycleRuntime.hpp】跨日运行时
// ============================================================================
// 提供跨日刷新逻辑所需的上下文结构体和函数声明。
//
// 主要职责：
// - 定义 DayCycleRuntimeContext 结构体，统一管理跨日刷新所需的所有状态
// - 提供 HandleNaturalDayTransition 函数（处理自然跨日）
// - 提供 SleepToNextMorning 函数（处理主动睡眠跨日）
//
// 设计原则：
// - 所有跨日状态通过上下文结构体传递，避免全局变量
// - 使用构造函数确保所有引用成员都被正确初始化
// - 回调函数使用 std::function，支持灵活的事件处理
// ============================================================================

#include "CloudSeamanor/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/CloudGuardianContract.hpp"
#include "CloudSeamanor/CloudSystem.hpp"
#include "CloudSeamanor/DynamicLifeSystem.hpp"
#include "CloudSeamanor/FestivalSystem.hpp"
#include "CloudSeamanor/GameClock.hpp"
#include "CloudSeamanor/Stamina.hpp"
#include "CloudSeamanor/WorkshopSystem.hpp"

#include <functional>
#include <string>
#include <vector>

namespace CloudSeamanor::engine {

// ============================================================================
// 【DayCycleRuntimeContext】跨日刷新运行时上下文
// ============================================================================
// 收敛"自然跨日"和"主动睡眠跨日"的共享逻辑输入。
//
// 用途：
// - 当游戏时钟推进到新的一天时（自然跨日）
// - 当玩家主动睡眠到次日早晨时（主动睡眠跨日）
// 两者共享大部分逻辑，差异仅在于时间推进方式
//
// 使用方式：
// - 通过构造函数创建实例，确保所有引用成员有效
// - 将实例传递给 HandleNaturalDayTransition 或 SleepToNextMorning 函数
//
// 设计决策：
// - 使用引用而非指针，确保使用时必须提供有效对象
// - 回调函数使用 std::function，便于 UI 更新和存档触发
// ============================================================================
struct DayCycleRuntimeContext {
    // ============================================================================
    // 【核心系统引用】
    // 指向游戏运行时持有的系统实例，通过引用访问确保数据一致性
    // ============================================================================
    /** 游戏时钟引用：用于获取当前日期、判断季节 */
    CloudSeamanor::domain::GameClock& clock;
    
    /** 云海系统引用：用于推进云海状态、计算灵气收益 */
    CloudSeamanor::domain::CloudSystem& cloud_system;
    
    /** 云海守护者契约引用：用于检查契约卷解锁 */
    CloudSeamanor::domain::CloudGuardianContract& contract;
    
    /** 体力系统引用：用于重置体力值 */
    CloudSeamanor::domain::StaminaSystem& stamina;

    // ============================================================================
    // 【游戏对象引用】
    // ============================================================================
    /** 主屋修缮项目引用：用于每日影响计算 */
    RepairProject& main_house_repair;
    
    /** 茶田地块列表引用：用于每日浇水状态重置、影响计算 */
    std::vector<TeaPlot>& tea_plots;
    
    /** 灵兽状态引用：用于每日交互状态重置 */
    SpiritBeast& spirit_beast;
    
    /** 灵兽今日是否浇水标记的引用 */
    bool& spirit_beast_watered_today;
    
    /** NPC 列表引用：用于每日送礼标记重置、NPC 好感影响 */
    std::vector<NpcActor>& npcs;

    // ============================================================================
    // 【领域系统引用】
    // ============================================================================
    /** 节日系统引用：用于更新节日状态、获取节日预告 */
    CloudSeamanor::domain::FestivalSystem& festivals;
    
    /** 工坊系统引用：用于每日加工状态更新 */
    CloudSeamanor::domain::WorkshopSystem& workshop;
    
    /** 动态人生系统引用：用于 NPC 每日成长更新 */
    CloudSeamanor::domain::DynamicLifeSystem& dynamic_life;

    // ============================================================================
    // 【UI 状态引用】
    // ============================================================================
    /** 节日预告文本引用：用于更新界面上的节日预告显示 */
    std::string& festival_notice_text;

    // ============================================================================
    // 【回调函数】
    // 使用 std::function 实现灵活的依赖注入，便于测试和替换实现
    // ============================================================================
    /** 推送提示消息回调：text 为提示文本，duration 为显示时长（秒） */
    std::function<void(const std::string&, float)> push_hint;
    
    /** 记录信息日志回调：用于调试和追踪 */
    std::function<void(const std::string&)> log_info;
    
    /** 保存游戏回调：用于每日自动存档 */
    std::function<void()> save_game;
    
    /** 更新高亮交互目标回调：用于刷新当前可交互对象 */
    std::function<void()> update_highlighted_interactable;
    
    /** 刷新 HUD 文本回调：用于更新界面显示 */
    std::function<void()> update_hud_text;
    
    /** 刷新地块视觉回调：plot 为目标地块，highlighted 是否高亮 */
    std::function<void(TeaPlot&, bool)> refresh_plot_visual;
    
    /** 执行灵兽浇水协助回调：用于自动为已浇水的地块补水 */
    std::function<void()> run_spirit_beast_watering_aid;

    // ============================================================================
    // 【构造函数】
    // ============================================================================
    /**
     * @brief 构造函数 - 初始化所有成员
     * 
     * @param clk                       游戏时钟引用
     * @param cloud                     云海系统引用
     * @param con                       云海守护者契约引用
     * @param sta                       体力系统引用
     * @param repair                    主屋修缮项目引用
     * @param plots                     茶田地块列表引用
     * @param beast                     灵兽状态引用
     * @param beast_watered             灵兽今日浇水标记引用
     * @param npcs_list                NPC 列表引用
     * @param fest                      节日系统引用
     * @param ws                        工坊系统引用
     * @param dl                        动态人生系统引用
     * @param fest_notice               节日预告文本引用
     * @param hint_fn                   推送提示回调
     * @param log_fn                    记录日志回调
     * @param save_fn                   保存游戏回调
     * @param hud_fn                    刷新 HUD 回调
     * @param plot_fn                   刷新地块视觉回调
     * @param beast_fn                  灵兽浇水协助回调
     * @param highlight_fn              更新高亮交互目标回调（可选）
     */
    DayCycleRuntimeContext(
        CloudSeamanor::domain::GameClock& clk,
        CloudSeamanor::domain::CloudSystem& cloud,
        CloudSeamanor::domain::CloudGuardianContract& con,
        CloudSeamanor::domain::StaminaSystem& sta,
        RepairProject& repair,
        std::vector<TeaPlot>& plots,
        SpiritBeast& beast,
        bool& beast_watered,
        std::vector<NpcActor>& npcs_list,
        CloudSeamanor::domain::FestivalSystem& fest,
        CloudSeamanor::domain::WorkshopSystem& ws,
        CloudSeamanor::domain::DynamicLifeSystem& dl,
        std::string& fest_notice,
        std::function<void(const std::string&, float)> hint_fn,
        std::function<void(const std::string&)> log_fn,
        std::function<void()> save_fn,
        std::function<void()> hud_fn,
        std::function<void(TeaPlot&, bool)> plot_fn,
        std::function<void()> beast_fn,
        std::function<void()> highlight_fn = nullptr
    ) : clock(clk),
        cloud_system(cloud),
        contract(con),
        stamina(sta),
        main_house_repair(repair),
        tea_plots(plots),
        spirit_beast(beast),
        spirit_beast_watered_today(beast_watered),
        npcs(npcs_list),
        festivals(fest),
        workshop(ws),
        dynamic_life(dl),
        festival_notice_text(fest_notice),
        push_hint(hint_fn),
        log_info(log_fn),
        save_game(save_fn),
        update_hud_text(hud_fn),
        refresh_plot_visual(plot_fn),
        run_spirit_beast_watering_aid(beast_fn),
        update_highlighted_interactable(highlight_fn)
    {}
};

// ============================================================================
// 【HandleNaturalDayTransition】处理自然跨日刷新
// ============================================================================
/**
 * @brief 处理时钟 Tick 导致的日期变化
 * 
 * 触发条件：GameClock::Tick 调用后，Day() 返回值发生变化
 * 
 * 执行逻辑：
 * 1. 推进云海状态，计算每日灵气收益
 * 2. 计算玩家行为对云海的每日影响
 * 3. 检查契约卷是否解锁
 * 4. 更新节日系统
 * 5. 更新动态人生系统（NPC 成长）
 * 6. 重置每日标记（浇水、NPC 送礼等）
 * 7. 触发灵兽浇水协助
 * 8. 保存游戏
 * 
 * @param context       跨日刷新上下文
 * @param previous_day  Tick 前的日期（用于判断是否真的跨日）
 */
void HandleNaturalDayTransition(DayCycleRuntimeContext& context, int previous_day);

// ============================================================================
// 【SleepToNextMorning】执行主动睡眠到次日早晨
// ============================================================================
/**
 * @brief 处理玩家主动睡眠到次日早晨
 * 
 * 触发条件：玩家按 T 键，且当前时间在 22:00 之后或 06:00 之前
 * 
 * 与自然跨日的区别：
 * - 时间直接推进到次日 06:00，而非累计到 24:00
 * - 逻辑流程基本相同，共享大部分代码
 * 
 * 执行逻辑：
 * 1. 推进云海状态，计算每日灵气收益
 * 2. 计算玩家行为对云海的每日影响
 * 3. 检查契约卷是否解锁
 * 4. 更新节日系统
 * 5. 更新动态人生系统（NPC 成长）
 * 6. 重置每日标记（浇水、NPC 送礼等）
 * 7. 触发灵兽浇水协助
 * 8. 保存游戏
 * 9. 重置时钟到次日早晨
 * 
 * @param context 跨日刷新上下文
 */
void SleepToNextMorning(DayCycleRuntimeContext& context);

} // namespace CloudSeamanor::engine
