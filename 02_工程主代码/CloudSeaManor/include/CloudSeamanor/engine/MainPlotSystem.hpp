#pragma once

// ============================================================================
// 【MainPlotSystem】主线剧情系统
// ============================================================================
// 管理游戏主线剧情的触发、推进与多结局分支。
//
// 主要职责：
// - 管理10章主线剧情的触发条件检查
// - 驱动剧情对话（复用DialogueEngine）
// - 处理多结局分支路由（平衡/打开/关闭三条路线）
// - 管理剧情相关flag（玩家选择记录）
// - 与存档系统联动（持久化剧情进度）
//
// 与其他系统的关系：
    // - 依赖：GameClock（日期/季节）、DialogueEngine（对话播放）
    //          CloudSystem（灵气值）、NpcDialogueManager（好感度）
    // - 被依赖：GameRuntime（主循环）、GameAppSave（存档/读档）
//
// 设计原则：
// - 数据驱动：所有章节和剧情条目由JSON配置驱动
// - 复用复用：对话播放复用DialogueEngine，不重写打字机/路由
// - 事件驱动：剧情触发通过EventBus分发，其他系统可订阅
// - 多路由支持：第5章锁定结局路线（平衡/打开/关闭）
// - 存档兼容：剧情进度作为GameWorldState的一部分持久化
//
// 剧情触发优先级（由高到低）：
// 1. 剧情事件（Plot Event）—— 最高优先级，锁定玩家控制
// 2. 心事件（Heart Event）—— 需要在指定地点触发
// 3. 日常对话（Daily）—— 随时可触发
//
// 章节解锁条件示例：
// - 第2章：第1章完成 + 山庄苏醒祭完成 + 灵气>=100
// - 第3章：第2章完成 + 灵茶初芽节完成 + 灵气>=300
// - 第5章：第4章完成 + 灵气>=700 + 至少3位角色10心
// - 第10章：第9章完成 + 对应结局条件满足
// ============================================================================

#include "CloudSeamanor/engine/DialogueEngine.hpp"
#include "CloudSeamanor/domain/GameClock.hpp"
#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/engine/EventBus.hpp"

#include <filesystem>
#include <functional>
#include <optional>
#include <array>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace CloudSeamanor::engine {
using CloudSeamanor::domain::Season;
using CloudSeamanor::domain::GameClock;
using CloudSeamanor::domain::CloudSystem;
using CloudSeamanor::engine::DialogueChoice;
using CloudSeamanor::engine::DialogueContext;
using CloudSeamanor::engine::DialogueEngine;


// ============================================================================
// 【PlotRoute】剧情路线枚举
// ============================================================================
enum class PlotRoute {
    None,   // 尚未选择路线
    Balance,  // 平衡路线（真结局）
    Open,      // 打开路线（茶九线）
    Close,     // 关闭路线（雾川线）
};

// ============================================================================
// 【PlotConditionType】剧情条件类型
// ============================================================================
enum class PlotConditionType {
    DayRange,         // 游戏天数范围 [min_day, max_day]
    Season,            // 特定季节
    ChapterCompleted,  // 某章节已完成
    CloudLevel,        // 灵气值 >= 阈值
    HeartCount,         // 某NPC心级 >= 阈值
    TotalHeartCount,    // 总心级 >= 阈值
    FestivalJoined,    // 参与过某节日
    Location,           // 玩家在特定地点
    ItemOwned,          // 持有特定物品
    FlagSet,           // 特定flag已设置
    RouteSelected,      // 已选择某条路线
    CloudState,         // 云海状态
    AllNpcHeartCount,  // 全NPC平均心级
};

// ============================================================================
// 【PlotCondition】剧情条件
// ============================================================================
struct PlotCondition {
    PlotConditionType type = PlotConditionType::DayRange;

    // 数值条件
    int int_value = 0;
    int int_value2 = 0;  // 用于范围 [min, max]

    // 字符串条件
    std::string str_value;
    std::string str_value2;

    // 期望值
    std::string expected_value;

    bool IsSatisfied(int day, Season season, int cloud_level,
                     const std::string& location,
                     const std::unordered_set<std::string>& flags,
                     const std::unordered_map<std::string, int>& npc_hearts,
                     PlotRoute current_route) const;
};

// ============================================================================
// 【PlotChoiceEffect】剧情选项效果
// ============================================================================
struct PlotChoiceEffect {
    std::string flag_to_set;         // 设置的flag
    std::string next_plot_id;        // 跳转到的剧情ID
    int cloud_level_delta = 0;        // 灵气变化
    int favor_delta = 0;             // 好感度变化（特定NPC）
    std::string npc_id_for_favor;    // 好感度变化的NPC ID
    std::string reward_item;         // 奖励物品
    std::string route_lock;          // 锁定路线（balance/open/close）
};

