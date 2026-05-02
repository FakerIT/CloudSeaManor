#include "CloudSeamanor/domain/RecipeData.hpp"

#include "CloudSeamanor/infrastructure/Logger.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <sstream>
#include <regex>

namespace CloudSeamanor::domain {

namespace {
const std::string& ItemDisplayLabel_(const std::string& item_id) {
    return item_id;
}

std::optional<int> ParseIntField_(const std::string& raw, const std::string& field_name, int line_no) {
    try {
        return std::stoi(raw);
    } catch (...) {
        infrastructure::Logger::Warning(
            "RecipeTable: 行 " + std::to_string(line_no) + " 的整数字段非法: " + field_name + "=" + raw);
        return std::nullopt;
    }
}

std::optional<float> ParseFloatField_(const std::string& raw, const std::string& field_name, int line_no) {
    try {
        return std::stof(raw);
    } catch (...) {
        infrastructure::Logger::Warning(
            "RecipeTable: 行 " + std::to_string(line_no) + " 的浮点字段非法: " + field_name + "=" + raw);
        return std::nullopt;
    }
}

// 解析 "item_idx" 格式，返回 (item_id, count)
std::pair<std::string, int> ParseItemWithCount_(const std::string& raw) {
    std::pair<std::string, int> result;
    result.second = 1; // 默认数量为1

    // 匹配 "item_name x count" 或 "item_name xcount"
    std::regex pattern(R"(^(.+?)\s*[xX]?\s*(\d+)$)");
    std::smatch match;
    if (std::regex_match(raw, match, pattern)) {
        result.first = match[1].str();
        try {
            result.second = std::stoi(match[2].str());
        } catch (...) {
            result.second = 1;
        }
    } else {
        result.first = raw;
    }
    return result;
}

// 解析 "item1xcount1;item2xcount2" 格式的组合材料
std::vector<std::pair<std::string, int>> ParseMultiItemRequirement_(const std::string& raw) {
    std::vector<std::pair<std::string, int>> result;
    if (raw.empty()) return result;

    std::istringstream ss(raw);
    std::string token;
    while (std::getline(ss, token, ';')) {
        if (!token.empty()) {
            result.push_back(ParseItemWithCount_(token));
        }
    }
    return result;
}

} // anonymous namespace

// ============================================================================
// 【RecipeTable::LoadFromFile】从 CSV 加载配方表
// ============================================================================
// CSV格式：Id,Name,Result,EffectType,EffectValue,Duration,Cooldown,RequiredItems,RequiredStamina,Description
// ============================================================================
bool RecipeTable::LoadFromFile(const std::string& file_path) {
    recipes_.clear();
    index_.clear();
    machine_index_.clear();

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

    int line_no = 1;
    int duplicate_id_count = 0;
    int invalid_row_count = 0;

    while (std::getline(file, line)) {
        ++line_no;
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream ss(line);
        std::string field;
        RecipeDefinition def;
        int column = 0;

        // 存储原始字段值用于后续处理
        std::string result_field;
        std::string effect_type_field;
        std::string effect_value_field;
        std::string required_items_field;
        int required_stamina = 0;

        while (std::getline(ss, field, ',')) {
            switch (column) {
            case 0: def.id = field; break;
            case 1: def.name = field; break;
            case 2: result_field = field; break;
            case 3: effect_type_field = field; break;
            case 4: effect_value_field = field; break;
            case 5: /* Duration - 暂不处理 */ break;
            case 6: /* Cooldown - 暂不处理 */ break;
            case 7: required_items_field = field; break;
            case 8: {
                const auto parsed = ParseIntField_(field, "RequiredStamina", line_no);
                if (parsed) {
                    required_stamina = *parsed;
                }
                break;
            }
            case 9: /* Description - 暂不处理 */ break;
            }
            ++column;
        }

        // 处理 Result 字段 (格式: "output_item x count" 或 "output_item")
        if (!result_field.empty()) {
            auto [output_item, output_count] = ParseItemWithCount_(result_field);
            def.output_item = output_item;
            def.output_count = output_count;
        }

        // 处理 EffectValue 字段 (作为 output_count 或其他数值)
        if (!effect_value_field.empty()) {
            const auto parsed = ParseFloatField_(effect_value_field, "EffectValue", line_no);
            if (parsed) {
                // 如果 output_count 还没有被正确设置，使用 EffectValue
                if (def.output_count == 1 && *parsed >= 1.0f) {
                    def.output_count = static_cast<int>(*parsed);
                }
                // 同时设置基础成功率（如果小于等于1认为是概率）
                if (*parsed <= 1.0f && *parsed > 0) {
                    def.base_success_rate = *parsed;
                }
            }
        }

        // 处理 RequiredItems 字段 (格式: "item1xcount1;item2xcount2")
        if (!required_items_field.empty()) {
            auto items = ParseMultiItemRequirement_(required_items_field);
            if (!items.empty()) {
                def.input_item = items[0].first;
                def.input_count = items[0].second;
            }
        }

        // 处理 RequiredStamina 字段 (作为加工时间)
        if (required_stamina > 0) {
            def.process_time = required_stamina;
        }

        // 验证必填字段
        if (def.id.empty()) {
            ++invalid_row_count;
            infrastructure::Logger::Warning(
                "RecipeTable: 跳过非法配方行 " + std::to_string(line_no)
                + "（id 不能为空）");
            continue;
        }

        // 如果 input_item 为空，设置为默认值
        if (def.input_item.empty()) {
            def.input_item = def.id; // 使用配方ID作为默认输入
        }

        // 如果 output_item 为空，设置为默认值
        if (def.output_item.empty()) {
            def.output_item = def.id + "_result";
        }

        if (index_.contains(def.id)) {
            ++duplicate_id_count;
            infrastructure::Logger::Warning(
                "RecipeTable: Duplicate recipe id=" + def.id
                + " (line " + std::to_string(line_no) + "), later entry overwrites earlier.");
            recipes_[index_[def.id]] = def;
            continue;
        }

        recipes_.push_back(def);
        const std::size_t idx = recipes_.size() - 1;
        index_[def.id] = idx;
        machine_index_.emplace(def.machine_id, idx);
    }

    infrastructure::Logger::Info(
        "RecipeTable: 已加载 " + std::to_string(recipes_.size())
        + " 条配方；重复覆盖=" + std::to_string(duplicate_id_count)
        + "；非法行=" + std::to_string(invalid_row_count)
        + "；source=" + file_path);
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
