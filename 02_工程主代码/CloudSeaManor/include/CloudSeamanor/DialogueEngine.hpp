#pragma once

// ============================================================================
// 【DialogueEngine】对话引擎
// ============================================================================
// 驱动 NPC 对话体验：打字机效果、分支选择、选项系统。
//
// 主要职责：
// - 管理对话状态机（Idle → Typing → WaitingChoice → Completed）
// - 实现打字机效果（逐字符显示，支持跳过）
// - 管理分支对话树和选项选择
// - 处理对话完成后的回调（触发奖励/任务等）
// - 支持特殊标记：$[PLAYER_NAME]、$[ITEM_NAME]、$[FARM_NAME] 等
//
// 数据驱动：
// - 所有对话内容由外部对话表（JSON）驱动
// - 支持分支路由条件（好感度、持有物品、已完成任务等）
//
// 设计原则：
// - 无帧循环逻辑，外部 Update 驱动状态变化
// - 不持有渲染对象，只管理数据和状态
// - 打字速度可配置，支持每字符 40ms ~ 80ms
// ============================================================================

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace CloudSeamanor::engine {

// ============================================================================
// 【DialogueChoice】对话选项
// ============================================================================
struct DialogueChoice {
    /** 选项唯一标识（用于路由） */
    std::string id;

    /** 选项显示文本 */
    std::string text;

    /** 目标节点 ID（空字符串表示结束对话） */
    std::string next_node_id;

    /** 条件：最低好感度（可选） */
    std::optional<int> min_favor;

    /** 条件：是否需要持有物品（可选，当前以 has_item 表示） */
    std::optional<std::string> require_item;

    /** 条件不满足时的回退节点（可选） */
    std::string fallback_next_node_id;
};

// ============================================================================
// 【DialogueNode】对话节点
// ============================================================================
struct DialogueNode {
    /** 节点唯一标识 */
    std::string id;

    /** 说话者名称（NPC 名称） */
    std::string speaker;

    /** 对话文本内容 */
    std::string text;

    /** 该节点可用的选项列表（空列表表示自动结束） */
    std::vector<DialogueChoice> choices;

    /** 是否为分支节点（有多个选项） */
    [[nodiscard]] bool IsBranch() const { return choices.size() > 1; }

    /** 是否为普通节点（无选项） */
    [[nodiscard]] bool IsTerminal() const { return choices.empty(); }
};

// ============================================================================
// 【DialogueState】对话状态机
// ============================================================================
enum class DialogueState {
    /** 无对话进行 */
    Idle,
    /** 打字机效果播放中 */
    Typing,
    /** 等待玩家选择 */
    WaitingChoice,
    /** 对话已完成 */
    Completed,
};

// ============================================================================
// 【DialogueContext】对话上下文（条件路由用）
// ============================================================================
struct DialogueContext {
    std::string player_name = "云海旅人";
    std::string farm_name = "云海山庄";
    std::string npc_name;
    std::string current_weather;
    std::string item_name;
    int player_favor = 0;       // 与当前 NPC 的好感度
    int current_day = 1;
    std::string current_season = "春";
    bool has_item = false;
    bool quest_completed = false;
    // 可扩展：持有物品列表、已完成任务列表等
};

// ============================================================================
// 【DialogueCallbacks】对话回调
// ============================================================================
struct DialogueCallbacks {
    /** 对话文本变化回调（打字机更新） */
    std::function<void(const std::string& partial_text)> on_text_update;

    /** 对话节点变化回调（进入新节点） */
    std::function<void(const DialogueNode& node)> on_node_change;

    /** 选项列表变化回调 */
    std::function<void(const std::vector<DialogueChoice>& choices)> on_choices_change;

    /** 对话完成回调 */
    std::function<void()> on_complete;

    /** 好感度变化回调 */
    std::function<void(int npc_id, int delta)> on_favor_change;
};

// ============================================================================
// 【DialogueEngine】对话引擎
// ============================================================================
class DialogueEngine {
public:
    // ========================================================================
    // 【初始化】
    // ========================================================================

    /** 设置回调 */
    void SetCallbacks(DialogueCallbacks callbacks);

    // ========================================================================
    // 【对话控制】
    // ========================================================================

    /**
     * @brief 开始一段对话。
     * @param nodes 对话节点表（由外部对话表解析而来）
     * @param start_node_id 起始节点 ID
     * @param ctx 对话上下文（影响路由条件）
     */
    void StartDialogue(const std::vector<DialogueNode>& nodes,
                       const std::string& start_node_id,
                       const DialogueContext& ctx);

    /**
     * @brief 开始一段对话（使用对话表 ID，自动加载）。
     * @param dialogue_table_id 对话文件路径（相对于 assets/data/）
     * @param start_node_id 起始节点 ID
     * @param ctx 对话上下文
     * @note 通过 data_root_ 拼接路径，从 JSON 文件惰性加载对话树。
     *       支持路径格式：
     *         - "dialogue/npc_heart_acha_h2.json" → assets/data/dialogue/npc_heart_acha_h2.json
     *         - "daily_dialogue/npc_daily_acha.json" → assets/data/daily_dialogue/npc_daily_acha.json
     */
    void StartDialogueById(const std::string& dialogue_table_id,
                           const std::string& start_node_id,
                           const DialogueContext& ctx);

    /**
     * @brief 停止当前对话。
     */
    void EndDialogue();

    // ========================================================================
    // 【帧更新（外部驱动）】
    // ========================================================================

    /**
     * @brief 每帧调用，推进打字机效果。
     * @param delta_seconds 帧间隔（秒）
     */
    void Update(float delta_seconds);

    // ========================================================================
    // 【玩家输入】
    // ========================================================================

    /**
     * @brief 玩家按确认键（E / Space / Enter）。
     * @return true 表示输入被消费。
     */
    bool OnConfirm();

    /**
     * @brief 玩家选择选项。
     * @param choice_index 选项索引（0-based）
     * @return true 表示选择有效。
     */
    bool SelectChoice(std::size_t choice_index);

    /**
     * @brief 玩家跳过打字机效果。
     */
    void SkipTyping();

    // ========================================================================
    // 【状态查询】
    // ========================================================================

    [[nodiscard]] bool IsActive() const;
    [[nodiscard]] DialogueState State() const { return state_; }
    [[nodiscard]] const DialogueNode* CurrentNode() const;
    [[nodiscard]] const std::string& CurrentText() const { return current_text_; }
    /** 打字中的部分文本（打字未完成时返回已显示部分） */
    [[nodiscard]] const std::string& TypingText() const { return current_text_; }
    /** 完整文本（打字完成后的全部内容） */
    [[nodiscard]] const std::string& FullText() const { return full_text_; }
    [[nodiscard]] const std::vector<DialogueChoice>& CurrentChoices() const { return current_choices_; }
    [[nodiscard]] float TypingProgress() const;  // 0.0~1.0

    // ========================================================================
    // 【打字机配置】
    // ========================================================================

    /** 设置打字速度（每字符毫秒数，范围 20~100） */
    void SetTypingSpeed(int ms_per_char);

    /**
     * @brief 设置对话数据根目录。
     * @note 用于 StartDialogueById 拼接 JSON 文件路径。
     */
    void SetDataRoot(const std::string& root) { data_root_ = root; }

    // ========================================================================
    // 【特殊标记替换】
    // ========================================================================

    /**
     * @brief 替换文本中的特殊标记。
     * 替换规则：
     *   $[PLAYER_NAME]  → 玩家名称
     *   $[FARM_NAME]    → 农场名称
     *   $[ITEM_NAME]    → 最近获得的物品名称
     *   $[CROP_NAME]    → 当前作物名称
     *   $[SEASON]       → 当前季节
     */
    [[nodiscard]] std::string ReplaceTokens(const std::string& raw,
                                              const DialogueContext& ctx) const;

private:
    // 路由：给定选项和上下文，决定走哪个 next_node_id
    [[nodiscard]] std::string RouteChoice_(const DialogueChoice& choice,
                                            const DialogueContext& ctx) const;

    // 进入新节点
    void EnterNode_(const std::string& node_id);

    DialogueCallbacks callbacks_;
    DialogueContext context_;

    std::vector<DialogueNode> nodes_;
    std::string data_root_;  // 对话数据根目录（用于 StartDialogueById）
    std::string current_text_;
    std::string full_text_;  // 完整文本（打字机目标）
    std::vector<DialogueChoice> current_choices_;

    DialogueState state_ = DialogueState::Idle;
    float typing_timer_ = 0.0f;
    std::size_t typing_char_index_ = 0;
    int ms_per_char_ = 45;  // 默认每字符 45ms（约22字符/秒）
    std::string current_node_id_;
    bool typing_skipped_ = false;
};

// ============================================================================
// 【LoadDialogueFromJson】从 JSON 加载对话表（外部调用）
// ============================================================================
/**
 * @brief 解析 JSON 对话表为节点列表。
 *
 * 期望 JSON 格式：
 * @code
 * {
 *   "nodes": [
 *     {
 *       "id": "intro",
 *       "speaker": "阿茶",
 *       "text": "欢迎回家 $[PLAYER_NAME]！今天想做什么呢？",
 *       "choices": [
 *         { "id": "c1", "text": "去茶田看看", "next": "tea_field" },
 *         { "id": "c2", "text": "想聊聊天", "next": "chat" }
 *       ]
 *     }
 *   ]
 * }
 * @endcode
 */
std::vector<DialogueNode> LoadDialogueFromJson(const std::string& json_content);

}  // namespace CloudSeamanor::engine
