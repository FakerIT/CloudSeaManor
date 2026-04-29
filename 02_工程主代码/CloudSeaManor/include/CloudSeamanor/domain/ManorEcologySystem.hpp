#pragma once

// ============================================================================
// 【ManorEcologySystem】庄园灵气生态系统
// ============================================================================
// 统一管理庄园的灵气浓度、元素平衡与生态状态。
//
// 主要职责：
// - 维护庄园五元素权重（云/风/露/霞/潮）
// - 计算生态平衡分数与主导元素
// - 判定生态阶段（枯静/平稳/充盈/澄明）
// - 触发祥瑞事件
//
// 设计原则：
// - 非惩罚型：失衡不是负债，而是另一种生态状态
// - 轻可视化：只给状态描述与简图，不显示精确数值
// - 可调可救：玩家可通过经营、净化、装饰调整生态
// - 长期缓变：以"天"为单位，不追求瞬时剧烈波动
// ============================================================================

#include <array>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace CloudSeamanor::domain {

// ============================================================================
// 【ManorElement】庄园五元素枚举
// ============================================================================
enum class ManorElement {
    Cloud = 0,  // 云
    Wind = 1,   // 风
    Dew = 2,    // 露
    Glow = 3,   // 霞
    Tide = 4    // 潮
};

constexpr int kManorElementCount = 5;

// ============================================================================
// 【EcologyStage】生态阶段枚举
// ============================================================================
// 灵气浓度的四个阶段，影响作物品质和祥瑞触发。
enum class EcologyStage {
    Withered = 0,  // 枯静：偏低，庄园沉寂
    Stable = 1,    // 平稳：常态区间
    Abundant = 2,  // 充盈：适合高品质茶事
    Luminous = 3   // 澄明：高阶状态，可触发祥瑞
};

// ============================================================================
// 【BalanceState】元素平衡状态
// ============================================================================
// 描述庄园当前的元素偏向状态。
enum class BalanceState {
    SingleElementDominant,  // 单元素偏盛
    DualElementResonant,   // 双元素共振
    NearBalanced,         // 近似平衡
    PerfectlyBalanced     // 完美平衡
};

// ============================================================================
// 【EcologyDeltaEvent】生态增量事件
// ============================================================================
// 记录每次影响生态的行为及其效果。
struct EcologyDeltaEvent {
    std::string source_type;      // planting / purification / decoration / festival
    std::string source_id;        // 源头ID（如作物ID、净化目标ID等）
    ManorElement primary_element;  // 主元素
    int aura_delta = 0;           // 灵气浓度变化
    int element_delta = 0;        // 元素权重变化
};

// ============================================================================
// 【EcologyEventRecord】祥瑞事件记录
// ============================================================================
// 追踪已触发的事件和冷却状态。
struct EcologyEventRecord {
    std::string event_id;
    int last_trigger_day = 0;
    int cooldown_remaining_days = 0;
};

// ============================================================================
// 【AuspiciousEvent】祥瑞事件定义
// ============================================================================
struct AuspiciousEvent {
    std::string id;
    std::string name;
    std::string description;
    int cooldown_days = 0;
    EcologyStage min_aura_stage = EcologyStage::Abundant;
    BalanceState required_balance = BalanceState::NearBalanced;
    int required_balance_days = 0;
    int min_dominant_element_weight = 0;
    std::string result_type;   // spirit_appear / harvest_boost / visitor / gift
    std::string result_value;
};

// ============================================================================
// 【ManorEcologyState】庄园生态状态
// ============================================================================
struct ManorEcologyState {
    int aura_level = 50;                              // 灵气浓度（0-100）
    int spirit_beast_activity = 50;                   // 灵兽活跃度（0-100）
    std::array<int, kManorElementCount> element_weights{};  // 五元素权重
    int balance_score = 0;                            // 平衡分数（越高越平衡）
    int balanced_days_streak = 0;                    // 连续平衡天数
    int dominant_element_index = -1;                   // 当前主导元素索引（-1表示无）
    EcologyStage current_stage = EcologyStage::Stable; // 当前生态阶段
    BalanceState balance_state = BalanceState::NearBalanced;  // 当前平衡状态
    int total_events_triggered = 0;                   // 已触发祥瑞事件总数
};

