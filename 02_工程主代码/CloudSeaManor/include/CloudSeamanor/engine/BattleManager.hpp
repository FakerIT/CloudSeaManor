#pragma once

// ============================================================================
// 【BattleManager.hpp】战斗总管理器
// ============================================================================
// 战斗系统的顶层协调者：处理进入/退出、暂停、场景切换、外部集成。
//
// 主要职责：
// - 管理战斗的进入（触发检测）和退出（结算）
// - 协调 BattleField（逻辑）、BattleUI（渲染）、外部系统（灵兽/技能）
// - 处理玩家输入（技能释放）
// - 与 GameWorldState 集成
//
// 与其他系统的关系：
// - 依赖：BattleField（核心逻辑）、CloudSystem（天气）、SkillSystem（技能数据）
// - 依赖：SpiritBeastSystem（灵兽羁绊数据）、ParticleSystem（战斗特效）
// - 被依赖：GameApp（主循环 Update）
// ============================================================================

#include "CloudSeamanor/engine/BattleField.hpp"
#include "CloudSeamanor/engine/BattleEntities.hpp"
#include "CloudSeamanor/engine/BattleRenderer.hpp"
#include "CloudSeamanor/domain/CloudSystem.hpp"
#include <memory>
#include <functional>

namespace CloudSeamanor::engine {

// 前向声明
class BattleUI;
class BattleRenderer;

// ============================================================================
// 【BattleManager】战斗总管理器
// ============================================================================
class BattleManager {
public:
    struct RewardCallbacks {
        std::function<void(const std::string& item_id, int count)> on_item_reward;
        std::function<void(float exp)> on_exp_reward;
        std::function<void(const std::string& partner_id, int favor_delta)> on_partner_favor_reward;
        std::function<void(const std::string& message)> on_notice;
        // 背包满时的通知回调（新增）
        std::function<void(const std::string& item_id, int overflow_count, const std::string& reason)> on_inventory_full;
    };
    // ========================================================================
    // 【初始化】
    // ========================================================================

    /** 构造函数 */
    BattleManager();

    /** 析构函数（在 .cpp 中定义以避免不完整类型删除问题） */
    ~BattleManager();

    /**
     * @brief 注入依赖系统
     * @param cloud_system 云海系统引用
     * @param skill_table 技能表数据
     * @param spirit_table 敌人表数据
     */
    void Initialize(
        CloudSeamanor::domain::CloudSystem* cloud_system
    );

    // ========================================================================
    // 【帧更新】
    // ========================================================================

    /**
     * @brief 每帧更新
     * @param delta_seconds 帧间隔
     * @param player_pos_x 玩家X坐标
     * @param player_pos_y 玩家Y坐标
     */
    void Update(float delta_seconds, float player_pos_x, float player_pos_y);

    // ========================================================================
    // 【战斗控制】
    // ========================================================================

    /**
     * @brief 进入战斗（由碰撞检测或事件触发）
     * @param zone 战场区域
     * @param spirit_ids 要生成的敌人ID列表
     * @return 是否成功进入
     */
    bool EnterBattle(const BattleZone& zone, const std::vector<std::string>& spirit_ids);

    /**
     * @brief 退出战斗（撤退）
     */
    void Retreat();

    /**
     * @brief 确认胜利结算
     */
    void ConfirmVictory();

    // ========================================================================
    // 【技能释放】
    // ========================================================================

    /**
     * @brief 玩家按技能键释放技能
     * @param skill_slot 技能槽位（0-3）
     * @param mouse_x 鼠标X坐标
     * @param mouse_y 鼠标Y坐标
     * @return 是否成功释放
     */
    bool OnSkillKeyPressed(int skill_slot, float mouse_x, float mouse_y);

    /**
     * @brief 选择目标灵体（点击选中）
     * @param spirit_id 灵体ID
     */
    void SelectTarget(const std::string& spirit_id);

