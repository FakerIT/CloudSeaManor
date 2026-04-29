#include "CloudSeamanor/domain/NpcDevelopmentSystem.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace CloudSeamanor::domain {

void NpcDevelopmentSystem::Initialize(const std::vector<std::string>& npc_ids, int current_day) {
    const int safe_day = std::max(1, current_day);
    for (const auto& npc_id : npc_ids) {
        if (npc_id.empty()) {
            continue;
        }
        if (developments_.find(npc_id) != developments_.end()) {
            continue;
        }
        NPCDynamicDevelopment data;
        data.npc_id = npc_id;
        data.current_stage = 0;
        data.current_job = "stage0";
        data.current_house_id = "default_house";
        data.current_room_location = "default_room";
        data.appearance_variant = "default";
        data.first_day_in_stage = safe_day;
        data.last_stage_change_day = safe_day;
        developments_[npc_id] = std::move(data);
    }
}

bool NpcDevelopmentSystem::LoadStageRules(const std::string& csv_path) {
    stage_rules_.clear();
    std::ifstream in(csv_path);
    if (!in.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        if (line.rfind("NpcId,", 0) == 0) {
            continue;
        }
        const auto fields = SplitCsvLine_(line);
        if (fields.size() < 11) {
            continue;
        }

        NpcDevelopmentStageRule rule;
        try {
            rule.stage = std::stoi(fields[1]);
            rule.requires_player_action = (std::stoi(fields[3]) != 0);
            rule.world_timer_years = std::max(0, std::stoi(fields[4]));
        } catch (...) {
            continue;
        }
        rule.player_condition = fields[5];
        rule.next_job = fields[6];
        rule.next_house_id = fields[7];
        rule.next_room_location = fields[8];
        rule.appearance_variant = fields[9];
        rule.branch_group = fields[10];
        if (fields.size() >= 12) {
            rule.min_days_since_last_stage = std::max(0, std::atoi(fields[11].c_str()));
        }
        stage_rules_[fields[0]].push_back(std::move(rule));
    }

    for (auto& [_, rules] : stage_rules_) {
        std::sort(rules.begin(), rules.end(), [](const NpcDevelopmentStageRule& a, const NpcDevelopmentStageRule& b) {
            return a.stage < b.stage;
        });
    }
    return !stage_rules_.empty();
}

bool NpcDevelopmentSystem::LoadBranchRules(const std::string& csv_path) {
    branch_rules_.clear();
    std::ifstream in(csv_path);
    if (!in.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        if (line.rfind("NpcId,", 0) == 0) {
            continue;
        }
        const auto fields = SplitCsvLine_(line);
        if (fields.size() < 9 || fields[0].empty()) {
            continue;
        }
        NpcDevelopmentBranchRule rule;
        rule.branch_id = fields[1];
        rule.trigger_condition = fields[2];
        rule.result_job = fields[3];
        rule.result_shop_type = fields[4];
        rule.result_dialogue_tag = fields[5];
        rule.player_school_tag = fields[6];
        rule.player_action_hint = fields[7];
        rule.fallback_branch_id = fields[8];
        branch_rules_[fields[0]].push_back(std::move(rule));
    }
    return !branch_rules_.empty();
}

void NpcDevelopmentSystem::RecordPlayerAction(const std::string& npc_id, const std::string& action_tag) {
    if (npc_id.empty() || action_tag.empty()) {
        return;
    }
    action_flags_[npc_id][action_tag] = true;
}

bool NpcDevelopmentSystem::TryAdvanceByDay(const std::string& npc_id, int current_day) {
    auto* development = GetDevelopment(npc_id);
    if (development == nullptr) {
        return false;
    }
    const auto it = stage_rules_.find(npc_id);
    if (it == stage_rules_.end() || it->second.empty()) {
        return false;
    }

    const int current_stage = development->current_stage;
    const int next_stage = current_stage + 1;
    const auto rule_it = std::find_if(it->second.begin(), it->second.end(), [next_stage](const NpcDevelopmentStageRule& r) {
        return r.stage == next_stage;
    });
    if (rule_it == it->second.end()) {
        return false;
    }
    const auto& rule = *rule_it;
    const int years_elapsed = YearFromDay_(current_day) - YearFromDay_(development->first_day_in_stage);
    const bool world_timer_ready = years_elapsed >= rule.world_timer_years;
    const bool player_action_ready = !rule.requires_player_action || IsConditionSatisfied_(npc_id, rule.player_condition);
    const bool accelerated_by_player = player_action_ready && !world_timer_ready;
    const int required_min_days = std::max(
        rule.min_days_since_last_stage,
        accelerated_by_player ? kMinPlayerDrivenAdvanceDays : 0);
    const bool min_days_ready = (current_day - development->last_stage_change_day) >= required_min_days;
    if ((!world_timer_ready && !player_action_ready) || !min_days_ready) {
        return false;
    }

    development->current_stage = next_stage;
    development->current_job = rule.next_job.empty() ? development->current_job : rule.next_job;
    development->current_house_id = rule.next_house_id.empty() ? development->current_house_id : rule.next_house_id;
    development->current_room_location =
        rule.next_room_location.empty() ? development->current_room_location : rule.next_room_location;
    development->appearance_variant =
        rule.appearance_variant.empty() ? development->appearance_variant : rule.appearance_variant;
    development->stage_conditions = {rule.player_condition, "world_timer_years=" + std::to_string(rule.world_timer_years)};
    if (const auto* branch = ResolveBranchForNpc_(npc_id)) {
        if (!branch->result_job.empty()) {
            development->current_job = branch->result_job;
        }
        development->stage_conditions.push_back("branch=" + branch->branch_id);
        if (!branch->player_school_tag.empty()) {
            development->stage_conditions.push_back("school=" + branch->player_school_tag);
        }
        if (!branch->player_action_hint.empty()) {
            development->stage_conditions.push_back("action_hint=" + branch->player_action_hint);
        }
    }
    development->first_day_in_stage = current_day;
    development->last_stage_change_day = current_day;
    return true;
}