// ============================================================================
// 【PlantingEffect】种植效果配置
// ============================================================================
struct PlantingEffect {
    std::string tea_id;
    ManorElement primary_element;
    int aura_delta = 0;
    int element_delta = 0;
    float quality_bonus_at_high_aura = 0.0f;
};

// ============================================================================
// 【DecorationEffect】装饰效果配置
// ============================================================================
struct DecorationEffect {
    std::string decor_id;
    ManorElement primary_element;
    int aura_delta = 0;
    int element_delta = 0;
    int stability_delta = 0;
};

// ============================================================================
// 【NpcEcologyComment】NPC生态评论配置
// ============================================================================
struct NpcEcologyComment {
    std::string id;
    EcologyStage ecology_stage = EcologyStage::Stable;
    ManorElement dominant_element = ManorElement::Cloud;
    std::string speaker_id;
    int min_heart_level = 0;
    std::string text;
};

// ============================================================================
// 【ManorEcologySystem】庄园生态领域系统
// ============================================================================
class ManorEcologySystem {
public:
    // ========================================================================
    // 【构造函数】
    // ========================================================================
    ManorEcologySystem();

    // ========================================================================
    // 【状态访问】
    // ========================================================================
    [[nodiscard]] const ManorEcologyState& GetState() const { return state_; }
    [[nodiscard]] ManorEcologyState& MutableState() { return state_; }

    // ========================================================================
    // 【日切结算】
    // ========================================================================
    // 在每天开始时调用，结算生态变化并判定祥瑞事件
    void OnDayChanged(int current_day);

    // ========================================================================
    // 【生态增量应用】
    // ========================================================================
    // 应用来自种植、净化、装饰等行为的生态变化
    void ApplyDelta(const EcologyDeltaEvent& event);
    void ApplyDelta(ManorElement element, int aura_delta, int element_delta);

    // ========================================================================
    // 【数据加载】
    // ========================================================================
    void LoadPlantingEffects(const std::vector<PlantingEffect>& effects);
    void LoadDecorationEffects(const std::vector<DecorationEffect>& effects);
    void LoadAuspiciousEvents(const std::vector<AuspiciousEvent>& events);
    void LoadNpcComments(const std::vector<NpcEcologyComment>& comments);

    // ========================================================================
    // 【效果查询】
    // ========================================================================
    [[nodiscard]] const PlantingEffect* GetPlantingEffect(const std::string& tea_id) const;
    [[nodiscard]] const DecorationEffect* GetDecorationEffect(const std::string& decor_id) const;

    // ========================================================================
    // 【生态信息查询】
    // ========================================================================
    [[nodiscard]] EcologyStage GetAuraStage() const;
    [[nodiscard]] BalanceState GetBalanceState() const;
    [[nodiscard]] std::string GetStageDescription() const;  // 阶段描述
    [[nodiscard]] std::string GetBalanceDescription() const;  // 平衡描述
    [[nodiscard]] std::string GetEcologySummary() const;  // 生态摘要文本
    [[nodiscard]] ManorElement GetDominantElement() const;

    // ========================================================================
    // 【品质修正】
    // ========================================================================
    // 返回基于生态的灵茶品质加成系数
    [[nodiscard]] float GetQualityBonus(ManorElement tea_element) const;

    // ========================================================================
    // 【祥瑞判定】
    // ========================================================================
    // 检查是否可以触发祥瑞事件，返回事件ID（空表示无）
    [[nodiscard]] std::string CheckAuspiciousEvent(int current_day);

