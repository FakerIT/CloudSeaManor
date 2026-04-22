#pragma once

#include "CloudSeamanor/CloudSystem.hpp"
#include "CloudSeamanor/GameClock.hpp"

// ============================================================================
// 【NpcWeather】NPC 对话天气条件（与 CloudState 一一对应，值相同）
// ============================================================================
enum class NpcWeather {
    Clear = 0,
    Mist = 1,
    DenseCloud = 2,
    Tide = 3,
};

constexpr const char* NpcWeatherName(NpcWeather w) {
    switch (w) {
        case NpcWeather::Clear:     return "晴";
        case NpcWeather::Mist:      return "薄雾";
        case NpcWeather::DenseCloud: return "浓云";
        case NpcWeather::Tide:       return "大潮";
    }
    return "晴";
}

// ============================================================================
// 【NpcDialogueManager】NPC 对话管理器
// ============================================================================
// 统一管理所有 NPC 的对话内容：日常对话池、心事件对话树。
//
// 主要职责：
// - 从 JSON 文件惰性加载对话数据
// - 按 NPC ID 查询对话树
// - 支持条件路由（好感度、季节、时段、天气）
// - 管理心事件触发状态
//
// 数据文件：
// - `daily_dialogue/npc_daily_{npc_id}.json` - NPC 日常对话池
// - `dialogue/npc_heart_{npc_id}_{heart}_event.json` - 心事件对话树
//
// 设计原则：
// - 惰性加载：首次访问某 NPC 对话时才从磁盘读取
// - 缓存管理：已加载的对话常驻内存，不重复解析
// - 条件路由：支持 $FAVOR、$SEASON、$TIME、$WEATHER 等条件标记
// ============================================================================

#include "CloudSeamanor/DialogueEngine.hpp"
#include "CloudSeamanor/GameAppRuntimeTypes.hpp"

#include <filesystem>
#include <functional>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace CloudSeamanor::engine {
using CloudSeamanor::domain::DayPhase;
using CloudSeamanor::domain::Season;

// ============================================================================
// 【DailyDialogueEntry】日常对话条目
// ============================================================================
struct DailyDialogueEntry {
    std::string id;        // 对话唯一标识
    std::string text;       // 对话文本（可含特殊标记）
    int favor_min = 0;     // 最低好感度要求
    int favor_max = 999;   // 最高好感度要求
    std::optional<Season> season;       // 季节条件（空=任意）
    std::optional<DayPhase> time_of_day; // 时段条件（空=任意）
    std::optional<NpcWeather> weather;    // 天气条件（空=任意）
    int heart_min = 0;     // 最低心级要求
    int heart_max = 10;    // 最高心级要求
};

// ============================================================================
// 【DailyDialoguePool】NPC 日常对话池
// ============================================================================
struct DailyDialoguePool {
    std::string npc_id;
    std::vector<DailyDialogueEntry> greetings;  // 问候语
    std::vector<DailyDialogueEntry> small_talks; // 闲聊
    std::vector<DailyDialogueEntry> farewells;  // 告别语
};

// ============================================================================
// 【HeartEventEntry】心事件条目
// ============================================================================
struct HeartEventEntry {
    std::string event_id;        // 事件ID
    std::string trigger_condition;
    std::string map_id;
    std::string anchor_id;
    std::string dialogue_tree_id;
    int favor_threshold = 0;
    int heart_threshold = 0;
    bool repeatable = false;
    std::optional<Season> season;
    std::optional<NpcWeather> weather;
    std::optional<DayPhase> time_of_day;
    bool completed = false;
    int reward_favor = 0;
    std::string reward_flag;
};

// ============================================================================
// 【NpcDialogueContext】NPC 对话上下文（比 DialogueContext 更丰富）
// ============================================================================
struct NpcDialogueContext {
    std::string npc_id;
    std::string npc_name;
    std::string player_name = "云海旅人";
    std::string farm_name = "云海山庄";

    // 条件路由参数
    int player_favor = 0;
    int npc_heart_level = 0;
    int current_day = 1;
    int current_day_in_season = 1;
    int current_hour = 8;
    std::optional<Season> current_season;
    std::optional<NpcWeather> current_weather;
    std::optional<DayPhase> current_time_of_day;
    bool has_item = false;
    bool quest_completed = false;
    std::string current_location;
    std::string current_activity;
    NpcMood npc_mood = NpcMood::Normal;

