#include "CloudSeamanor/domain/PlayerMemorySystem.hpp"

#include <algorithm>
#include <sstream>

namespace CloudSeamanor::domain {

PlayerMemorySystem::PlayerMemorySystem() {
    memories_.reserve(kMaxMemories);
}

void PlayerMemorySystem::AddMemory(
    PlayerMemoryType type,
    const std::string& subject_id,
    int game_day,
    int weight,
    int decay_days
) {
    if (type == PlayerMemoryType::None) {
        return;
    }

    // 尝试合并相同类型记忆
    MergeMemories_(type, subject_id, game_day, weight);

    // 如果没有合并（新增），添加到列表
    bool merged = false;
    for (auto& mem : memories_) {
        if (mem.type == type && mem.subject_id == subject_id) {
            mem.weight += weight;
            mem.game_day = game_day;  // 更新为最新日期
            merged = true;
            break;
        }
    }

    if (!merged) {
        PlayerMemoryRecord record;
        record.type = type;
        record.subject_id = subject_id;
        record.game_day = game_day;
        record.weight = weight;
        record.decay_days = decay_days;
        memories_.push_back(record);

        // 超过最大数量时移除最旧的
        if (static_cast<int>(memories_.size()) > kMaxMemories) {
            int oldest_day = memories_[0].game_day;
            std::size_t oldest_idx = 0;
            for (std::size_t i = 1; i < memories_.size(); ++i) {
                if (memories_[i].game_day < oldest_day) {
                    oldest_day = memories_[i].game_day;
                    oldest_idx = i;
                }
            }
            memories_.erase(memories_.begin() + oldest_idx);
        }
    }
}

void PlayerMemorySystem::OnDayChanged(int current_day) {
    RemoveExpired_(current_day);

    // 重置每日对话消费标记
    for (auto& mem : memories_) {
        mem.consumed_by_dialogue = false;
    }
}

void PlayerMemorySystem::RemoveExpired_(int current_day) {
    memories_.erase(
        std::remove_if(memories_.begin(), memories_.end(),
            [current_day](const PlayerMemoryRecord& mem) {
                return (current_day - mem.game_day) > mem.decay_days;
            }),
        memories_.end()
    );
}

void PlayerMemorySystem::MergeMemories_(
    PlayerMemoryType type,
    const std::string& subject_id,
    int game_day,
    int weight
) {
    for (auto& mem : memories_) {
        if (mem.type == type && mem.subject_id == subject_id) {
            mem.weight += weight;
            mem.game_day = game_day;
            return;
        }
    }
}

std::vector<const PlayerMemoryRecord*> PlayerMemorySystem::GetHotMemories(
    int current_day,
    int max_days,
    int max_count
) const {
    std::vector<const PlayerMemoryRecord*> result;

    for (const auto& mem : memories_) {
        const int age = current_day - mem.game_day;
        if (age <= max_days && !mem.consumed_by_dialogue) {
            result.push_back(&mem);
        }
    }

    // 按权重和新鲜度排序
    std::sort(result.begin(), result.end(),
        [current_day](const PlayerMemoryRecord* a, const PlayerMemoryRecord* b) {
            const int age_a = current_day - a->game_day;
            const int age_b = current_day - b->game_day;
            const int score_a = a->weight * 100 - age_a * 10;
            const int score_b = b->weight * 100 - age_b * 10;
            return score_a > score_b;
        });

    if (static_cast<int>(result.size()) > max_count) {
        result.resize(max_count);
    }

    return result;
}

std::string PlayerMemorySystem::GetDialogueHook(
    const std::string& npc_id,
    int npc_heart_level,
    int current_day,
    const std::string& season,
    const std::string& weather
) const {
    const auto* hook = FindMatchingHook_(npc_id, npc_heart_level, current_day, season, weather);
    if (hook != nullptr) {
        return hook->text;
    }
    return {};
}

const MemoryHook* PlayerMemorySystem::FindMatchingHook_(
    const std::string& npc_id,
    int npc_heart_level,
    int current_day,
    const std::string& season,
    const std::string& weather
) const {
    const MemoryHook* best = nullptr;
    int best_priority = -1;

    for (const auto& hook : hooks_) {
        // 检查 NPC
        if (hook.speaker_id != npc_id) {
            continue;
        }

        // 检查好感等级
        if (npc_heart_level < hook.min_heart_level) {
            continue;
        }

        // 检查季节限制
        if (!hook.season_limit.empty() && hook.season_limit != season) {
            continue;
        }

        // 检查天气限制
        if (!hook.weather_limit.empty() && hook.weather_limit != weather) {
            continue;
        }

        // 查找匹配的记忆
        bool has_matching_memory = false;
        for (const auto& mem : memories_) {
            if (mem.type == hook.memory_type) {
                const int age = current_day - mem.game_day;
                if (age <= hook.max_days_since_memory) {
                    has_matching_memory = true;
                    break;
                }
            }
        }

        if (!has_matching_memory) {
            continue;
        }

        // 选择优先级最高的
        if (hook.priority > best_priority) {
            best_priority = hook.priority;
            best = &hook;
        }
    }

    return best;
}

std::string PlayerMemorySystem::GetPendingMailId(int current_day) {
    for (const auto& mem : memories_) {
        if (mem.consumed_by_mail) {
            continue;
        }

        for (const auto& tpl : mail_templates_) {
            if (tpl.memory_type != mem.type) {
                continue;
            }

            // 检查冷却
            const auto it = mail_cooldowns_.find(tpl.id);
            if (it != mail_cooldowns_.end()) {
                if ((current_day - it->second) < tpl.cooldown_days) {
                    continue;
                }
            }

            // 检查权重
            if (mem.weight < tpl.min_weight) {
                continue;
            }

            // 找到合适的邮件
            return tpl.id;
        }
    }
    return {};
}

void PlayerMemorySystem::MarkMailConsumed(const std::string& memory_id) {
    for (auto& mem : memories_) {
        if (mem.type == PlayerMemoryType::None) {
            continue;
        }
        mem.consumed_by_mail = true;
        break;
    }
}

const MemoryMailTemplate* PlayerMemorySystem::GetMailTemplateById(const std::string& id) const {
    for (const auto& tpl : mail_templates_) {
        if (tpl.id == id) {
            return &tpl;
        }
    }
    return nullptr;
}

void PlayerMemorySystem::MarkDialogueConsumed(int memory_index) {
    if (memory_index >= 0 && memory_index < static_cast<int>(memories_.size())) {
        memories_[static_cast<std::size_t>(memory_index)].consumed_by_dialogue = true;
    }
}

void PlayerMemorySystem::LoadHooks(const std::vector<MemoryHook>& hooks) {
    hooks_ = hooks;
}

void PlayerMemorySystem::LoadMailTemplates(const std::vector<MemoryMailTemplate>& templates) {
    mail_templates_ = templates;
}

bool PlayerMemorySystem::HasRecentMemory(
    PlayerMemoryType type,
    int current_day,
    int max_days
) const {
    for (const auto& mem : memories_) {
        if (mem.type == type) {
            const int age = current_day - mem.game_day;
            if (age <= max_days) {
                return true;
            }
        }
    }
    return false;
}

std::string PlayerMemorySystem::Serialize() const {
    std::ostringstream oss;
    for (const auto& mem : memories_) {
        oss << ToString(mem.type) << "|"
            << mem.subject_id << "|"
            << mem.game_day << "|"
            << mem.weight << "|"
            << mem.decay_days << "|"
            << (mem.consumed_by_mail ? 1 : 0) << "|"
            << (mem.consumed_by_dialogue ? 1 : 0) << "\n";
    }
    return oss.str();
}

void PlayerMemorySystem::Deserialize(const std::string& data) {
    if (data.empty()) {
        return;  // 旧存档无数据
    }

    memories_.clear();
    std::istringstream iss(data);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;

        std::istringstream line_ss(line);
        std::string type_str, subject_id, game_day_str, weight_str, decay_str, mail_used_str, dialogue_used_str;

        if (!std::getline(line_ss, type_str, '|')) continue;
        if (!std::getline(line_ss, subject_id, '|')) continue;
        if (!std::getline(line_ss, game_day_str, '|')) continue;
        if (!std::getline(line_ss, weight_str, '|')) continue;
        if (!std::getline(line_ss, decay_str, '|')) continue;
        if (!std::getline(line_ss, mail_used_str, '|')) continue;
        if (!std::getline(line_ss, dialogue_used_str, '|')) continue;

        PlayerMemoryRecord mem;
        mem.type = ParseMemoryType(type_str);
        mem.subject_id = subject_id;
        mem.game_day = std::stoi(game_day_str);
        mem.weight = std::stoi(weight_str);
        mem.decay_days = std::stoi(decay_str);
        mem.consumed_by_mail = (mail_used_str == "1");
        mem.consumed_by_dialogue = (dialogue_used_str == "1");

        memories_.push_back(mem);
    }
}

}  // namespace CloudSeamanor::domain
