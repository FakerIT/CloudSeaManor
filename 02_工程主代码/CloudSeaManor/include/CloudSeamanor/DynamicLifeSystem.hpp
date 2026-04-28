#pragma once

// ============================================================================
// 【DynamicLifeSystem】NPC动态人生发展系统
// ============================================================================
// 统一管理NPC的人生阶段发展、进度累积和跃迁触发。
//
// 主要职责：
// - 管理12位NPC的4个人生阶段（初遇期、立足期、成长期、归属期）
// - 追踪进度点数（由玩家行为、云海状态、随机事件共同决定）
// - 检查阶段跃迁条件并触发相应反馈
// - 维护NPC的日程、立绘和对话池
//
// 与其他系统的关系：
// - 依赖：GameClock（日期）、CloudSystem（云海状态）、SocialSystem（好感度）
// - 被依赖：GameAppNpc（NPC展示）、GameAppSave（存档/读档）
//
// 设计原则：
// - 阶段跃迁触发条件：好感阈值、特定契约完成、建筑升级等
// - 进度公式：每日点数 = 玩家行为×70% + 云海加成×20% + 随机事件×10%
// - 跃迁后必须触发4类可见反馈：新对话池、新日程、新外观/住所、系统加成
//
// 阶段说明：
// - 阶段0（初遇期）：第1年春-夏，陌生、漂泊、试探
// - 阶段1（立足期）：第1年秋-第2年春，开始稳定、尝试融入
// - 阶段2（成长期）：第2年夏-第3年春，职业稳定、关系建立
// - 阶段3（归属期）：第3年夏以后，完全融入山庄
// ============================================================================

#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::domain {

// ============================================================================
// 【LifeStage】人生阶段枚举
// ============================================================================
enum class LifeStage {
    Stage0 = 0,  // 初遇期
    Stage1 = 1,   // 立足期
    Stage2 = 2,   // 成长期
    Stage3 = 3,   // 归属期
};

// ============================================================================
// 【NpcLifeState】NPC人生状态
// ============================================================================
// 表示单个NPC的人生发展状态。
struct NpcLifeState {
    std::string npc_id;                 // NPC ID
    LifeStage stage = LifeStage::Stage0;  // 当前阶段
    float progress_points = 0.0f;     // 进度点数
    bool stage_changed_today = false;   // 今日是否已跃迁
    int current_branch = -1;            // 当前分支（-1未定）
};

// ============================================================================
// 【StageConfig】阶段配置
// ============================================================================
// 表示一个阶段的跃迁配置。
struct StageConfig {
    LifeStage stage = LifeStage::Stage0;          // 阶段
    float threshold = 0.0f;                     // 跃迁阈值
    std::vector<std::string> required_flags;     // 必需条件标签
    std::vector<std::string> unlock_dialog_pools; // 解锁对话池
    std::vector<std::string> unlock_schedules;  // 解锁日程
    std::string visual_state_id;                // 视觉状态ID
    std::string reward_description;             // 奖励描述
};

// ============================================================================
// 【DynamicLifeSystem】动态人生系统领域对象
// ============================================================================
// 管理所有NPC的人生发展状态。
//
// 设计决策：
// - 进度公式可配置，支持后续扩展
// - 阶段跃迁触发可见反馈（对话、日程、视觉、系统加成）
// - 支持分支选择（婚恋/独立/隐藏）
//
// 使用示例：
// @code
// DynamicLifeSystem life;
// life.Initialize();
// life.AddPlayerPoints("xiaoqi", 10);  // 玩家行为加分
// life.CheckStageTransitions(is_season_start, is_year_start);
// @endcode
class DynamicLifeSystem {
public:
    // ========================================================================
    // 【Initialize】初始化系统
    // ========================================================================
    void Initialize();
    bool LoadFromFile(const std::string& file_path = "assets/data/npc/npc_data.csv");

    // ========================================================================
    // 【UpdateDaily】每日更新
    // ========================================================================
    // @param cloud_density 云海浓度
    // @note 每日根据云海状态给予进度点数
    void UpdateDaily(float cloud_density);

    // ========================================================================
    // 【AddPlayerPoints】添加玩家行为点数
    // ========================================================================
    // @param npc_id NPC ID
    // @param points 点数
    void AddPlayerPoints(const std::string& npc_id, float points);

    // ========================================================================
    // 【CheckStageTransitions】检查阶段跃迁
    // ========================================================================
    // @param is_season_start 是否季节开始
    // @param is_year_start 是否年初
    // @note 在季节开始或年初检查是否满足跃迁条件
    void CheckStageTransitions(bool is_season_start, bool is_year_start);

    // ========================================================================
    // 【ApplyStageTransition】应用阶段跃迁
    // ========================================================================
    // @param npc_id NPC ID
    // @param new_stage 新阶段
    void ApplyStageTransition(const std::string& npc_id, LifeStage new_stage);

    // ========================================================================
    // 【访问器】
    // ========================================================================
    [[nodiscard]] const NpcLifeState* GetNpcState(const std::string& npc_id) const;
    [[nodiscard]] NpcLifeState* GetNpcState(const std::string& npc_id);
    [[nodiscard]] const std::string& GetStageName(LifeStage stage) const;
    [[nodiscard]] std::string GetNpcStageText(const std::string& npc_id) const;

private:
    // ========================================================================
    // 【CreateDefaultConfig_】创建默认NPC配置
    // ========================================================================
    [[nodiscard]] std::unordered_map<std::string, std::vector<StageConfig>> CreateDefaultConfig_() const;
    [[nodiscard]] std::vector<StageConfig> BuildProfileConfig_(const std::string& profile_id,
                                                               const std::string& npc_id) const;

    // ========================================================================
    // 成员变量
    // ========================================================================
    std::unordered_map<std::string, NpcLifeState> npc_states_;
    std::unordered_map<std::string, std::vector<StageConfig>> npc_configs_;
};

}  // namespace CloudSeamanor::domain
