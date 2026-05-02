#include "CloudSeamanor/ContractSystem.hpp"
#include "CloudSeamanor/GameConstants.hpp"
#include "CloudSeamanor/Inventory.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>

namespace CloudSeamanor::domain {

// ============================================================================
// ContractSystem 实现
// ============================================================================
ContractSystem& ContractSystem::Instance() {
    static ContractSystem instance;
    return instance;
}

bool ContractSystem::Initialize() {
    const auto& assets = GameConstants::AssetsPath();

    LoadContractData(assets + "/data/contracts/contract_data.csv");
    InitializeProgress();

    return !contracts_.empty();
}

void ContractSystem::LoadContractData(const std::string& csv_path) {
    std::ifstream file(csv_path);
    if (!file.is_open()) return;

    std::string line;
    bool in_contract_section = false;
    bool in_bundle_section = false;
    bool in_reward_section = false;
    bool in_spirit_section = false;

    ContractData* current_contract = nullptr;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        // 检测章节
        if (line.find("Bundle条目数据表") != std::string::npos) {
            in_bundle_section = true;
            in_contract_section = false;
            in_reward_section = false;
            in_spirit_section = false;
            std::getline(file, line); // 跳过表头
            continue;
        }
        if (line.find("契约奖励数据表") != std::string::npos) {
            in_reward_section = true;
            in_bundle_section = false;
            in_spirit_section = false;
            std::getline(file, line);
            continue;
        }
        if (line.find("茶灵图鉴数据表") != std::string::npos) {
            in_spirit_section = true;
            in_reward_section = false;
            std::getline(file, line);
            continue;
        }

        // 解析契约数据
        if (!in_bundle_section && !in_reward_section && !in_spirit_section && line.find(',') != std::string::npos) {
            std::stringstream ss(line);
            std::string id, name_zh, name_en, desc_zh, req_item, unlock_cond, reward_item, reward_count, prereq;

            std::getline(ss, id, ',');
            std::getline(ss, name_zh, ',');
            std::getline(ss, name_en, ',');
            std::getline(ss, desc_zh, ',');
            std::getline(ss, req_item, ',');
            std::getline(ss, unlock_cond, ',');
            std::getline(ss, reward_item, ',');
            std::getline(ss, reward_count, ',');
            std::getline(ss, prereq, ',');

            ContractData contract;
            contract.id = id;
            contract.name_zh = name_zh;
            contract.name_en = name_en;
            contract.description_zh = desc_zh;
            contract.required_item = req_item;
            contract.unlock_condition = unlock_cond;
            contract.reward_item_id = reward_item;
            contract.reward_count = std::stoi(reward_count);
            contract.prerequisite_id = prereq;

            contracts_[contract.id] = contract;
            current_contract = &contracts_[contract.id];
        }

        // 解析Bundle数据
        if (in_bundle_section && line.find(',') != std::string::npos) {
            std::stringstream ss(line);
            std::string bundle_id, contract_id, item_id, count, name_zh, desc_zh;

            std::getline(ss, bundle_id, ',');
            std::getline(ss, contract_id, ',');
            std::getline(ss, item_id, ',');
            std::getline(ss, count, ',');
            std::getline(ss, name_zh, ',');
            std::getline(ss, desc_zh, ',');

            ContractBundle bundle;
            bundle.id = bundle_id;
            bundle.contract_id = contract_id;
            bundle.item_id = item_id;
            bundle.item_count = std::stoi(count);
            bundle.item_name_zh = name_zh;
            bundle.description_zh = desc_zh;

            bundles_[bundle.id] = bundle;

            if (contracts_.count(contract_id)) {
                contracts_[contract_id].bundles.push_back(bundle);
            }
        }

        // 解析Reward数据
        if (in_reward_section && line.find(',') != std::string::npos) {
            std::stringstream ss(line);
            std::string reward_id, contract_id, reward_type, reward_item, count, desc_zh, bonus;

            std::getline(ss, reward_id, ',');
            std::getline(ss, contract_id, ',');
            std::getline(ss, reward_type, ',');
            std::getline(ss, reward_item, ',');
            std::getline(ss, count, ',');
            std::getline(ss, desc_zh, ',');
            std::getline(ss, bonus, ',');

            ContractReward reward;
            reward.id = reward_id;
            reward.contract_id = contract_id;
            reward.reward_type = reward_type;
            reward.reward_id = reward_item;
            reward.reward_count = std::stoi(count);
            reward.description_zh = desc_zh;
            reward.bonus_effect = bonus;

            rewards_[reward.id] = reward;

            if (contracts_.count(contract_id)) {
                contracts_[contract_id].rewards.push_back(reward);
            }
        }

        // 解析Spirit数据
        if (in_spirit_section && line.find(',') != std::string::npos) {
            std::stringstream ss(line);
            std::string spirit_id, contract_id, name_zh, name_en, desc_zh, color, animation, habitat;

            std::getline(ss, spirit_id, ',');
            std::getline(ss, contract_id, ',');
            std::getline(ss, name_zh, ',');
            std::getline(ss, name_en, ',');
            std::getline(ss, desc_zh, ',');
            std::getline(ss, color, ',');
            std::getline(ss, animation, ',');
            std::getline(ss, habitat, ',');

            TeaSpirit spirit;
            spirit.id = spirit_id;
            spirit.contract_id = contract_id;
            spirit.name_zh = name_zh;
            spirit.name_en = name_en;
            spirit.description_zh = desc_zh;
            spirit.visual_color = color;
            spirit.unlock_animation = animation;
            spirit.habitat_zh = habitat;

            spirits_[spirit.id] = spirit;

            if (contracts_.count(contract_id)) {
                contracts_[contract_id].spirit = spirit;
            }
        }
    }
}