// ============================================================================
// 【PlotNode】剧情节点
// ============================================================================
struct PlotNode {
    std::string id;                              // 节点ID
    std::string speaker;                         // 说话者（空=旁白）
    std::string text;                            // 对话文本（可含特殊标记）
    std::vector<DialogueChoice> choices;  // 选项列表
    std::string next_node_id;                    // 自动跳转节点（空=结束对话）
    std::string background_id;                   // 背景图ID
    std::string music_id;                        // BGM ID
    std::string effect_id;                      // 特效ID（particle/cloud/sparkle）
    std::vector<PlotChoiceEffect> choice_effects; // 选项效果
    int cloud_delta = 0;                         // 节点执行后的灵气变化
    std::string flag_to_set;                    // 节点执行后设置的flag
    std::string unlock_region;                  // 解锁的区域
    std::string unlock_npc;                     // 解锁的NPC
    std::string chapter_complete_flag;           // 设置章节完成flag
    std::string trigger_event;                   // 触发EventBus事件
    bool is_cutscene = false;                    // 是否为过场动画（无选项）
    bool blocks_player_control = true;            // 是否锁定玩家控制
};

// ============================================================================
// 【PlotEntry】剧情条目（一个完整的剧情事件）
// ============================================================================
struct PlotEntry {
    std::string id;                             // 剧情ID（唯一标识）
    std::string chapter_id;                      // 所属章节
    std::string title;                          // 剧情标题（用于日志）
    std::string description;                     // 剧情描述
    std::vector<PlotCondition> trigger_conditions; // 触发条件（AND关系）
    std::vector<PlotCondition> enter_conditions;  // 进入条件（更严格）
    std::string map_id;                         // 触发地图ID
    std::string anchor_id;                       // 触发锚点ID
    std::vector<PlotNode> nodes;                 // 剧情节点列表
    std::string start_node_id;                   // 起始节点ID
    std::string next_plot_id;                    // 完成后的下一个剧情ID
    bool repeatable = false;                     // 是否可重复触发
    bool completed = false;                      // 是否已完成
    int priority = 0;                            // 优先级（数字越大优先级越高）
};

// ============================================================================
// 【ChapterEntry】章节定义
// ============================================================================
struct ChapterEntry {
    std::string id;                             // 章节ID（如 "ch1", "ch2"）
    int chapter_number = 1;                       // 章节编号（1-10）
    std::string title;                           // 章节标题
    std::string subtitle;                        // 章节副标题
    std::string description;                      // 章节描述
    std::vector<PlotCondition> unlock_conditions; // 解锁条件
    std::string prologue_plot_id;                // 章节序章剧情ID
    std::string epilogue_plot_id;                // 章节尾声剧情ID
    std::vector<std::string> plot_ids;            // 章节内所有剧情ID
    bool completed = false;                       // 是否已完成
    int target_cloud_level = 0;                  // 章节目标灵气值
    int target_heart_count = 0;                  // 章节目标心级数
    std::string next_chapter_id;                 // 下一章节ID
    std::string route_lock;                      // 锁定的路线（可选）
};

// ============================================================================
// 【PlotState】剧情状态机
// ============================================================================
enum class PlotState {
    Idle,           // 无剧情进行
    TriggerPending,  // 等待触发检查
    Playing,         // 剧情对话进行中
    ChoicePending,   // 等待玩家选择
    Transition,      // 场景切换/过渡
    Completed,       // 剧情完成
};

// ============================================================================
// 【PlotCallbacks】剧情系统回调
// ============================================================================
struct PlotCallbacks {
    std::function<void(const std::string&)> on_chapter_start;
    std::function<void(const std::string&)> on_chapter_complete;
    std::function<void(const std::string&)> on_plot_start;
    std::function<void(const std::string&)> on_plot_complete;
    std::function<void(const std::string&)> on_unlock_region;
    std::function<void(const std::string&)> on_unlock_npc;
    std::function<void(const std::string&)> on_flag_set;
    std::function<void(int)> on_cloud_delta;
    std::function<void(const std::string&, int)> on_favor_delta;
    std::function<void(const std::string&)> on_route_lock;
    std::function<void(const std::string&)> on_notice;  // 剧情预告通知
    std::function<void(const std::string&)> log_info;
};

// ============================================================================
// 【MainPlotSystem】主线剧情系统
// ============================================================================
class MainPlotSystem {
public:
    // ========================================================================
    // 【构造函数】
    // ========================================================================
    MainPlotSystem();