int NpcDevelopmentSystem::GetPreparingDaysRemaining(const std::string& npc_id, int current_day) const {
    const auto* development = GetDevelopment(npc_id);
    if (development == nullptr) {
        return -1;
    }
    const auto it = stage_rules_.find(npc_id);
    if (it == stage_rules_.end() || it->second.empty()) {
        return -1;
    }

    const int next_stage = development->current_stage + 1;
    const auto rule_it = std::find_if(it->second.begin(), it->second.end(), [next_stage](const NpcDevelopmentStageRule& r) {
        return r.stage == next_stage;
    });
    if (rule_it == it->second.end()) {
        return -1;
    }

    const auto& rule = *rule_it;
    const int years_elapsed = YearFromDay_(current_day) - YearFromDay_(development->first_day_in_stage);
    const bool world_timer_ready = years_elapsed >= rule.world_timer_years;
    const bool player_action_ready = !rule.requires_player_action || IsConditionSatisfied_(npc_id, rule.player_condition);
    if (!world_timer_ready && !player_action_ready) {
        return -1;
    }

    const bool accelerated_by_player = player_action_ready && !world_timer_ready;
    const int required_min_days = std::max(
        rule.min_days_since_last_stage,
        accelerated_by_player ? kMinPlayerDrivenAdvanceDays : 0);
    const int days_since_last = current_day - development->last_stage_change_day;
    if (days_since_last >= required_min_days) {
        return 0;
    }
    return required_min_days - days_since_last;
}

const NPCDynamicDevelopment* NpcDevelopmentSystem::GetDevelopment(const std::string& npc_id) const {
    const auto it = developments_.find(npc_id);
    return it == developments_.end() ? nullptr : &it->second;
}

NPCDynamicDevelopment* NpcDevelopmentSystem::GetDevelopment(const std::string& npc_id) {
    const auto it = developments_.find(npc_id);
    return it == developments_.end() ? nullptr : &it->second;
}

int NpcDevelopmentSystem::YearFromDay_(int day) {
    return std::max(0, (std::max(1, day) - 1) / 112);
}

std::vector<std::string> NpcDevelopmentSystem::SplitCsvLine_(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string part;
    while (std::getline(ss, part, ',')) {
        fields.push_back(part);
    }
    return fields;
}

bool NpcDevelopmentSystem::IsConditionSatisfied_(const std::string& npc_id, const std::string& condition) const {
    if (condition.empty()) {
        return true;
    }
    const auto it = action_flags_.find(npc_id);
    if (it == action_flags_.end()) {
        return false;
    }
    const auto cond_it = it->second.find(condition);
    return cond_it != it->second.end() && cond_it->second;
}

const NpcDevelopmentBranchRule* NpcDevelopmentSystem::ResolveBranchForNpc_(const std::string& npc_id) const {
    const auto it = branch_rules_.find(npc_id);
    if (it == branch_rules_.end() || it->second.empty()) {
        return nullptr;
    }
    const auto& rules = it->second;

    for (const auto& rule : rules) {
        if (!rule.player_action_hint.empty() && IsConditionSatisfied_(npc_id, rule.player_action_hint)) {
            return &rule;
        }
    }
    for (const auto& rule : rules) {
        if (!rule.trigger_condition.empty() && IsConditionSatisfied_(npc_id, rule.trigger_condition)) {
            return &rule;
        }
    }
    for (const auto& rule : rules) {
        if (!rule.fallback_branch_id.empty()) {
            if (const auto* fallback = FindBranchById_(rules, rule.fallback_branch_id)) {
                return fallback;
            }
        }
    }
    return &rules.front();
}

const NpcDevelopmentBranchRule* NpcDevelopmentSystem::FindBranchById_(
    const std::vector<NpcDevelopmentBranchRule>& rules,
    const std::string& branch_id) const {
    if (branch_id.empty()) {
        return nullptr;
    }
    const auto it = std::find_if(rules.begin(), rules.end(), [&branch_id](const NpcDevelopmentBranchRule& r) {
        return r.branch_id == branch_id;
    });
    return it == rules.end() ? nullptr : &(*it);
}

}  // namespace CloudSeamanor::domain
