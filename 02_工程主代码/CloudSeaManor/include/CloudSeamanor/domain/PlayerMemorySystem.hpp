#pragma once

// ============================================================================
// 【PlayerMemorySystem】玩家行为记忆系统
// ============================================================================
// 统一管理玩家的行为记忆，让 NPC 能够对玩家近期行为做出自然回应。
//
// 主要职责：
// - 记录玩家高价值行为（制茶、探索、社交等）
// - 管理记忆衰减与清理
// - 提供对话钩子查询
// - 生成情感邮件触发
//
// 设计原则：
// - 轻追踪：只记录少量高价值行为
// - 弱打扰：反馈以闲聊、信件为主
// - 重语气：同一事件由不同 NPC 给出不同情绪角度
// - 可衰减：行为记忆会自然淡出
// ============================================================================

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace CloudSeamanor::domain {

// ============================================================================
// 【PlayerMemoryType】行为记忆类型枚举
// ============================================================================
enum class PlayerMemoryType {
    None = 0,
    BrewHolyTea,           // 制出圣品茶
    BrewNewTea,            // 首次完成某高品质茶
    FarmFocused,            // 连续多日忙于农田
    WorkshopFocused,       // 连续多日忙于工坊
    ExploreBackMountain,   // 连续前往后山
    PurifyMiasma,         // 净化浊灵
    FestivalAttend,        // 参加节日
    RelationshipMilestone,  // 关系里程碑
    EcoBalanced,           // 庄园元素趋于平衡
    EcoImbalanced,         // 庄园元素失衡
    TeaSpiritUnlocked      // 新茶灵加入图鉴
};

// ============================================================================
// 【PlayerMemoryRecord】行为记忆记录
// ============================================================================
struct PlayerMemoryRecord {
    PlayerMemoryType type = PlayerMemoryType::None;
    std::string subject_id;        // tea_id / npc_id / map_id / element_id
    int game_day = 0;              // 记录发生的游戏天数
    int weight = 1;               // 强度，用于排序
    int decay_days = 7;            // 记忆衰减时长
    bool consumed_by_mail = false;  // 是否已被邮件使用
    bool consumed_by_dialogue = false;  // 是否已被对话使用
};

// ============================================================================
// 【MemoryHook】对话记忆钩子
// ============================================================================
struct MemoryHook {
    std::string id;
    PlayerMemoryType memory_type = PlayerMemoryType::None;
    std::string speaker_id;         // NPC ID
    int min_heart_level = 0;       // 最低好感等级
    int max_days_since_memory = 7;  // 最大追溯天数
    int priority = 0;             // 抽取优先级
    std::string season_limit;       // 季节限制
    std::string weather_limit;     // 天气限制
    std::string text;              // 对话文本
};

// ============================================================================
// 【MailTemplate】邮件模板
// ============================================================================
struct MemoryMailTemplate {
    std::string id;
    PlayerMemoryType memory_type = PlayerMemoryType::None;
    std::string speaker_id;         // NPC ID
    int min_heart_level = 0;        // 最低好感等级
    int min_weight = 1;            // 最低权重
    std::string subject;           // 邮件主题
    std::string body;              // 邮件正文
    int cooldown_days = 7;          // 冷却天数
};

// ============================================================================
// 【PlayerMemorySystem】玩家行为记忆系统
// ============================================================================
class PlayerMemorySystem {
public:
    // ========================================================================
    // 【构造函数】
    // ========================================================================
    PlayerMemorySystem();

    // ========================================================================
    // 【记忆操作】
    // ========================================================================

    // 添加新记忆
    void AddMemory(PlayerMemoryType type, const std::string& subject_id, int game_day, int weight = 1, int decay_days = 7);

    // 日切时调用，执行衰减和清理
    void OnDayChanged(int current_day);

    // 获取热点记忆（供对话系统使用）
    [[nodiscard]] std::vector<const PlayerMemoryRecord*> GetHotMemories(
        int current_day,
        int max_days = 2,
        int max_count = 2) const;

    // 获取适合特定 NPC 的对话钩子
    [[nodiscard]] std::string GetDialogueHook(
        const std::string& npc_id,
        int npc_heart_level,
        int current_day,
        const std::string& season,
        const std::string& weather) const;

    // 获取可发送的情感邮件
    [[nodiscard]] std::string GetPendingMailId(int current_day);

    // 获取邮件模板详情
    [[nodiscard]] const MemoryMailTemplate* GetMailTemplateById(const std::string& id) const;

    // 标记邮件已发送
    void MarkMailConsumed(const std::string& memory_id);

    // 标记对话已使用
    void MarkDialogueConsumed(int memory_index);

    // ========================================================================
    // 【数据加载】
    // ========================================================================
    void LoadHooks(const std::vector<MemoryHook>& hooks);
    void LoadMailTemplates(const std::vector<MemoryMailTemplate>& templates);

    // ========================================================================
    // 【存档序列化】
    // ========================================================================
    [[nodiscard]] std::string Serialize() const;
    void Deserialize(const std::string& data);

    // ========================================================================
    // 【状态查询】
    // ========================================================================
    [[nodiscard]] bool HasRecentMemory(PlayerMemoryType type, int current_day, int max_days = 3) const;
    [[nodiscard]] int GetMemoryCount() const { return static_cast<int>(memories_.size()); }

private:
    // 移除过期记忆
    void RemoveExpired_(int current_day);

    // 合并相同类型记忆
    void MergeMemories_(PlayerMemoryType type, const std::string& subject_id, int game_day, int weight);

    // 查找匹配的对话钩子
    [[nodiscard]] const MemoryHook* FindMatchingHook_(
        const std::string& npc_id,
        int npc_heart_level,
        int current_day,
        const std::string& season,
        const std::string& weather) const;

    std::vector<PlayerMemoryRecord> memories_;
    std::vector<MemoryHook> hooks_;
    std::vector<MemoryMailTemplate> mail_templates_;

    // 邮件冷却追踪
    std::unordered_map<std::string, int> mail_cooldowns_;  // template_id -> last_sent_day

    static constexpr int kMaxMemories = 50;  // 最大记忆数量
};

// ============================================================================
// 【辅助函数】
// ============================================================================
[[nodiscard]] inline const char* ToString(PlayerMemoryType t) {
    switch (t) {
        case PlayerMemoryType::BrewHolyTea: return "BrewHolyTea";
        case PlayerMemoryType::BrewNewTea: return "BrewNewTea";
        case PlayerMemoryType::FarmFocused: return "FarmFocused";
        case PlayerMemoryType::WorkshopFocused: return "WorkshopFocused";
        case PlayerMemoryType::ExploreBackMountain: return "ExploreBackMountain";
        case PlayerMemoryType::PurifyMiasma: return "PurifyMiasma";
        case PlayerMemoryType::FestivalAttend: return "FestivalAttend";
        case PlayerMemoryType::RelationshipMilestone: return "RelationshipMilestone";
        case PlayerMemoryType::EcoBalanced: return "EcoBalanced";
        case PlayerMemoryType::EcoImbalanced: return "EcoImbalanced";
        case PlayerMemoryType::TeaSpiritUnlocked: return "TeaSpiritUnlocked";
        default: return "None";
    }
}

[[nodiscard]] inline PlayerMemoryType ParseMemoryType(const std::string& s) {
    if (s == "BrewHolyTea") return PlayerMemoryType::BrewHolyTea;
    if (s == "BrewNewTea") return PlayerMemoryType::BrewNewTea;
    if (s == "FarmFocused") return PlayerMemoryType::FarmFocused;
    if (s == "WorkshopFocused") return PlayerMemoryType::WorkshopFocused;
    if (s == "ExploreBackMountain") return PlayerMemoryType::ExploreBackMountain;
    if (s == "PurifyMiasma") return PlayerMemoryType::PurifyMiasma;
    if (s == "FestivalAttend") return PlayerMemoryType::FestivalAttend;
    if (s == "RelationshipMilestone") return PlayerMemoryType::RelationshipMilestone;
    if (s == "EcoBalanced") return PlayerMemoryType::EcoBalanced;
    if (s == "EcoImbalanced") return PlayerMemoryType::EcoImbalanced;
    if (s == "TeaSpiritUnlocked") return PlayerMemoryType::TeaSpiritUnlocked;
    return PlayerMemoryType::None;
}

}  // namespace CloudSeamanor::domain
