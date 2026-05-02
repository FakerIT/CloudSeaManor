#include "CloudSeamanor/engine/systems/NpcDeliverySystem.hpp"

#include "CloudSeamanor/app/GameAppText.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <random>
#include <sstream>
#include <stdexcept>

namespace CloudSeamanor::engine {

namespace {

int ParsePositiveInt_(const std::string& s, int fallback) {
    try {
        return std::max(0, std::stoi(s));
    } catch (const std::invalid_argument&) {
        return fallback;
    } catch (const std::out_of_range&) {
        return fallback;
    }
}

std::mt19937& Rng_() {
    thread_local std::mt19937 rng(std::random_device{}());
    return rng;
}

} // namespace

bool NpcDeliverySystem::LoadFromCsv(const std::string& path) {
    pool_.clear();

    std::ifstream in(path);
    if (!in.is_open()) {
        CloudSeamanor::infrastructure::Logger::LogConfigLoadFailure(
            std::string("NpcDelivery CSV open failed: ") + path);
        return false;
    }

    std::string line;
    if (!std::getline(in, line)) {
        CloudSeamanor::infrastructure::Logger::LogConfigLoadFailure(
            std::string("NpcDelivery CSV header read failed: ") + path);
        return false;
    }

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream ss(line);
        std::string f;
        QuestDelivery q;
        int col = 0;
        while (std::getline(ss, f, ',')) {
            switch (col) {
            case 0: q.npc_id = f; break;
            case 1: q.quest_id = f; break;
            case 2: q.title = f; break;
            case 3: q.description = f; break;
            case 4: q.item_id = f; break;
            case 5: q.item_count = ParsePositiveInt_(f, 0); break;
            case 6: q.reward_gold = ParsePositiveInt_(f, 0); break;
            case 7: q.reward_favor = ParsePositiveInt_(f, 0); break;
            default: break;
            }
            ++col;
        }
        if (!q.npc_id.empty() && !q.quest_id.empty() && !q.item_id.empty() && q.item_count > 0) {
            pool_.push_back(std::move(q));
        }
    }
    if (pool_.empty()) {
        CloudSeamanor::infrastructure::Logger::Warning(
            std::string("NpcDeliverySystem: no valid rows loaded from ") + path);
        return false;
    }
    CloudSeamanor::infrastructure::Logger::Info(
        std::string("NpcDeliverySystem: loaded rows = ") + std::to_string(pool_.size()));
    return true;
}

void NpcDeliverySystem::Update(GameWorldState& world_state, int current_day, int current_hour) {
    if (current_hour >= 6 && current_day != last_refresh_day_) {
        RefreshDaily_(world_state.MutableRuntimeQuests(), current_day);
        last_refresh_day_ = current_day;
    }

    EvaluateCompletion_(world_state.MutableRuntimeQuests(), world_state.GetInventory());
}

bool NpcDeliverySystem::TryClaimRewards(
    GameWorldState& world_state,
    const std::string& npc_id
) {
    bool claimed_any = false;
    auto& quests = world_state.MutableRuntimeQuests();
    auto& inventory = world_state.MutableInventory();
    auto& gold = world_state.MutableGold();

    for (auto& q : quests) {
        if (q.state != QuestState::Completed) {
            continue;
        }
        const auto parsed = ParseQuestId_(q.id);
        if (!parsed) {
            continue;
        }
        if (parsed->npc_id != npc_id) {
            continue;
        }
        if (inventory.CountOf(parsed->item_id) < parsed->item_count) {
            continue; // 避免完成后被玩家消耗导致负奖励
        }

        (void)inventory.TryRemoveItem(parsed->item_id, parsed->item_count);
        if (parsed->reward_gold > 0) {
            gold += parsed->reward_gold;
        }

        // 好感奖励：直接加到目标 NPC（避免引入额外依赖链）
        for (auto& npc : world_state.MutableNpcs()) {
            if (npc.id == npc_id) {
                npc.favor += parsed->reward_favor;
                npc.heart_level = NpcHeartLevelFromFavor(npc.favor);
                break;
            }
        }

        q.state = QuestState::Claimed;
        claimed_any = true;

        if (hint_callback_) {
            std::string msg = "领取委托奖励：金币 +" + std::to_string(parsed->reward_gold);
            if (parsed->reward_favor > 0) {
                msg += "，好感 +" + std::to_string(parsed->reward_favor);
            }
            hint_callback_(msg, 2.6f);
        }
    }

    return claimed_any;
}