    // ========================================================================
    // 【初始化】
    // ========================================================================
    void Initialize(const std::string& data_root = "assets/data");
    void SetCallbacks(PlotCallbacks callbacks);

    // ========================================================================
    // 【系统引用注入】
    // ========================================================================
    void SetGameClock(const GameClock* clock);
    void SetCloudSystem(const CloudSystem* cloud);
    void SetNpcHeartGetter(
        const std::function<int(const std::string& npc_id)>& getter);

    // ========================================================================
    // 【帧更新】
    // ========================================================================
    void Update(float delta_seconds);

    // ========================================================================
    // 【剧情触发检查】
    // ========================================================================
    // 在每日开始时调用，检查是否有可触发的剧情
    void CheckPlotTriggers();

    // 在玩家进入特定地点时调用（剧情地点触发）
    void OnLocationEntered(const std::string& map_id,
                          const std::string& anchor_id);

    // 在特定游戏事件发生时调用（如节日参与、建筑升级）
    void OnGameEvent(const std::string& event_type,
                     const std::unordered_map<std::string, std::string>& event_data);

    // ========================================================================
    // 【对话控制】
    // ========================================================================
    void StartPlot(const std::string& plot_id);
    void StartChapter(const std::string& chapter_id);
    void EndPlot();
    bool OnConfirm();       // 打字完成/继续
    bool SelectChoice(std::size_t choice_index);  // 选择选项

    // ========================================================================
    // 【状态查询】
    // ========================================================================
    [[nodiscard]] PlotState State() const { return state_; }
    [[nodiscard]] bool IsPlaying() const {
        return state_ == PlotState::Playing ||
               state_ == PlotState::ChoicePending;
    }
    [[nodiscard]] const PlotEntry* CurrentPlot() const;
    [[nodiscard]] const ChapterEntry* CurrentChapter() const;
    [[nodiscard]] const PlotNode* CurrentNode() const;
    [[nodiscard]] const std::string& CurrentText() const;
    [[nodiscard]] const std::vector<DialogueChoice>& CurrentChoices() const;
    [[nodiscard]] float TypingProgress() const;
    [[nodiscard]] bool IsTyping() const;
    [[nodiscard]] PlotRoute CurrentRoute() const { return current_route_; }
    [[nodiscard]] const std::string& CurrentChapterId() const {
        return current_chapter_id_;
    }
    [[nodiscard]] bool IsChapterUnlocked(const std::string& chapter_id) const;
    [[nodiscard]] bool IsPlotCompleted(const std::string& plot_id) const;
    [[nodiscard]] bool HasFlag(const std::string& flag) const;
    [[nodiscard]] const ChapterEntry* GetChapter(
        const std::string& chapter_id) const;
    [[nodiscard]] const std::vector<ChapterEntry>& GetAllChapters() const {
        return chapters_;
    }
    [[nodiscard]] std::vector<const PlotEntry*> GetPendingPlots() const;
    [[nodiscard]] std::string GetChapterNotice() const;

    // ========================================================================
    // 【系统访问】
    // ========================================================================
    [[nodiscard]] DialogueEngine& Engine() { return dialogue_engine_; }
    [[nodiscard]] const DialogueEngine& Engine() const { return dialogue_engine_; }

    // ========================================================================
    // 【剧情通知系统】
    // ========================================================================
    // 玩家可以主动查看即将到来的剧情预告
    [[nodiscard]] std::vector<const PlotEntry*> GetUpcomingPlots(
        int max_count = 3) const;
    [[nodiscard]] std::string GetUpcomingNotice() const;

    // ========================================================================
    // 【手动剧情触发（Debug/测试）】
    // ========================================================================
    void DebugForceStartPlot(const std::string& plot_id);
    void DebugForceStartChapter(const std::string& chapter_id);
    void DebugSetRoute(PlotRoute route);
    void DebugSetFlag(const std::string& flag);
    void DebugResetAllProgress();

    // ========================================================================
    // 【持久化】
    // ========================================================================
    void SaveState(std::vector<std::string>& lines) const;
    void LoadState(const std::vector<std::string>& lines);

private:
    static constexpr std::array<std::string_view, 12> kNpcIds{
        "acha", "xiaoman", "wanxing", "lin",
        "song", "yu", "mo", "qiao",
        "he", "ning", "an", "shu"
    };
    // ========================================================================
    // 【内部方法】
    // ========================================================================

    // 加载所有章节和剧情数据（从JSON文件）
    void LoadChapterData_();
    void LoadPlotData_();

    // 检查单个剧情是否可以触发
    bool CanTriggerPlot_(const PlotEntry& plot) const;

    // 检查进入条件
    bool CanEnterPlot_(const PlotEntry& plot) const;
    std::unordered_map<std::string, int> GetNpcHeartLevels() const;