void ContractSystem::InitializeProgress() {
    for (const auto& [id, contract] : contracts_) {
        ContractProgress progress;
        progress.contract_id = id;
        progress.total_required = static_cast<int>(contract.bundles.size());
        progress_[id] = progress;

        for (const auto& bundle : contract.bundles) {
            BundleItemProgress item_progress;
            item_progress.bundle_id = bundle.id;
            item_progress.item_id = bundle.item_id;
            item_progress.required = bundle.item_count;
            item_progress.collected = 0;
            bundle_progress_[bundle.id] = item_progress;
        }
    }
}

// ============================================================================
// 查询接口
// ============================================================================
const ContractData* ContractSystem::GetContract(const std::string& contract_id) const {
    auto it = contracts_.find(contract_id);
    return it != contracts_.end() ? &it->second : nullptr;
}

std::vector<const ContractData*> ContractSystem::GetAllContracts() const {
    std::vector<const ContractData*> result;
    for (const auto& [id, contract] : contracts_) {
        result.push_back(&contract);
    }
    return result;
}

std::vector<const ContractData*> ContractSystem::GetAvailableContracts() const {
    std::vector<const ContractData*> result;
    for (const auto& [id, contract] : contracts_) {
        if (IsContractUnlocked(id) && !IsContractCompleted(id)) {
            result.push_back(&contract);
        }
    }
    return result;
}

std::vector<const ContractData*> ContractSystem::GetCompletedContracts() const {
    std::vector<const ContractData*> result;
    for (const auto& [id, contract] : contracts_) {
        if (IsContractCompleted(id)) {
            result.push_back(&contract);
        }
    }
    return result;
}

bool ContractSystem::IsContractUnlocked(const std::string& contract_id) const {
    const auto* contract = GetContract(contract_id);
    if (!contract) return false;

    // 检查前置条件
    return CheckPrerequisite(contract_id);
}

bool ContractSystem::IsContractCompleted(const std::string& contract_id) const {
    auto it = progress_.find(contract_id);
    return it != progress_.end() && it->second.is_completed;
}

bool ContractSystem::IsContractRewarded(const std::string& contract_id) const {
    auto it = progress_.find(contract_id);
    return it != progress_.end() && it->second.is_rewarded;
}

const ContractProgress* ContractSystem::GetProgress(const std::string& contract_id) const {
    auto it = progress_.find(contract_id);
    return it != progress_.end() ? &it->second : nullptr;
}

std::vector<const BundleItemProgress*> ContractSystem::GetBundleProgress(const std::string& contract_id) const {
    std::vector<const BundleItemProgress*> result;
    const auto* contract = GetContract(contract_id);
    if (!contract) return result;

    for (const auto& bundle : contract->bundles) {
        auto it = bundle_progress_.find(bundle.id);
        if (it != bundle_progress_.end()) {
            result.push_back(&it->second);
        }
    }
    return result;
}