void NpcDeliverySystem::RefreshDaily_(
    std::vector<RuntimeQuest>& quests,
    int day
) {
    // 清理旧的已领取委托，避免列表无上限增长
    for (std::size_t i = 0; i < quests.size();) {
        if (quests[i].id.rfind("npc_delivery|", 0) == 0 && quests[i].state == QuestState::Claimed) {
            quests[i] = quests.back();
            quests.pop_back();
            continue;
        }
        ++i;
    }

    if (pool_.empty()) {
        return;
    }

    std::uniform_int_distribution<int> count_dist(1, 3);
    const int generate_count = count_dist(Rng_());

    std::uniform_int_distribution<std::size_t> pick_dist(0, pool_.size() - 1);
    for (int i = 0; i < generate_count; ++i) {
        const QuestDelivery& def = pool_[pick_dist(Rng_())];
        RuntimeQuest quest;
        quest.id = BuildQuestId_(def.npc_id, def.item_id, def.item_count, def.reward_gold, def.reward_favor);
        quest.title = def.title.empty() ? ("委托：" + def.item_id) : def.title;
        quest.description = def.description.empty() ? "完成 NPC 委托后回去领取奖励。" : def.description;
        quest.objective = "交付 " + def.item_id + " x" + std::to_string(def.item_count) + "（交付给 " + def.npc_id + "）";
        quest.reward = "金币 " + std::to_string(def.reward_gold) + "，好感 +" + std::to_string(def.reward_favor);
        quest.auto_accept = true;
        quest.state = QuestState::InProgress;
        quests.push_back(std::move(quest));
    }

    if (hint_callback_) {
        hint_callback_("今日 NPC 委托已刷新（6:00）。", 2.4f);
    }
    CloudSeamanor::infrastructure::Logger::Info(
        "NpcDeliverySystem: refreshed daily quests for day " + std::to_string(day));
}

void NpcDeliverySystem::EvaluateCompletion_(
    std::vector<RuntimeQuest>& quests,
    const CloudSeamanor::domain::Inventory& inventory
) {
    for (auto& q : quests) {
        if (q.state != QuestState::InProgress) {
            continue;
        }
        if (q.id.rfind("npc_delivery|", 0) != 0) {
            continue;
        }
        const auto parsed = ParseQuestId_(q.id);
        if (!parsed) {
            continue;
        }
        if (inventory.CountOf(parsed->item_id) >= parsed->item_count) {
            q.state = QuestState::Completed;
            if (hint_callback_) {
                hint_callback_("委托已完成，去找对应 NPC 领取奖励。", 2.4f);
            }
        }
    }
}

std::optional<QuestDelivery> NpcDeliverySystem::ParseQuestId_(const std::string& quest_id) {
    // npc_delivery|npc_id|item_id|count|gold|favor
    constexpr const char* kPrefix = "npc_delivery|";
    if (quest_id.rfind(kPrefix, 0) != 0) {
        return std::nullopt;
    }
    QuestDelivery q;
    std::string rest = quest_id.substr(std::char_traits<char>::length(kPrefix));
    std::istringstream ss(rest);
    std::string f;
    int col = 0;
    while (std::getline(ss, f, '|')) {
        switch (col) {
        case 0: q.npc_id = f; break;
        case 1: q.item_id = f; break;
        case 2: q.item_count = ParsePositiveInt_(f, 0); break;
        case 3: q.reward_gold = ParsePositiveInt_(f, 0); break;
        case 4: q.reward_favor = ParsePositiveInt_(f, 0); break;
        default: break;
        }
        ++col;
    }
    if (q.npc_id.empty() || q.item_id.empty() || q.item_count <= 0) {
        return std::nullopt;
    }
    return q;
}

std::string NpcDeliverySystem::BuildQuestId_(
    const std::string& npc_id,
    const std::string& item_id,
    int item_count,
    int reward_gold,
    int reward_favor
) {
    return "npc_delivery|" + npc_id + "|" + item_id + "|"
        + std::to_string(item_count) + "|" + std::to_string(reward_gold) + "|"
        + std::to_string(reward_favor);
}

} // namespace CloudSeamanor::engine

