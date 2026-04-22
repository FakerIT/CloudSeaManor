#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/CropData.hpp"

#include "CloudSeamanor/Logger.hpp"

#include <fstream>
#include <sstream>

namespace CloudSeamanor::domain {

// ============================================================================
// 【CropTable::LoadFromFile】从 CSV 加载作物表
// ============================================================================
bool CropTable::LoadFromFile(const std::string& file_path) {
    if (!crops_.empty()) {
        return true;  // 已加载，直接返回
    }

    std::ifstream file(file_path);
    if (!file.is_open()) {
        infrastructure::Logger::Warning("无法打开作物数据表：" + file_path);
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

        CropDefinition def;
        int column = 0;

        while (std::getline(ss, field, ',')) {
            switch (column) {
            case 0: def.id = field; break;
            case 1: def.name = field; break;
            case 2: def.seed_item_id = field; break;
            case 3: def.harvest_item_id = field; break;
            case 4: def.growth_time = std::stof(field); break;
            case 5: def.stages = std::stoi(field); break;
            case 6: def.base_harvest = std::stoi(field); break;
            case 7: def.stamina_cost = std::stoi(field); break;
            case 8: def.tags = ParseTags_(field); break;
            }
            ++column;
        }

        if (!def.id.empty()) {
            crops_.push_back(def);
            index_[def.id] = crops_.size() - 1;
        }
    }

    infrastructure::Logger::Info("已加载 " + std::to_string(crops_.size()) + " 条作物数据。");
    return !crops_.empty();
}

// ============================================================================
// 【CropTable::Get】按 ID 获取作物定义
// ============================================================================
const CropDefinition* CropTable::Get(const std::string& id) const {
    const auto it = index_.find(id);
    if (it != index_.end()) {
        return &crops_[it->second];
    }
    return nullptr;
}

// ============================================================================
// 【CropTable::Exists】检查作物是否存在
// ============================================================================
bool CropTable::Exists(const std::string& id) const {
    return index_.contains(id);
}

// ============================================================================
// 【CropTable::GetByTag】按标签获取作物列表
// ============================================================================
std::vector<const CropDefinition*> CropTable::GetByTag(const std::string& tag) const {
    std::vector<const CropDefinition*> result;
    for (const auto& crop : crops_) {
        for (const auto& t : crop.tags) {
            if (t == tag) {
                result.push_back(&crop);
                break;
            }
        }
    }
    return result;
}

// ============================================================================
// 【CropTable::ParseTags_】解析逗号分隔的标签字符串
// ============================================================================
std::vector<std::string> CropTable::ParseTags_(const std::string& raw) {
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
// 【CropTable::CalculateQuality】根据云海状态计算作物品质
// ============================================================================
CropQuality CropTable::CalculateQuality(const CloudState state, const bool is_spirit_tier) {
    switch (state) {
    case CloudState::Clear:
        return CropQuality::Normal;
    case CloudState::Mist:
        return CropQuality::Normal;  // 薄雾基础为普通
    case CloudState::DenseCloud:
        return CropQuality::Fine;   // 浓云稳定出优质
    case CloudState::Tide:
        return is_spirit_tier ? CropQuality::Spirit : CropQuality::Rare;
    }
    return CropQuality::Normal;
}

// ============================================================================
// 【CropTable::QualityToText】品质等级转中文文本
// ============================================================================
const char* CropTable::QualityToText(const CropQuality q) {
    switch (q) {
    case CropQuality::Normal: return "普通";
    case CropQuality::Fine:   return "优质";
    case CropQuality::Rare:   return "稀有";
    case CropQuality::Spirit: return "灵品";
    case CropQuality::Holy:   return "圣品";
    }
    return "普通";
}

// ============================================================================
// 【CropTable::QualityToPrefixText】品质等级转带前缀的文本
// ============================================================================
std::string CropTable::QualityToPrefixText(const CropQuality q) {
    switch (q) {
    case CropQuality::Normal: return "普通";
    case CropQuality::Fine:   return "【优质】";
    case CropQuality::Rare:   return "【稀有】";
    case CropQuality::Spirit: return "【灵品】";
    case CropQuality::Holy:   return "【圣品】";
    }
    return "普通";
}

// ============================================================================
// 【CropTable::QualityHarvestMultiplier】品质对应的产量倍率
// ============================================================================
float CropTable::QualityHarvestMultiplier(const CropQuality q) {
    switch (q) {
    case CropQuality::Normal: return 1.0f;
    case CropQuality::Fine:   return 1.3f;
    case CropQuality::Rare:   return 1.8f;
    case CropQuality::Spirit: return 2.5f;
    case CropQuality::Holy:   return 3.5f;
    }
    return 1.0f;
}

// ============================================================================
// 【CropTable::StageGrowthThreshold】某阶段的生长阈值
// ============================================================================
float CropTable::StageGrowthThreshold(const int stage, const int total_stages) {
    if (total_stages <= 0) return 1.0f;
    if (stage <= 0) return 0.0f;
    if (stage >= total_stages) return 1.0f;
    return static_cast<float>(stage) / static_cast<float>(total_stages);
}

// ============================================================================
// 【GetGlobalCropTable】全局作物表单例
// ============================================================================
CropTable& GetGlobalCropTable() {
    static CropTable table;
    return table;
}

} // namespace CloudSeamanor::domain