// ============================================================================
// 物品收集
// ============================================================================
void ContractSystem::CollectItem(const std::string& item_id, int count) {
    for (auto& [bundle_id, bundle] : bundles_) {
        if (bundle.item_id == item_id) {
            auto& item_progress = bundle_progress_[bundle_id];
            item_progress.collected = std::min(item_progress.collected + count, item_progress.required);

            if (on_bundle_item_collected_) {
                on_bundle_item_collected_(item_id, count);
            }

            UpdateContractProgress(bundle.contract_id);
        }
    }
}

// ============================================================================
// 契约操作
// ============================================================================
bool ContractSystem::TryCompleteContract(const std::string& contract_id) {
    // 默认使用 1 作为兼容值
    return TryCompleteContract(contract_id, 1);
}

bool ContractSystem::TryCompleteContract(const std::string& contract_id, int current_day) {
    if (!IsContractUnlocked(contract_id)) return false;
    if (IsContractCompleted(contract_id)) return false;

    // 检查是否所有bundle都完成
    auto bundle_progress = GetBundleProgress(contract_id);
    for (const auto* bp : bundle_progress) {
        if (!bp->IsComplete()) return false;
    }

    // 完成契约，记录当前天数
    progress_[contract_id].is_completed = true;
    progress_[contract_id].completed_day = current_day;

    TriggerContractComplete(contract_id);
    return true;
}

bool ContractSystem::TryClaimReward(const std::string& contract_id) {
    if (!IsContractCompleted(contract_id)) return false;
    if (IsContractRewarded(contract_id)) return false;

    GrantReward(contract_id);
    progress_[contract_id].is_rewarded = true;

    if (on_reward_claimed_) {
        on_reward_claimed_(contract_id);
    }

    return true;
}

// ============================================================================
// 统计
// ============================================================================
int ContractSystem::GetCompletedCount() const {
    int count = 0;
    for (const auto& [id, progress] : progress_) {
        if (progress.is_completed) count++;
    }
    return count;
}

int ContractSystem::GetTotalContractCount() const {
    return static_cast<int>(contracts_.size());
}

float ContractSystem::GetOverallProgress() const {
    if (contracts_.empty()) return 0.0f;
    int total_bundles = 0;
    int completed_bundles = 0;

    for (const auto& [id, bp] : bundle_progress_) {
        total_bundles++;
        if (bp.collected >= bp.required) {
            completed_bundles++;
        }
    }

    return total_bundles > 0 ? static_cast<float>(completed_bundles) / total_bundles : 0.0f;
}

// ============================================================================
// 存档
// ============================================================================
void ContractSystem::LoadProgress(
    const std::map<std::string, ContractProgress>& progress,
    const std::map<std::string, BundleItemProgress>& bundle_progress
) {
    progress_ = progress;
    bundle_progress_ = bundle_progress;
}

// ============================================================================
// 辅助方法
// ============================================================================
bool ContractSystem::CheckPrerequisite(const std::string& contract_id) const {
    const auto* contract = GetContract(contract_id);
    if (!contract) return false;

    // 无前置契约则解锁
    if (contract->prerequisite_id.empty()) return true;

    // 检查前置契约是否完成
    return IsContractCompleted(contract->prerequisite_id);
}

void ContractSystem::UpdateContractProgress(const std::string& contract_id) {
    auto bundle_progress = GetBundleProgress(contract_id);
    int completed_count = 0;

    for (const auto* bp : bundle_progress) {
        if (bp->IsComplete()) {
            completed_count++;
        }
    }

    progress_[contract_id].completion_count = completed_count;
}

void ContractSystem::TriggerContractComplete(const std::string& contract_id) {
    const auto* contract = GetContract(contract_id);
    if (!contract) return;

    ContractCompleteInfo info;
    info.contract_id = contract_id;
    info.contract_name = contract->name_zh;

    if (!contract->spirit.name_zh.empty()) {
        info.spirit_name = contract->spirit.name_zh;
    }

    for (const auto& reward : contract->rewards) {
        info.reward_description = reward.description_zh;
        info.bonus_effects.push_back(reward.bonus_effect);
    }

    if (on_contract_complete_) {
        on_contract_complete_(info);
    }
}

void ContractSystem::GrantReward(const std::string& contract_id) {
    const auto* contract = GetContract(contract_id);
    if (!contract) return;

    for (const auto& reward : contract->rewards) {
        if (on_reward_grant_) {
            // 通过回调让外部系统处理奖励发放
            on_reward_grant_(reward.reward_type, reward.reward_id, reward.reward_count);
        }
    }
}

}  // namespace CloudSeamanor::domain
