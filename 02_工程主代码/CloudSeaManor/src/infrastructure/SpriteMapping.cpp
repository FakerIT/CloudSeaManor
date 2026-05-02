// ============================================================================
// 【SpriteMapping】美术资源映射表管理器 - 实现
// ============================================================================

#include "CloudSeamanor/infrastructure/SpriteMapping.hpp"

#include "CloudSeamanor/infrastructure/Logger.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>

namespace CloudSeamanor::infrastructure {

bool SpriteMapping::LoadFromFile(const std::string& csv_path, const std::string& base_path) {
    namespace fs = std::filesystem;

    base_path_ = base_path;
    mappings_.clear();
    category_index_.clear();

    // 构建完整路径
    fs::path full_path = csv_path;
    if (!full_path.is_absolute() && !base_path.empty()) {
        full_path = fs::path(base_path) / csv_path;
    }

    if (!fs::exists(full_path)) {
        Logger::Error("SpriteMapping: CSV file not found: " + full_path.string());
        return false;
    }

    std::ifstream file(full_path);
    if (!file.is_open()) {
        Logger::Error("SpriteMapping: Cannot open CSV file: " + full_path.string());
        return false;
    }

    std::string line;
    int line_number = 0;
    bool header_found = false;
    std::vector<std::string> headers;

    while (std::getline(file, line)) {
        line_number++;

        // 跳过空行
        if (line.empty() || Trim(line).empty()) continue;

        // 跳过注释行
        std::string trimmed = Trim(line);
        if (trimmed.starts_with('#')) continue;

        // 解析 CSV 行
        auto fields = ParseCsvLine(line);

        // 解析表头
        if (!header_found) {
            headers = fields;
            header_found = true;

            // 验证必需的列
            bool has_sprite_id = false, has_category = false, has_file_path = false;
            for (const auto& h : headers) {
                if (h == "SpriteId") has_sprite_id = true;
                if (h == "Category") has_category = true;
                if (h == "FilePath") has_file_path = true;
            }

            if (!has_sprite_id || !has_category || !has_file_path) {
                Logger::Error("SpriteMapping: CSV missing required columns (SpriteId/Category/FilePath). File: "
                             + full_path.string());
                return false;
            }
            continue;
        }

        // 跳过字段数不匹配的行
        if (fields.size() != headers.size()) {
            Logger::Warning("SpriteMapping: Line " + std::to_string(line_number)
                          + " has mismatched field count, skipping.");
            continue;
        }

        // 构建字段映射
        std::unordered_map<std::string, std::string> row;
        for (size_t i = 0; i < headers.size(); ++i) {
            row[headers[i]] = Trim(fields[i]);
        }

        // 创建 SpriteInfo
        SpriteInfo info;
        info.sprite_id = row["SpriteId"];
        info.category = row["Category"];
        info.file_path = row["FilePath"];

        // 可选字段
        if (row.count("FrameCount") && !row["FrameCount"].empty())
            info.frame_count = ParseInt(row["FrameCount"]);
        if (row.count("FPS") && !row["FPS"].empty())
            info.fps = ParseInt(row["FPS"]);
        if (row.count("Width") && !row["Width"].empty())
            info.width = ParseInt(row["Width"]);
        if (row.count("Height") && !row["Height"].empty())
            info.height = ParseInt(row["Height"]);
        if (row.count("Description") && !row["Description"].empty())
            info.description = row["Description"];
        if (row.count("AtlasSource") && !row["AtlasSource"].empty())
            info.atlas_source = row["AtlasSource"];

        // 检查重复 ID
        if (mappings_.count(info.sprite_id)) {
            Logger::Warning("SpriteMapping: Duplicate SpriteId '" + info.sprite_id
                          + "' at line " + std::to_string(line_number) + ", skipping.");
            continue;
        }

        // 存储
        mappings_[info.sprite_id] = info;
        category_index_[info.category].push_back(info.sprite_id);
    }

    file.close();

    loaded_ = true;
    loaded_path_ = full_path.string();

    Logger::Info("SpriteMapping: Loaded " + std::to_string(mappings_.size())
               + " sprites from " + full_path.filename().string());
    return true;
}

bool SpriteMapping::Reload(const std::string& csv_path, const std::string& base_path) {
    return LoadFromFile(csv_path, base_path);
}

std::optional<SpriteInfo> SpriteMapping::Get(const std::string& sprite_id) const {
    auto it = mappings_.find(sprite_id);
    if (it != mappings_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<SpriteInfo> SpriteMapping::GetByCategory(const std::string& category) const {
    std::vector<SpriteInfo> result;
    auto it = category_index_.find(category);
    if (it != category_index_.end()) {
        for (const auto& id : it->second) {
            auto info = Get(id);
            if (info.has_value()) {
                result.push_back(info.value());
            }
        }
    }
    return result;
}

std::vector<std::string> SpriteMapping::GetAllCategories() const {
    std::vector<std::string> categories;
    for (const auto& pair : category_index_) {
        categories.push_back(pair.first);
    }
    std::sort(categories.begin(), categories.end());
    return categories;
}

std::vector<std::string> SpriteMapping::ValidateFiles() const {
    namespace fs = std::filesystem;
    std::vector<std::string> missing_files;

    for (const auto& [id, info] : mappings_) {
        std::string full_path = info.GetFullPath(base_path_);
        if (!fs::exists(full_path)) {
            missing_files.push_back(full_path);
        }
    }

    return missing_files;
}

std::vector<SpriteMapping::CategoryStats> SpriteMapping::GetCategoryStats() const {
    std::vector<CategoryStats> stats;
    for (const auto& [category, ids] : category_index_) {
        stats.push_back({category, ids.size()});
    }
    std::sort(stats.begin(), stats.end(),
              [](const CategoryStats& a, const CategoryStats& b) {
                  return a.count > b.count;
              });
    return stats;
}

std::vector<std::string> SpriteMapping::ParseCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool in_quotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (c == '"') {
            in_quotes = !in_quotes;
        } else if (c == ',' && !in_quotes) {
            fields.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    fields.push_back(field);  // 最后一个字段

    return fields;
}

int SpriteMapping::ParseInt(const std::string& str, int default_val) {
    try {
        return std::stoi(str);
    } catch (...) {
        return default_val;
    }
}

std::string SpriteMapping::Trim(const std::string& str) {
    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }
    size_t end = str.size();
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }
    return str.substr(start, end - start);
}

}  // namespace CloudSeamanor::infrastructure