    // 构建对话上下文
    DialogueContext BuildDialogueContext_() const;

    // 进入剧情节点
    void EnterNode_(const std::string& node_id);

    // 执行节点效果
    void ExecuteNodeEffect_(const PlotNode& node);

    // 执行选项效果
    void ExecuteChoiceEffect_(const PlotChoiceEffect& effect);

    // 章节完成处理
    void OnChapterComplete_(const std::string& chapter_id);

    // 剧情完成处理
    void OnPlotComplete_();

    // 路线锁定处理
    void LockRoute_(PlotRoute route);

    // 解析JSON为剧情数据
    std::vector<ChapterEntry> ParseChaptersFromJson_(
        const std::string& json_content) const;
    std::vector<PlotEntry> ParsePlotsFromJson_(
        const std::string& json_content) const;
    std::vector<PlotNode> ParseNodesFromJson_(
        const std::string& json_content) const;

    // 获取所有可触发的剧情（按优先级排序）
    std::vector<const PlotEntry*> GetTriggerablePlots_() const;

    // 数据根目录
    std::string data_root_;

    // 回调
    PlotCallbacks callbacks_;

    // 系统引用
    const GameClock* clock_ = nullptr;
    const CloudSystem* cloud_system_ = nullptr;
    std::function<int(const std::string&)> npc_heart_getter_;

    // 数据
    std::vector<ChapterEntry> chapters_;
    std::vector<PlotEntry> all_plots_;

    // 索引
    std::unordered_map<std::string, const ChapterEntry*> chapter_index_;
    std::unordered_map<std::string, const PlotEntry*> plot_index_;
    std::unordered_map<std::string, const PlotNode*> node_index_;

    // 当前状态
    PlotState state_ = PlotState::Idle;
    std::string current_chapter_id_;
    std::string current_plot_id_;
    std::string current_node_id_;

    // 运行时数据
    PlotRoute current_route_ = PlotRoute::None;
    std::unordered_set<std::string> flags_;         // 剧情flag集合
    std::unordered_set<std::string> completed_plots_; // 已完成的剧情ID
    std::unordered_set<std::string> completed_chapters_; // 已完成的章节ID
    std::unordered_set<std::string> triggered_this_day_; // 今日已触发的剧情（防重复）

    // 对话引擎（复用）
    DialogueEngine dialogue_engine_;

    // 打字机状态
    bool typing_skipped_ = false;
};

// ============================================================================
// 【RouteName】路线名称转换
// ============================================================================
constexpr const char* RouteName(PlotRoute r) {
    switch (r) {
        case PlotRoute::Balance: return "balance";
        case PlotRoute::Open:    return "open";
        case PlotRoute::Close:   return "close";
        default:                return "none";
    }
}

// ============================================================================
// 【PlotRouteFromString】字符串转路线
// ============================================================================
[[nodiscard]] inline PlotRoute PlotRouteFromString(
    const std::string& s) {
    if (s == "balance") return PlotRoute::Balance;
    if (s == "open")    return PlotRoute::Open;
    if (s == "close")   return PlotRoute::Close;
    return PlotRoute::None;
}

// ============================================================================
// 【SeasonFromString】字符串转季节
// ============================================================================
[[nodiscard]] inline Season SeasonFromString(const std::string& s) {
    if (s == "春" || s == "spring") return Season::Spring;
    if (s == "夏" || s == "summer") return Season::Summer;
    if (s == "秋" || s == "autumn") return Season::Autumn;
    if (s == "冬" || s == "winter") return Season::Winter;
    return Season::Spring;
}

}  // namespace CloudSeamanor::engine

// Backward-compatible alias for existing domain call sites.
namespace CloudSeamanor::domain {
using MainPlotSystem = CloudSeamanor::engine::MainPlotSystem;
using PlotRoute = CloudSeamanor::engine::PlotRoute;
using PlotConditionType = CloudSeamanor::engine::PlotConditionType;
using PlotCondition = CloudSeamanor::engine::PlotCondition;
using PlotChoiceEffect = CloudSeamanor::engine::PlotChoiceEffect;
using PlotNode = CloudSeamanor::engine::PlotNode;
using PlotEntry = CloudSeamanor::engine::PlotEntry;
using ChapterEntry = CloudSeamanor::engine::ChapterEntry;
using PlotState = CloudSeamanor::engine::PlotState;
using PlotCallbacks = CloudSeamanor::engine::PlotCallbacks;
using CloudSeamanor::engine::RouteName;
using CloudSeamanor::engine::PlotRouteFromString;
using CloudSeamanor::engine::SeasonFromString;
}  // namespace CloudSeamanor::domain