    // 特殊变量（用于 $[VARIABLE] 替换）
    std::string recent_item;
    std::string current_crop;
    int spirit_energy = 0;
    int cloud_level = 0;

    /** 转换为基础 DialogueContext（用于 DialogueEngine） */
    DialogueContext ToDialogueContext() const {
        DialogueContext ctx;
        ctx.player_name = player_name;
        ctx.farm_name = farm_name;
        ctx.npc_name = npc_name;
        ctx.player_favor = player_favor;
        ctx.current_day = current_day;
        ctx.current_season =
            current_season ? CloudSeamanor::domain::GameClock::SeasonName(*current_season) : "";
        ctx.current_weather = current_weather ? NpcWeatherName(*current_weather) : "";
        ctx.item_name = recent_item;
        ctx.has_item = has_item;
        ctx.quest_completed = quest_completed;
        return ctx;
    }
};

// ============================================================================
// 【DialogueLoadResult】对话加载结果
// ============================================================================
struct DialogueLoadResult {
    bool success = false;
    std::string error_message;
    std::vector<DialogueNode> nodes;
};

// ============================================================================
// 【NpcDialogueManagerCallbacks】对话管理器回调
// ============================================================================
struct NpcDialogueManagerCallbacks {
    std::function<void(const std::string&, float)> push_hint;
    std::function<void(const std::string&)> log_info;
    std::function<void(int npc_id, int delta)> on_favor_change;
};

// ============================================================================
// 【NpcDialogueManager】NPC 对话管理器
// ============================================================================
class NpcDialogueManager {
public:
    // ========================================================================
    // 【初始化】
    // ========================================================================

    /**
     * @brief 构造函数
     * @param data_root 对话数据根目录（相对于 assets/data/）
     */
    explicit NpcDialogueManager(const std::string& data_root = "assets/data");

    /**
     * @brief 设置回调
     */
    void SetCallbacks(NpcDialogueManagerCallbacks callbacks);

    /**
     * @brief 设置所有 NPC 状态引用（用于查询当前好感度等）
     */
    void SetNpcList(const std::vector<NpcActor>* npcs);

    // ========================================================================
    // 【持久化】
    // ========================================================================

    /** 保存心事件完成状态（用于存档） */
    void SaveState(std::vector<std::string>& lines) const;

    /** 加载心事件完成状态（用于读档） */
    void LoadState(const std::vector<std::string>& lines);

    // ========================================================================
    // 【对话加载】
    // ========================================================================

    /**
     * @brief 加载 NPC 的日常对话池
     * @param npc_id NPC 标识
     * @return true 加载成功
     */
    bool LoadDailyDialogue(const std::string& npc_id);

    /**
     * @brief 加载心事件对话树
     * @param npc_id NPC 标识
     * @param heart_level 心级（2,4,6,8,10）
     * @return 对话节点列表
     */
    std::vector<DialogueNode> LoadHeartEventDialogue(const std::string& npc_id, int heart_level);

    /**
     * @brief 标记心事件为已完成（对话引擎调用）
     */
    void MarkHeartEventComplete(const std::string& npc_id, const std::string& event_id);

    /**
     * @brief 从 JSON 文本加载对话树（内部用）
     */
    DialogueLoadResult LoadDialogueFromJson(const std::string& json_content);

    // ========================================================================
    // 【对话选择】
    // ========================================================================

    /**
     * @brief 获取 NPC 的问候语
     * @param npc_id NPC 标识
     * @param ctx 对话上下文
     * @return 选中的问候 DialogueNode，无匹配返回空 vector
     */
    std::vector<DialogueNode> GetGreeting(const std::string& npc_id,
                                          const NpcDialogueContext& ctx);

    /**
     * @brief 获取 NPC 的闲聊语
     */
    std::vector<DialogueNode> GetSmallTalk(const std::string& npc_id,
                                            const NpcDialogueContext& ctx);

    /**
     * @brief 获取 NPC 的告别语
     */
    std::vector<DialogueNode> GetFarewell(const std::string& npc_id,
                                           const NpcDialogueContext& ctx);

    // ========================================================================
    // 【心事件管理】
    // ========================================================================

