#include "CloudSeamanor/engine/systems/QuestManager.hpp"

#include <cctype>
#include <fstream>
#include <sstream>

namespace CloudSeamanor::engine {

bool QuestManager::LoadFromCsv(const std::string& path, std::vector<RuntimeQuest>& out_quests) const {
    out_quests.clear();

    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    std::string line;
    if (!std::getline(in, line)) {
        return false;
    }

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream ss(line);
        std::string f;
        RuntimeQuest quest;
        int col = 0;
        while (std::getline(ss, f, ',')) {
            switch (col) {
            case 0: quest.id = f; break;
            case 1: quest.title = f; break;
            case 2: quest.description = f; break;
            case 3: quest.objective = f; break;
            case 4: quest.reward = f; break;
            default: break;
            }
            ++col;
        }
        quest.auto_accept = quest.id.find("contract") == std::string::npos
            && quest.id.find("cloud") == std::string::npos;
        if (!quest.id.empty()) {
            out_quests.push_back(std::move(quest));
        }
    }
    return true;
}

void QuestManager::RefreshByTimeslot(std::vector<RuntimeQuest>& quests, int current_hour) const {
    if (current_hour < 6) {
        return;
    }
    const bool valid_slot = (current_hour == 6 || current_hour == 12 || current_hour == 18);
    if (!valid_slot) {
        return;
    }
    int activated = 0;
    for (auto& quest : quests) {
        if (quest.auto_accept && quest.state == QuestState::NotTaken) {
            quest.state = QuestState::InProgress;
            ++activated;
            if (activated >= 2) {
                break;
            }
        }
    }
}

void QuestManager::EvaluateProgress(
    std::vector<RuntimeQuest>& quests,
    const CloudSeamanor::domain::Inventory& inventory,
    int spirit_farm_level,
    int& gold,
    const std::function<void(const std::string&)>& on_completed) const {
    for (auto& quest : quests) {
        if (quest.state != QuestState::InProgress) {
            continue;
        }

        bool completed = false;
        if (quest.id == "daily_commission_tea" && inventory.CountOf("TeaLeaf") >= 3) {
            completed = true;
        } else if (quest.id == "tool_upgrade_intro" && spirit_farm_level >= 2) {
            completed = true;
        }

        if (!completed) {
            continue;
        }

        quest.state = QuestState::Completed;
        int gold_reward = 0;
        if (quest.reward.find("金币") != std::string::npos) {
            std::string digits;
            for (char ch : quest.reward) {
                if (std::isdigit(static_cast<unsigned char>(ch)) != 0) {
                    digits.push_back(ch);
                }
            }
            if (!digits.empty()) {
                gold_reward = std::stoi(digits);
            }
        }
        if (gold_reward > 0) {
            gold += gold_reward;
        }
        if (on_completed) {
            std::string msg = "完成委托：" + quest.title + "。";
            if (gold_reward > 0) {
                msg += " 获得金币 " + std::to_string(gold_reward) + "。";
            }
            on_completed(msg);
        }
    }
}

} // namespace CloudSeamanor::engine

