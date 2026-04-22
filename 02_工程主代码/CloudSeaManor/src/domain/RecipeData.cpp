#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/RecipeData.hpp"

#include "CloudSeamanor/Logger.hpp"

#include <fstream>
#include <sstream>

namespace CloudSeamanor::domain {

namespace {
const std::string& ItemDisplayLabel_(const std::string& item_id) {
    return item_id;
}
}

// ============================================================================
// 【RecipeTable::LoadFromFile】从 CSV 加载配方表
// ============================================================================
bool RecipeTable::LoadFromFile(const std::string& file_path) {
    if (!recipes_.empty()) {
        return true;  // 已加载，直接返回
    }

    std::ifstream file(file_path);
    if (!file.is_open()) {
        infrastructure::Logger::Warning("无法打开配方数据表：" + file_path);
        return false;
    }

    std::string line;
    // 跳过标题行
    if (!std::getline(file, line)) {
        return false;
    }

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream ss(line);
        std::string field;
        RecipeDefinition def;
        int column = 0;

        while (std::getline(ss, field, ',')) {
            switch (column) {
            case 0: def.id = field; break;
            case 1: def.name = field; break;
            case 2: def.machine_id = field; break;
            case 3: def.input_item = field; break;
            case 4: def.input_count = std::stoi(field); break;
            case 5: def.output_item = field; break;
            case 6: def.output_count = std::stoi(field); break;
            case 7: def.process_time = std::stoi(field); break;
            case 8: def.base_success_rate = std::stof(field); break;
            case 9: def.tags = ParseTags_(field); break;
            }
            ++column;
        }

        if (!def.id.empty()) {
            recipes_.push_back(def);
            const std::size_t idx = recipes_.size() - 1;
            index_[def.id] = idx;
            machine_index_.emplace(def.machine_id, idx);
        }
    }

    infrastructure::Logger::Info("已加载 " + std::to_string(recipes_.size()) + " 条配方数据。");
    return !recipes_.empty();
}

// ============================================================================
// 【RecipeTable::Get】按 ID 获取配方定义
// ============================================================================
const RecipeDefinition* RecipeTable::Get(const std::string& id) const {
    const auto it = index_.find(id);
    if (it != index_.end()) {
        return &recipes_[it->second];
    }
    return nullptr;
}

// ============================================================================
// 【RecipeTable::GetByMachine】按机器 ID 获取配方列表
// ============================================================================
std::vector<const RecipeDefinition*> RecipeTable::GetByMachine(
    const std::string& machine_id) const {
    std::vector<const RecipeDefinition*> result;
    const auto range = machine_index_.equal_range(machine_id);
    for (auto it = range.first; it != range.second; ++it) {
        result.push_back(&recipes_[it->second]);
    }
    return result;
}

// ============================================================================
// 【RecipeTable::GetByTag】按标签获取配方列表
// ============================================================================
std::vector<const RecipeDefinition*> RecipeTable::GetByTag(
    const std::string& tag) const {
    std::vector<const RecipeDefinition*> result;
    for (const auto& recipe : recipes_) {
        for (const auto& t : recipe.tags) {
            if (t == tag) {
                result.push_back(&recipe);
                break;
            }
        }
    }
    return result;
}

// ============================================================================
// 【RecipeTable::Exists】检查配方是否存在
// ============================================================================
bool RecipeTable::Exists(const std::string& id) const {
    return index_.contains(id);
}

// ============================================================================
// 【RecipeTable::AvailableRecipes】获取指定库存可制作的配方
// ============================================================================
std::vector<const RecipeDefinition*> RecipeTable::AvailableRecipes(
    const std::string& machine_id,
    const std::unordered_map<std::string, int>& item_counts) const {
    std::vector<const RecipeDefinition*> result;
    for (const auto* recipe : GetByMachine(machine_id)) {
        const auto it = item_counts.find(recipe->input_item);
        if (it != item_counts.end() && it->second >= recipe->input_count) {
            result.push_back(recipe);
        }
    }
    return result;
}

// ============================================================================
// 【RecipeTable::ParseTags_】解析分号分隔的标签字符串
// ============================================================================
std::vector<std::string> RecipeTable::ParseTags_(const std::string& raw) {
    std::vector<std::string> result;
    std::istringstream ss(raw);
    std::string tag;
    while (std::getline(ss, tag, ';')) {
        if (!tag.empty()) {
            result.push_back(tag);
        }
    }
    return result;
}

// ============================================================================
// 【RecipeTable::RecipeDisplayText】构建配方 HUD 显示文本
// ============================================================================
std::string RecipeTable::RecipeDisplayText(const RecipeDefinition& recipe) {
    return recipe.name + "：" + ItemDisplayLabel_(recipe.input_item) + " x" +
           std::to_string(recipe.input_count) + " -> " +
           ItemDisplayLabel_(recipe.output_item) + " x" +
           std::to_string(recipe.output_count) +
           "（" + std::to_string(recipe.process_time) + "秒）";
}

// ============================================================================
// 【GetGlobalRecipeTable】全局配方表单例
// ============================================================================
RecipeTable& GetGlobalRecipeTable() {
    static RecipeTable table;
    return table;
}

} // namespace CloudSeamanor::domain