    /**
     * @brief 初始化心事件表（从 CSV 或代码定义）
     */
    void InitializeHeartEvents();

    /**
     * @brief 检查某 NPC 是否有可触发的心事件
     * @param npc_id NPC 标识
     * @param ctx 对话上下文
     * @return 可触发的事件，无则返回 nullptr
     */
    const HeartEventEntry* CheckHeartEventTrigger(const std::string& npc_id,
                                                  const NpcDialogueContext& ctx);

    /**
     * @brief 标记心事件为已完成
     */
    void CompleteHeartEvent(const std::string& npc_id, const std::string& event_id);

    /**
     * @brief 获取某 NPC 所有心事件完成状态
     */
    std::vector<HeartEventEntry> GetNpcHeartEvents(const std::string& npc_id) const;

    /**
     * @brief 设置心事件完成状态（读档恢复用）
     */
    void SetHeartEventCompleted(const std::string& npc_id, const std::string& event_id, bool completed);

    // ========================================================================
    // 【对话路由】
    // ========================================================================

    /**
     * @brief 为 NPC 选择最合适的日常对话
     * @param npc_id NPC 标识
     * @param ctx 对话上下文
     * @return 对话节点（自动附加 greeting/small_talk/farewell 前缀）
     */
    std::vector<DialogueNode> SelectDailyDialogue(const std::string& npc_id,
                                                   const NpcDialogueContext& ctx);

    /**
     * @brief 将日常对话条目转换为 DialogueNode
     */
    DialogueNode DialogueEntryToNode(const DailyDialogueEntry& entry,
                                     const NpcDialogueContext& ctx) const;

    // ========================================================================
    // 【状态查询】
    // ========================================================================

    /**
     * @brief 检查是否已加载某 NPC 的日常对话
     */
    [[nodiscard]] bool IsDailyDialogueLoaded(const std::string& npc_id) const;

    /**
     * @brief 获取已加载的 NPC ID 列表
     */
    [[nodiscard]] std::vector<std::string> GetLoadedNpcIds() const;

    // ========================================================================
    // 【文本替换工具】
    // ========================================================================

    /**
     * @brief 扩展文本替换（支持 NPC 专属变量）
     */
    std::string ReplaceExtendedTokens(const std::string& raw,
                                      const NpcDialogueContext& ctx) const;

private:
    // ========================================================================
    // 【内部工具】
    // ========================================================================

    // 按条件过滤并随机选择对话条目（返回池中的索引）
    std::optional<std::size_t> SelectMatchingEntry_(
        const std::vector<DailyDialogueEntry>& pool,
        const NpcDialogueContext& ctx) const;

    // 解析条件表达式
    bool MatchesCondition_(const DailyDialogueEntry& entry,
                           const NpcDialogueContext& ctx) const;

    // 获取心事件存储键
    static std::string HeartEventKey_(const std::string& npc_id, const std::string& event_id);

    // 线程安全的 RNG（每次调用获取新种子，避免 static 初始化顺序问题）
    std::mt19937& Rng() const;

    std::string data_root_;
    NpcDialogueManagerCallbacks callbacks_;

    // 缓存：已加载的日常对话池
    std::unordered_map<std::string, DailyDialoguePool> daily_pools_;

    // 缓存：已加载的对话树（心事件）
    std::unordered_map<std::string, std::vector<DialogueNode>> heart_event_trees_;

    // NPC 列表引用
    const std::vector<NpcActor>* npc_list_ = nullptr;

    // 心事件注册表
    std::unordered_map<std::string, std::vector<HeartEventEntry>> npc_heart_events_;

    // 心事件完成状态（npc_id + event_id → completed）
    std::unordered_map<std::string, bool> heart_event_completed_;

    // 已加载标记
    std::unordered_set<std::string> loaded_npc_ids_;
};

// ============================================================================
// 【BuildNpcDialogueContext】从游戏状态构建 NPC 对话上下文
// ============================================================================
NpcDialogueContext BuildNpcDialogueContext(
    const NpcActor& npc,
    const CloudSeamanor::domain::GameClock& clock,
    const CloudSeamanor::domain::CloudState cloud_state,
    const std::string& player_name = "云海旅人",
    const std::string& farm_name = "云海山庄");

} // namespace CloudSeamanor::engine
