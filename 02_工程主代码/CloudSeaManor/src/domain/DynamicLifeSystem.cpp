#include "CloudSeamanor/DynamicLifeSystem.hpp"
#include "CloudSeamanor/GameConstants.hpp"
#include "CloudSeamanor/GameAppNpc.hpp"

#include <algorithm>
#include <fstream>

namespace CloudSeamanor::domain {

namespace {

namespace GC = CloudSeamanor::GameConstants::DynamicLife;

}  // namespace

// ============================================================================
// 【Initialize】初始化系统
// ============================================================================
void DynamicLifeSystem::Initialize() {
    npc_states_.clear();
    if (!LoadFromFile()) {
        npc_configs_ = CreateDefaultConfig_();
    }

    // 初始化所有NPC状态
    for (const auto& [npc_id, configs] : npc_configs_) {
        NpcLifeState state;
        state.npc_id = npc_id;
        state.stage = LifeStage::Stage0;
        state.progress_points = 0.0f;
        state.stage_changed_today = false;
        state.current_branch = -1;
        npc_states_[npc_id] = state;
    }
}

bool DynamicLifeSystem::LoadFromFile(const std::string& file_path) {
    std::unordered_map<std::string, CloudSeamanor::engine::NpcDataRow> npc_rows;
    if (!CloudSeamanor::engine::LoadNpcDataTable(file_path, npc_rows)) {
        return false;
    }

    npc_states_.clear();
    npc_configs_.clear();
    for (const auto& [npc_id, row] : npc_rows) {
        auto configs = BuildProfileConfig_(row.life_stage_profile_id, npc_id);
        if (configs.empty()) {
            continue;
        }

        npc_configs_[npc_id] = configs;

        NpcLifeState state;
        state.npc_id = npc_id;
        state.stage = LifeStage::Stage0;
        state.progress_points = 0.0f;
        state.stage_changed_today = false;
        state.current_branch = -1;
        npc_states_[npc_id] = state;
    }
    return !npc_configs_.empty();
}

// ============================================================================
// 【UpdateDaily】每日更新
// ============================================================================
void DynamicLifeSystem::UpdateDaily(float cloud_density) {
    // 重置跃迁标记
    for (auto& [npc_id, state] : npc_states_) {
        state.stage_changed_today = false;
    }

    // 根据云海状态给予进度点数
    float cloud_bonus = GC::CloudBonusLow;
    if (cloud_density >= GC::CloudDensityHigh) {
        cloud_bonus = GC::CloudBonusHigh;
    } else if (cloud_density >= GC::CloudDensityMid) {
        cloud_bonus = GC::CloudBonusMid;
    }

    for (auto& [npc_id, state] : npc_states_) {
        state.progress_points += cloud_bonus * GC::CloudWeight;
    }
}

// ============================================================================
// 【AddPlayerPoints】添加玩家行为点数
// ============================================================================
void DynamicLifeSystem::AddPlayerPoints(const std::string& npc_id, float points) {
    auto it = npc_states_.find(npc_id);
    if (it == npc_states_.end()) return;

    it->second.progress_points += points * GC::PlayerWeight;
}

// ============================================================================
// 【CheckStageTransitions】检查阶段跃迁
// ============================================================================
void DynamicLifeSystem::CheckStageTransitions(bool is_season_start, bool is_year_start) {
    if (!is_season_start && !is_year_start) return;

    for (auto& [npc_id, state] : npc_states_) {
        if (state.stage == LifeStage::Stage3) continue;  // 已达最高阶段

        // 查找当前阶段配置
        auto config_it = npc_configs_.find(npc_id);
        if (config_it == npc_configs_.end()) continue;

        const auto& configs = config_it->second;
        int current_stage_index = static_cast<int>(state.stage);

        // 检查下一阶段
        if (current_stage_index + 1 < static_cast<int>(configs.size())) {
            const StageConfig& next_config = configs[current_stage_index + 1];

            if (state.progress_points >= next_config.threshold) {
                ApplyStageTransition(npc_id, next_config.stage);
            }
        }
    }
}

// ============================================================================
// 【ApplyStageTransition】应用阶段跃迁
// ============================================================================
void DynamicLifeSystem::ApplyStageTransition(const std::string& npc_id, LifeStage new_stage) {
    auto it = npc_states_.find(npc_id);
    if (it == npc_states_.end()) return;

    auto cfg_it = npc_configs_.find(npc_id);
    if (cfg_it == npc_configs_.end()) return;

    it->second.stage = new_stage;
    it->second.stage_changed_today = true;
    it->second.current_branch = static_cast<int>(new_stage);

    // 阶段跃迁后将进度至少对齐到该阶段阈值，避免跨季节回退显示。
    const auto& configs = cfg_it->second;
    const int stage_index = static_cast<int>(new_stage);
    if (stage_index >= 0 && stage_index < static_cast<int>(configs.size())) {
        it->second.progress_points = std::max(it->second.progress_points, configs[stage_index].threshold);
    }
}

// ============================================================================
// 【GetNpcState】获取NPC状态
// ============================================================================
const NpcLifeState* DynamicLifeSystem::GetNpcState(const std::string& npc_id) const {
    const auto it = npc_states_.find(npc_id);
    return (it != npc_states_.end()) ? &it->second : nullptr;
}

NpcLifeState* DynamicLifeSystem::GetNpcState(const std::string& npc_id) {
    const auto it = npc_states_.find(npc_id);
    return (it != npc_states_.end()) ? &it->second : nullptr;
}

// ============================================================================
// 【GetStageName】获取阶段名称
// ============================================================================
const std::string& DynamicLifeSystem::GetStageName(LifeStage stage) const {
    static const std::string names[] = {"初遇期", "立足期", "成长期", "归属期"};
    static const std::string unknown = "未知";
    int index = static_cast<int>(stage);
    return (index >= 0 && index < 4) ? names[index] : unknown;
}

// ============================================================================
// 【GetNpcStageText】获取NPC阶段文本
// ============================================================================
std::string DynamicLifeSystem::GetNpcStageText(const std::string& npc_id) const {
    const NpcLifeState* state = GetNpcState(npc_id);
    if (!state) return "未知NPC";

    std::string text = GetStageName(state->stage);
    text += " (" + std::to_string(static_cast<int>(state->progress_points)) + "点)";
    return text;
}

// ============================================================================
// 【CreateDefaultConfig_】创建默认NPC配置
// ============================================================================
// 注意：此处的 NPC ID 需要与 Schedule_Data.csv 和 BuildNpcs 中的实际 ID 一致。
// 当前游戏数据中实际存在的 NPC 为：lin（茶师）、bo（船夫）。
// 其他角色 ID 作为扩展预留，实际生效需要对应的 NPC 在 g_npcs 中存在才会更新进度。
std::unordered_map<std::string, std::vector<StageConfig>> DynamicLifeSystem::CreateDefaultConfig_() const {
    std::unordered_map<std::string, std::vector<StageConfig>> configs;

    // ========================================================================
    // lin（茶师）配置 — 与茶园、灵茶文化相关的角色
    // ========================================================================
    {
        std::vector<StageConfig> cfgs;
        cfgs.push_back({LifeStage::Stage0, 0.0f, {}, {}, {}, "tea_field", "茶园学徒"});
        cfgs.push_back({LifeStage::Stage1, GC::Stage1Threshold, {"tea_quality_2", "affinity_4"}, {}, {}, "tea_workshop", "制茶师"});
        cfgs.push_back({LifeStage::Stage2, GC::Stage2Threshold, {"pact_tea_done", "affinity_6"}, {}, {}, "tea_partner", "茶园合伙人"});
        cfgs.push_back({LifeStage::Stage3, GC::Stage3Threshold, {"spirit_value_80", "tea_master"}, {}, {}, "tea_master", "茶文化传承人"});
        configs["lin"] = std::move(cfgs);
    }

    // ========================================================================
    // bo（船夫）配置 — 负责运输和修理的角色
    // ========================================================================
    {
        std::vector<StageConfig> cfgs;
        cfgs.push_back({LifeStage::Stage0, 0.0f, {}, {}, {}, "dock", "码头工人"});
        cfgs.push_back({LifeStage::Stage1, GC::Stage1Threshold, {"trade_count_3", "affinity_4"}, {}, {}, "harbor", "驻船人"});
        cfgs.push_back({LifeStage::Stage2, GC::Stage2Threshold, {"repair_count_5", "affinity_6"}, {}, {}, "workshop", "工坊主理"});
        cfgs.push_back({LifeStage::Stage3, GC::Stage3Threshold, {"spirit_value_75", "trade_master"}, {}, {}, "trade_master", "山海贸易商"});
        configs["bo"] = std::move(cfgs);
    }

    // ========================================================================
    // xiaoqi（小乞）配置 — 扩展预留，需要添加对应 NPC 到 BuildNpcs
    // ========================================================================
    {
        std::vector<StageConfig> cfgs;
        cfgs.push_back({LifeStage::Stage0, 0.0f, {}, {}, {}, "ruined_temple", "流浪汉"});
        cfgs.push_back({LifeStage::Stage1, GC::Stage1Threshold, {"cloud_density_0_55", "affinity_4"}, {}, {}, "rented_house", "外卖员"});
        cfgs.push_back({LifeStage::Stage2, GC::Stage2Threshold, {"pact_guesthouse_done", "affinity_6"}, {}, {}, "tea_stall", "茶档老板"});
        cfgs.push_back({LifeStage::Stage3, GC::Stage3Threshold, {"spirit_value_75"}, {}, {}, "tea_house", "茶楼主理人"});
        configs["xiaoqi"] = std::move(cfgs);
    }

    // ========================================================================
    // alan（阿岚）配置 — 扩展预留
    // ========================================================================
    {
        std::vector<StageConfig> cfgs;
        cfgs.push_back({LifeStage::Stage0, 0.0f, {}, {}, {}, "mountain_hermit", "山顶隐士"});
        cfgs.push_back({LifeStage::Stage1, GC::Stage1Threshold, {"exploration_count_5", "affinity_4"}, {}, {}, "downtown_rental", "记录者"});
        cfgs.push_back({LifeStage::Stage2, GC::Stage2Threshold, {"spirit_value_50", "affinity_6"}, {}, {}, "cloud_tower", "云海预言师"});
        cfgs.push_back({LifeStage::Stage3, GC::Stage3Threshold, {"affinity_8", "spirit_value_75"}, {}, {}, "scholar_room", "云海录研究者"});
        configs["alan"] = std::move(cfgs);
    }

    return configs;
}

std::vector<StageConfig> DynamicLifeSystem::BuildProfileConfig_(const std::string& profile_id,
                                                                const std::string& npc_id) const {
    const auto defaults = CreateDefaultConfig_();
    if (const auto direct_it = defaults.find(npc_id); direct_it != defaults.end()) {
        return direct_it->second;
    }

    if (profile_id == "life_lin") {
        return {
            {LifeStage::Stage0, 0.0f, {}, {}, {}, "tea_field", "茶园学徒"},
            {LifeStage::Stage1, GC::Stage1Threshold, {"tea_quality_2", "affinity_4"}, {}, {}, "tea_workshop", "制茶师"},
            {LifeStage::Stage2, GC::Stage2Threshold, {"pact_tea_done", "affinity_6"}, {}, {}, "tea_partner", "茶园合伙人"},
            {LifeStage::Stage3, GC::Stage3Threshold, {"spirit_value_80", "tea_master"}, {}, {}, "tea_master", "茶文化传承人"},
        };
    }
    if (profile_id == "life_acha") {
        return {
            {LifeStage::Stage0, 0.0f, {}, {}, {}, "guest_room", "借住旅人"},
            {LifeStage::Stage1, GC::Stage1Threshold, {"inn_orders_3", "affinity_4"}, {}, {}, "inn_lobby", "客栈帮手"},
            {LifeStage::Stage2, GC::Stage2Threshold, {"festival_support_2", "affinity_6"}, {}, {}, "inn_host", "旅店主理"},
            {LifeStage::Stage3, GC::Stage3Threshold, {"relationship_steady", "affinity_8"}, {}, {}, "manor_host", "山庄迎宾人"},
        };
    }
    if (profile_id == "life_xiaoman") {
        return {
            {LifeStage::Stage0, 0.0f, {}, {}, {}, "market_stall", "跑腿学徒"},
            {LifeStage::Stage1, GC::Stage1Threshold, {"trade_count_3", "affinity_4"}, {}, {}, "market_helper", "集市帮手"},
            {LifeStage::Stage2, GC::Stage2Threshold, {"festival_score_1", "affinity_6"}, {}, {}, "festival_vendor", "节庆商贩"},
            {LifeStage::Stage3, GC::Stage3Threshold, {"trade_master", "affinity_8"}, {}, {}, "market_lead", "山庄交易联络人"},
        };
    }
    if (profile_id == "life_wanxing") {
        return {
            {LifeStage::Stage0, 0.0f, {}, {}, {}, "observatory_roof", "观星学徒"},
            {LifeStage::Stage1, GC::Stage1Threshold, {"night_watch_3", "affinity_4"}, {}, {}, "observatory", "观星记录者"},
            {LifeStage::Stage2, GC::Stage2Threshold, {"spirit_value_60", "affinity_6"}, {}, {}, "star_archive", "星历撰写人"},
            {LifeStage::Stage3, GC::Stage3Threshold, {"sky_ritual_done", "affinity_8"}, {}, {}, "cloud_oracle", "云海观测师"},
        };
    }

    return {
        {LifeStage::Stage0, 0.0f, {}, {}, {}, "npc_stage0", npc_id + " 初遇期"},
        {LifeStage::Stage1, GC::Stage1Threshold, {"affinity_4"}, {}, {}, "npc_stage1", npc_id + " 立足期"},
        {LifeStage::Stage2, GC::Stage2Threshold, {"affinity_6"}, {}, {}, "npc_stage2", npc_id + " 成长期"},
        {LifeStage::Stage3, GC::Stage3Threshold, {"affinity_8"}, {}, {}, "npc_stage3", npc_id + " 归属期"},
    };
}

}  // namespace CloudSeamanor::domain
