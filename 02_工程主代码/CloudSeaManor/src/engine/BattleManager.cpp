#include "CloudSeamanor/engine/BattleManager.hpp"

#include "CloudSeamanor/engine/BattleUI.hpp"
#include "CloudSeamanor/GameConstants.hpp"

#include <cmath>
#include <unordered_map>

namespace CloudSeamanor::engine {

// ============================================================================
// 【BattleManager】构造函数
// ============================================================================
BattleManager::BattleManager()
    : state_(BattleState::Inactive)
    , is_paused_(false)
    , result_display_timer_(0.0f) {
}

BattleManager::~BattleManager() = default;

// ============================================================================
// 【Initialize】初始化
// ============================================================================
void BattleManager::Initialize(
    CloudSeamanor::domain::CloudSystem* cloud_system
) {
    cloud_system_ = cloud_system;
    (void)field_.LoadSpiritTableFromCsv("assets/data/battle/spirit_table.csv");
    (void)field_.LoadSkillTableFromCsv("assets/data/battle/skill_table.csv");
    (void)field_.LoadZoneTableFromCsv("assets/data/battle/zone_table.csv");
    // UI 在需要时按需创建
}

// ============================================================================
// 【Update】每帧更新
// ============================================================================
void BattleManager::Update(float delta_seconds, float player_pos_x, float player_pos_y) {
    if (is_paused_) return;

    switch (state_) {
        case BattleState::Inactive:
            // 无操作
            break;

        case BattleState::Loading:
            // 加载完成后进入 Active
            if (field_.IsActive()) {
                state_ = BattleState::Active;
            }
            break;

        case BattleState::Active:
            UpdateField_(delta_seconds);
            UpdatePartners_(delta_seconds);
            if (ui_) {
                ui_->SyncFromField(field_);
                ui_->Update(delta_seconds);
            }
            UpdateState_(delta_seconds);
            break;

        case BattleState::Victory:
            result_display_timer_ += delta_seconds;
            if (ui_) {
                ui_->Update(delta_seconds);
            }
            if (result_display_timer_ >= kResultDisplayDuration) {
                // 玩家可以确认结算
            }
            break;

        case BattleState::Retreat:
            ProcessRetreat_();
            break;
    }
}

// ============================================================================
// 【EnterBattle】进入战斗
// ============================================================================
bool BattleManager::EnterBattle(const BattleZone& zone, const std::vector<std::string>& spirit_ids) {
    if (state_ != BattleState::Inactive) return false;

    LoadBattle_(zone, spirit_ids);
    state_ = BattleState::Loading;

    return true;
}

// ============================================================================
// 【Retreat】撤退
// ============================================================================
void BattleManager::Retreat() {
    if (state_ != BattleState::Active) return;
    state_ = BattleState::Retreat;
    ProcessRetreat_();
}

// ============================================================================
// 【ConfirmVictory】确认胜利
// ============================================================================
void BattleManager::ConfirmVictory() {
    if (state_ != BattleState::Victory) return;
    ProcessVictory_();
}

// ============================================================================
// 【OnSkillKeyPressed】技能键按下
// ============================================================================
bool BattleManager::OnSkillKeyPressed(int skill_slot, float mouse_x, float mouse_y) {
    if (state_ != BattleState::Active) return false;

    // TODO: 从技能表获取技能数据
    // 目前使用简化逻辑：直接释放技能
    (void)skill_slot;
    (void)mouse_x;
    (void)mouse_y;

    return true;
}

// ============================================================================
// 【SelectTarget】选择目标
// ============================================================================
void BattleManager::SelectTarget(const std::string& spirit_id) {
    selected_target_id_ = spirit_id;
}

// ============================================================================
// 【IsInBattle】是否在战斗中
// ============================================================================
bool BattleManager::IsInBattle() const {
    return state_ == BattleState::Active ||
           state_ == BattleState::Loading ||
           state_ == BattleState::Victory;
}

// ============================================================================
// 【CurrentState】当前状态
// ============================================================================
BattleState BattleManager::CurrentState() const {
    return state_;
}

// ============================================================================
// 【GetLastResult】获取上次战斗结果
// ============================================================================
const BattleResult& BattleManager::GetLastResult() const {
    return field_.GetResult();
}

// ============================================================================
// 【GetUI】获取UI
// ============================================================================
const BattleUI& BattleManager::GetUI() const {
    static BattleUI dummy;
    return ui_ ? *ui_ : dummy;
}

BattleUI& BattleManager::GetUI() {
    static BattleUI dummy;
    return ui_ ? *ui_ : dummy;
}

// ============================================================================
// 【ShouldTriggerBattle】是否应触发战斗
// ============================================================================
bool BattleManager::ShouldTriggerBattle(
    const std::string& spirit_id,
    float spirit_x, float spirit_y,
    float player_x, float player_y
) const {
    if (state_ != BattleState::Inactive) return false;

    float dx = spirit_x - player_x;
    float dy = spirit_y - player_y;
    float dist = std::sqrt(dx * dx + dy * dy);

    return dist < trigger_distance_;
}

// ============================================================================
// 【LoadPartnersFromSpiritBeasts】从灵兽系统加载伙伴
// ============================================================================
void BattleManager::LoadPartnersFromSpiritBeasts(const std::vector<std::string>& spirit_beast_ids) {
    int partner_offset = 0;
    for (const auto& id : spirit_beast_ids) {
        BattlePartner partner;
        partner.spirit_beast_id = id;
        partner.heart_level = 1; // TODO: 从 SpiritBeastSystem 获取实际羁绊等级
        partner.purification_rate_mod = 1.0f + partner.heart_level * 0.1f;
        partner.pos_x = 200.0f + static_cast<float>(partner_offset) * 30.0f;
        partner.pos_y = 400.0f;

        // 默认技能
        partner.active_skill_ids = {"spirit_basic_purify"};
        partner.cooldown_total = {15.0f};
        partner.cooldown_remaining.assign(partner.cooldown_total.begin(), partner.cooldown_total.end());

        field_.AddPartner(partner);
        ++partner_offset;
    }
}

// ============================================================================
// 【UpdateState_】更新状态
// ============================================================================
void BattleManager::UpdateState_(float delta_seconds) {
    (void)delta_seconds;

    if (field_.IsVictory()) {
        state_ = BattleState::Victory;
        result_display_timer_ = 0.0f;
    }
}

// ============================================================================
// 【UpdatePartners_】更新灵兽伙伴
// ============================================================================
void BattleManager::UpdatePartners_(float delta_seconds) {
    // 获取玩家位置（简化：使用固定值）
    field_.PartnerUpdate(delta_seconds, 640.0f, 360.0f);
}

// ============================================================================
// 【UpdateField_】更新战场
// ============================================================================
void BattleManager::UpdateField_(float delta_seconds) {
    field_.Update(delta_seconds, 640.0f, 360.0f);
}

// ============================================================================
// 【LoadBattle_】加载战斗
// ============================================================================
void BattleManager::LoadBattle_(const BattleZone& zone, const std::vector<std::string>& spirit_ids) {
    // 获取当前天气
    CloudSeamanor::domain::CloudState cloud = CloudSeamanor::domain::CloudState::Clear;
    if (cloud_system_ != nullptr) {
        cloud = cloud_system_->CurrentState();
    }

    // 获取敌人数量
    int num_common = 0;
    int num_elite = 0;
    std::optional<std::string> boss_id;

    for (const auto& id : spirit_ids) {
        // TODO: 从数据表判定类型
        (void)id;
        num_common++;
    }

    field_.StartBattle(zone, cloud, num_common, num_elite, boss_id);
}

// ============================================================================
// 【ProcessVictory_】处理胜利
// ============================================================================
void BattleManager::ProcessVictory_() {
    const auto& result = field_.GetResult();

    std::unordered_map<std::string, int> reward_count;
    for (const auto& item_id : result.items_gained) {
        if (!item_id.empty()) {
            reward_count[item_id] += 1;
        }
    }
    if (result.spirits_purified > 0) {
        reward_count["spirit_dust"] += result.spirits_purified;
    }

    for (const auto& [item_id, count] : reward_count) {
        if (count <= 0) continue;
        if (reward_callbacks_.on_item_reward) {
            reward_callbacks_.on_item_reward(item_id, count);
        }
    }

    if (result.total_exp_gained > 0.0f && reward_callbacks_.on_exp_reward) {
        reward_callbacks_.on_exp_reward(result.total_exp_gained);
    }

    for (const auto& [partner_id, favor_delta] : result.partner_favor_gained) {
        if (favor_delta <= 0) continue;
        if (reward_callbacks_.on_partner_favor_reward) {
            reward_callbacks_.on_partner_favor_reward(partner_id, favor_delta);
        }
    }

    if (reward_callbacks_.on_notice) {
        reward_callbacks_.on_notice(
            "战斗胜利：净化 " + std::to_string(result.spirits_purified)
            + "/" + std::to_string(result.spirits_total)
            + " 个目标，获得经验 " + std::to_string(static_cast<int>(result.total_exp_gained)) + "。");
    }

    field_.EndBattle();
    state_ = BattleState::Inactive;
    selected_target_id_.clear();
}

// ============================================================================
// 【ProcessRetreat_】处理撤退
// ============================================================================
void BattleManager::ProcessRetreat_() {
    // 撤退：灵体进入暂时退散状态（3分钟后重新出现）
    // TODO: 设置灵体退散计时器

    field_.EndBattle();
    state_ = BattleState::Inactive;
    selected_target_id_.clear();
}

}  // namespace CloudSeamanor::engine