    // ========================================================================
    // 【NPC评论查询】
    // ========================================================================
    // 获取适合当前生态状态的NPC评论列表
    [[nodiscard]] std::vector<std::string> GetNpcComments(int min_heart_level) const;

    // ========================================================================
    // 【存档序列化】
    // ========================================================================
    [[nodiscard]] std::string Serialize() const;
    void Deserialize(const std::string& data);

private:
    // ========================================================================
    // 【内部方法】
    // ========================================================================
    void RecalculateDerivedState_();
    void UpdateBalanceScore_();
    void UpdateBalanceDays_();
    [[nodiscard]] int CalculateDominantElement_() const;
    [[nodiscard]] BalanceState CalculateBalanceState_() const;
    [[nodiscard]] EcologyStage CalculateAuraStage_() const;

    ManorEcologyState state_;
    std::vector<PlantingEffect> planting_effects_;
    std::vector<DecorationEffect> decoration_effects_;
    std::vector<AuspiciousEvent> auspicious_events_;
    std::vector<NpcEcologyComment> npc_comments_;
    std::vector<EcologyEventRecord> event_cooldowns_;

    // 配置常量
    static constexpr int kMinAura = 0;
    static constexpr int kMaxAura = 100;
    static constexpr int kMinActivity = 0;
    static constexpr int kMaxActivity = 100;
    static constexpr int kElementWeightMin = 0;
    static constexpr int kElementWeightMax = 100;
    static constexpr int kPerfectBalanceThreshold = 10;  // 完美平衡的元素权重差阈值
    static constexpr int kNearBalanceThreshold = 25;     // 近似平衡的元素权重差阈值
};

// ============================================================================
// 【辅助函数】
// ============================================================================
[[nodiscard]] inline const char* ToString(ManorElement e) {
    switch (e) {
        case ManorElement::Cloud: return "Cloud";
        case ManorElement::Wind: return "Wind";
        case ManorElement::Dew: return "Dew";
        case ManorElement::Glow: return "Glow";
        case ManorElement::Tide: return "Tide";
    }
    return "Unknown";
}

[[nodiscard]] inline const char* ToDisplayName(ManorElement e) {
    switch (e) {
        case ManorElement::Cloud: return "云";
        case ManorElement::Wind: return "风";
        case ManorElement::Dew: return "露";
        case ManorElement::Glow: return "霞";
        case ManorElement::Tide: return "潮";
    }
    return "?";
}

[[nodiscard]] inline const char* ToString(EcologyStage s) {
    switch (s) {
        case EcologyStage::Withered: return "Withered";
        case EcologyStage::Stable: return "Stable";
        case EcologyStage::Abundant: return "Abundant";
        case EcologyStage::Luminous: return "Luminous";
    }
    return "Unknown";
}

[[nodiscard]] inline const char* ToDisplayName(EcologyStage s) {
    switch (s) {
        case EcologyStage::Withered: return "枯静";
        case EcologyStage::Stable: return "平稳";
        case EcologyStage::Abundant: return "充盈";
        case EcologyStage::Luminous: return "澄明";
    }
    return "?";
}

[[nodiscard]] inline const char* ToString(BalanceState b) {
    switch (b) {
        case BalanceState::SingleElementDominant: return "SingleElement";
        case BalanceState::DualElementResonant: return "DualElement";
        case BalanceState::NearBalanced: return "NearBalanced";
        case BalanceState::PerfectlyBalanced: return "PerfectlyBalanced";
    }
    return "Unknown";
}

[[nodiscard]] inline const char* ToDisplayName(BalanceState b) {
    switch (b) {
        case BalanceState::SingleElementDominant: return "单元素偏盛";
        case BalanceState::DualElementResonant: return "双元素共振";
        case BalanceState::NearBalanced: return "近似平衡";
        case BalanceState::PerfectlyBalanced: return "完美平衡";
    }
    return "?";
}

}  // namespace CloudSeamanor::domain