    // ========================================================================
    // 【状态查询】
    // ========================================================================

    [[nodiscard]] bool IsInBattle() const;
    [[nodiscard]] BattleState CurrentState() const;
    [[nodiscard]] const BattleField& GetField() const { return field_; }
    [[nodiscard]] BattleField& MutableField() { return field_; }
    [[nodiscard]] const BattleResult& GetLastResult() const;

    // ========================================================================
    // 【暂停控制】
    // ========================================================================

    void Pause() { is_paused_ = true; }
    void Resume() { is_paused_ = false; }
    [[nodiscard]] bool IsPaused() const { return is_paused_; }

    // ========================================================================
    // 【UI访问】
    // ========================================================================

    [[nodiscard]] const BattleUI& GetUI() const;
    [[nodiscard]] BattleUI& GetUI();

    // ========================================================================
    // 【渲染访问】
    // ========================================================================

    [[nodiscard]] const BattleRenderer& GetRenderer() const;
    [[nodiscard]] BattleRenderer& GetRenderer();

    // ========================================================================
    // 【战斗触发检测】
    // ========================================================================

    /**
     * @brief 检测是否应触发战斗（玩家接近敌人时调用）
     * @param spirit_id 敌人ID
     * @param spirit_x 敌人X坐标
     * @param spirit_y 敌人Y坐标
     * @param player_x 玩家X坐标
     * @param player_y 玩家Y坐标
     * @return 是否触发
     */
    [[nodiscard]] bool ShouldTriggerBattle(
        const std::string& spirit_id,
        float spirit_x, float spirit_y,
        float player_x, float player_y
    ) const;

    // ========================================================================
    // 【灵兽数据加载】
    // ========================================================================

    /**
     * @brief 从灵兽系统加载出战灵兽到战场
     * @param spirit_beast_ids 要出战的灵兽ID列表
     */
    void LoadPartnersFromSpiritBeasts(const std::vector<std::string>& spirit_beast_ids);
    void SetRewardCallbacks(const RewardCallbacks& callbacks) { reward_callbacks_ = callbacks; }
    void SetPartnerHeartLevelResolver(const std::function<int(const std::string&)>& resolver) {
        partner_heart_level_resolver_ = resolver;
    }
    bool SetEquippedWeapon(const std::string& weapon_id);
    [[nodiscard]] const std::string& GetEquippedWeapon() const { return equipped_weapon_id_; }
    void SetQuestSkills(const std::vector<std::string>& quest_skill_ids);

private:
    // ========================================================================
    // 内部方法
    // ========================================================================

    void UpdateState_(float delta_seconds);
    void UpdatePartners_(float delta_seconds);
    void UpdateField_(float delta_seconds);

    // 战斗加载（初始化战场）
    void LoadBattle_(const BattleZone& zone, const std::vector<std::string>& spirit_ids);

    // 战斗结算
    void ProcessVictory_();
    void ProcessRetreat_();

    // ========================================================================
    // 成员变量
    // ========================================================================

    BattleField field_;
    std::unique_ptr<BattleUI> ui_;
    std::unique_ptr<BattleRenderer> renderer_;

    BattleState state_ = BattleState::Inactive;
    bool is_paused_ = false;

    // 外部系统引用
    CloudSeamanor::domain::CloudSystem* cloud_system_ = nullptr;

    // 当前选中的目标
    std::string selected_target_id_;
    RewardCallbacks reward_callbacks_{};
    std::function<int(const std::string&)> partner_heart_level_resolver_{};
    std::string equipped_weapon_id_;
    std::vector<std::string> quest_skill_ids_;

    // 战斗触发距离阈值
    float trigger_distance_ = 60.0f;

    // 结算延迟计时（用于显示结算面板）
    float result_display_timer_ = 0.0f;
    static constexpr float kResultDisplayDuration = 3.0f;
};

}  // namespace CloudSeamanor::engine
