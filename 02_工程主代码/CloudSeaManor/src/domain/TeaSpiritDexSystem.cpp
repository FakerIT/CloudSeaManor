#include "CloudSeamanor/domain/TeaSpiritDexSystem.hpp"

#include <algorithm>
#include <sstream>

namespace CloudSeamanor::domain {

TeaSpiritDexSystem::TeaSpiritDexSystem() {
    spirits_.reserve(20);
    unlock_rules_.reserve(20);
    rewards_.reserve(5);
}

void TeaSpiritDexSystem::Initialize() {
    // 初始化时构建索引
    spirit_index_.clear();
    for (std::size_t i = 0; i < spirits_.size(); ++i) {
        spirit_index_[spirits_[i].tea_spirit_id] = i;
    }
}

void TeaSpiritDexSystem::LoadSpirits(const std::vector<TeaSpiritEntry>& spirits) {
    spirits_ = spirits;
    Initialize();
}

void TeaSpiritDexSystem::LoadUnlockRules(const std::vector<TeaSpiritUnlockRule>& rules) {
    unlock_rules_ = rules;
}

void TeaSpiritDexSystem::LoadRewards(const std::vector<TeaSpiritReward>& rewards) {
    rewards_ = rewards;
}

bool TeaSpiritDexSystem::TryUnlock(
    const std::string& tea_id,
    const std::string& quality,
    const std::string& season,
    int time_hour,
    const std::string& weather,
    const std::string& location_id,
    const std::string& extra_condition,
    int current_day
) {
    const auto* rule = FindMatchingRule_(tea_id);
    if (!rule) {
        return false;
    }

    // 检查是否已解锁
    const auto it = spirit_index_.find(rule->tea_spirit_id);
    if (it != spirit_index_.end()) {
        if (spirits_[it->second].unlocked) {
            return false;  // 已解锁
        }
    }

    // 检查规则条件
    if (!CheckRuleConditions_(*rule, quality, season, time_hour, weather, location_id, extra_condition)) {
        return false;
    }

    // 解锁茶灵
    if (it != spirit_index_.end()) {
        spirits_[it->second].unlocked = true;
        spirits_[it->second].first_unlock_day = current_day;
        return true;
    }

    return false;
}

const TeaSpiritUnlockRule* TeaSpiritDexSystem::FindMatchingRule_(const std::string& tea_id) const {
    for (const auto& rule : unlock_rules_) {
        if (rule.tea_id == tea_id) {
            return &rule;
        }
    }
    return nullptr;
}

bool TeaSpiritDexSystem::CheckRuleConditions_(
    const TeaSpiritUnlockRule& rule,
    const std::string& quality,
    const std::string& season,
    int time_hour,
    const std::string& weather,
    const std::string& location_id,
    const std::string& extra_condition
) const {
    // 品质等级检查：Holy > Rare > Fine > Normal
    // 如果规则要求 Holy，则只有 Holy 能通过
    // 如果规则要求 Rare，则 Rare 或更高能通过（但不能是 Holy 除非规则允许）
    // 简化为：检查是否达到所需品质等级
    if (rule.required_quality == "Holy") {
        if (quality != "Holy") {
            return false;
        }
    } else if (rule.required_quality == "Rare") {
        // Rare 或 Holy 都能通过
        if (quality != "Rare" && quality != "Holy") {
            return false;
        }
    } else if (rule.required_quality == "Fine") {
        // Fine 或更高品质能通过
        if (quality != "Fine" && quality != "Rare" && quality != "Holy") {
            return false;
        }
    } else if (rule.required_quality == "Normal") {
        // 任何品质都能通过
    }

    // 检查季节
    if (!rule.season.empty() && rule.season != season) {
        return false;
    }

    // 检查时辰
    if (!rule.time_range.empty()) {
        // 简单解析：格式为 "6-12" 表示 6 点到 12 点
        size_t dash_pos = rule.time_range.find('-');
        if (dash_pos != std::string::npos) {
            int start_hour = std::stoi(rule.time_range.substr(0, dash_pos));
            int end_hour = std::stoi(rule.time_range.substr(dash_pos + 1));
            if (time_hour < start_hour || time_hour > end_hour) {
                return false;
            }
        }
    }

    // 检查天气（支持部分匹配）
    if (!rule.weather_limit.empty()) {
        // 规则中的天气限制可能包含多个选项（用 | 分隔）
        bool weather_match = false;
        std::string rule_weather = rule.weather_limit;
        size_t pipe_pos = rule_weather.find('|');
        if (pipe_pos != std::string::npos) {
            // 多个天气选项
            std::stringstream ss(rule_weather);
            std::string opt;
            while (std::getline(ss, opt, '|')) {
                if (weather.find(opt) != std::string::npos || opt == weather) {
                    weather_match = true;
                    break;
                }
            }
        } else {
            weather_match = (weather.find(rule_weather) != std::string::npos || rule_weather == weather);
        }
        
        // 特殊处理 "Night" 天气
        if (rule_weather == "Night" && (time_hour >= 21 || time_hour < 5)) {
            weather_match = true;
        }
        
        if (!weather_match) {
            return false;
        }
    }

    // 检查场景
    if (!rule.location_id.empty() && rule.location_id != location_id) {
        return false;
    }

    // 检查额外条件
    if (!rule.extra_condition.empty() && rule.extra_condition != extra_condition) {
        return false;
    }

    return true;
}

int TeaSpiritDexSystem::GetUnlockedCount() const {
    int count = 0;
    for (const auto& spirit : spirits_) {
        if (spirit.unlocked) {
            ++count;
        }
    }
    return count;
}

int TeaSpiritDexSystem::GetTotalCount() const {
    return static_cast<int>(spirits_.size());
}

float TeaSpiritDexSystem::GetCompletionRatio() const {
    if (spirits_.empty()) {
        return 0.0f;
    }
    return static_cast<float>(GetUnlockedCount()) / static_cast<float>(spirits_.size());
}

const TeaSpiritEntry* TeaSpiritDexSystem::GetSpirit(const std::string& spirit_id) const {
    const auto it = spirit_index_.find(spirit_id);
    if (it != spirit_index_.end()) {
        return &spirits_[it->second];
    }
    return nullptr;
}

std::vector<const TeaSpiritEntry*> TeaSpiritDexSystem::GetUnlockedSpirits() const {
    std::vector<const TeaSpiritEntry*> result;
    for (const auto& spirit : spirits_) {
        if (spirit.unlocked) {
            result.push_back(&spirit);
        }
    }
    return result;
}

std::vector<const TeaSpiritEntry*> TeaSpiritDexSystem::GetAllSpirits() const {
    std::vector<const TeaSpiritEntry*> result;
    for (const auto& spirit : spirits_) {
        result.push_back(&spirit);
    }
    return result;
}

bool TeaSpiritDexSystem::IsSpiritUnlocked(const std::string& spirit_id) const {
    const auto* spirit = GetSpirit(spirit_id);
    return spirit && spirit->unlocked;
}

std::vector<const TeaSpiritReward*> TeaSpiritDexSystem::GetPendingRewards() const {
    std::vector<const TeaSpiritReward*> result;
    const int unlocked = GetUnlockedCount();

    for (const auto& reward : rewards_) {
        if (!reward.claimed && unlocked >= reward.milestone_count) {
            result.push_back(&reward);
        }
    }
    return result;
}

void TeaSpiritDexSystem::ClaimReward(const std::string& reward_id) {
    for (auto& reward : rewards_) {
        if (reward.reward_id == reward_id) {
            reward.claimed = true;
            return;
        }
    }
}

void TeaSpiritDexSystem::SetSpiritDisplay(const std::string& spirit_id, bool enabled) {
    const auto it = spirit_index_.find(spirit_id);
    if (it != spirit_index_.end()) {
        spirits_[it->second].display_enabled = enabled;
    }
}

std::vector<const TeaSpiritEntry*> TeaSpiritDexSystem::GetDisplayableSpirits() const {
    std::vector<const TeaSpiritEntry*> result;
    for (const auto& spirit : spirits_) {
        if (spirit.unlocked && spirit.display_enabled) {
            result.push_back(&spirit);
        }
    }
    return result;
}

std::string TeaSpiritDexSystem::Serialize() const {
    std::ostringstream oss;

    // 序列化茶灵状态
    for (const auto& spirit : spirits_) {
        oss << spirit.tea_spirit_id << "|"
            << (spirit.unlocked ? "1" : "0") << "|"
            << spirit.first_unlock_day << "|"
            << (spirit.display_enabled ? "1" : "0") << "\n";
    }

    // 序列化奖励领取状态
    for (const auto& reward : rewards_) {
        if (reward.claimed) {
            oss << "reward|" << reward.reward_id << "|1\n";
        }
    }

    // 序列化大师奖励
    if (master_reward_claimed_) {
        oss << "master_reward|1\n";
    }

    return oss.str();
}

void TeaSpiritDexSystem::Deserialize(const std::string& data) {
    if (data.empty()) {
        return;  // 旧存档无数据
    }

    std::istringstream iss(data);
    std::string line;

    while (std::getline(iss, line)) {
        if (line.empty()) continue;

        if (line.rfind("reward|", 0) == 0) {
            // 奖励领取状态
            std::istringstream line_ss(line);
            std::string prefix, reward_id, claimed;
            if (std::getline(line_ss, prefix, '|') &&
                std::getline(line_ss, reward_id, '|') &&
                std::getline(line_ss, claimed, '|')) {
                for (auto& reward : rewards_) {
                    if (reward.reward_id == reward_id) {
                        reward.claimed = (claimed == "1");
                        break;
                    }
                }
            }
        } else if (line.rfind("master_reward|", 0) == 0) {
            master_reward_claimed_ = true;
        } else {
            // 茶灵状态
            std::istringstream line_ss(line);
            std::string spirit_id, unlocked_str, day_str, display_str;

            if (std::getline(line_ss, spirit_id, '|') &&
                std::getline(line_ss, unlocked_str, '|') &&
                std::getline(line_ss, day_str, '|') &&
                std::getline(line_ss, display_str, '|')) {

                const auto it = spirit_index_.find(spirit_id);
                if (it != spirit_index_.end()) {
                    spirits_[it->second].unlocked = (unlocked_str == "1");
                    spirits_[it->second].first_unlock_day = std::stoi(day_str);
                    spirits_[it->second].display_enabled = (display_str == "1");
                }
            }
        }
    }
}

}  // namespace CloudSeamanor::domain
