#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::domain {

struct NPCDynamicDevelopment {
    std::string npc_id;
    int current_stage = 0;
    std::string current_job;
    std::string current_house_id;
    std::string current_room_location;
    std::string appearance_variant;
    std::vector<std::string> stage_conditions;
    std::unordered_map<std::string, int> npc_relationships;
    int first_day_in_stage = 1;
    int last_stage_change_day = 1;
    int last_notified_stage = -1;
};

struct NpcDevelopmentStageRule {
    int stage = 0;
    bool requires_player_action = false;
    int world_timer_years = 1;
    std::string player_condition;
    std::string next_job;
    std::string next_house_id;
    std::string next_room_location;
    std::string appearance_variant;
    std::string branch_group;
    int min_days_since_last_stage = 0;
};

struct NpcDevelopmentBranchRule {
    std::string branch_id;
    std::string trigger_condition;
    std::string result_job;
    std::string result_shop_type;
    std::string result_dialogue_tag;
    std::string player_school_tag;
    std::string player_action_hint;
    std::string fallback_branch_id;
};

class NpcDevelopmentSystem {
public:
    void Initialize(const std::vector<std::string>& npc_ids, int current_day);
    bool LoadStageRules(const std::string& csv_path);
    bool LoadBranchRules(const std::string& csv_path);

    void RecordPlayerAction(const std::string& npc_id, const std::string& action_tag);
    bool TryAdvanceByDay(const std::string& npc_id, int current_day);
    [[nodiscard]] int GetPreparingDaysRemaining(const std::string& npc_id, int current_day) const;

    [[nodiscard]] const NPCDynamicDevelopment* GetDevelopment(const std::string& npc_id) const;
    [[nodiscard]] NPCDynamicDevelopment* GetDevelopment(const std::string& npc_id);
    [[nodiscard]] const std::unordered_map<std::string, NPCDynamicDevelopment>& GetAllDevelopments() const {
        return developments_;
    }
    std::unordered_map<std::string, NPCDynamicDevelopment>& MutableAllDevelopments() { return developments_; }

private:
    static constexpr int kMinPlayerDrivenAdvanceDays = 5;

    [[nodiscard]] static int YearFromDay_(int day);
    [[nodiscard]] static std::vector<std::string> SplitCsvLine_(const std::string& line);
    [[nodiscard]] bool IsConditionSatisfied_(const std::string& npc_id, const std::string& condition) const;
    [[nodiscard]] const NpcDevelopmentBranchRule* ResolveBranchForNpc_(const std::string& npc_id) const;
    [[nodiscard]] const NpcDevelopmentBranchRule* FindBranchById_(
        const std::vector<NpcDevelopmentBranchRule>& rules,
        const std::string& branch_id) const;

    std::unordered_map<std::string, NPCDynamicDevelopment> developments_;
    std::unordered_map<std::string, std::vector<NpcDevelopmentStageRule>> stage_rules_;
    std::unordered_map<std::string, std::vector<NpcDevelopmentBranchRule>> branch_rules_;
    std::unordered_map<std::string, std::unordered_map<std::string, bool>> action_flags_;
};

}  // namespace CloudSeamanor::domain
