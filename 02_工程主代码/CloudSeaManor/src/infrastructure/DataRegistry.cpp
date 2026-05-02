#include "CloudSeamanor/infrastructure/DataRegistry.hpp"

#include "CloudSeamanor/infrastructure/JsonValue.hpp"
#include "CloudSeamanor/infrastructure/ResourceManager.hpp"

#include <cctype>
#include <fstream>
#include <sstream>

namespace CloudSeamanor::infrastructure {

namespace {

std::filesystem::path NormalizeRelativePath_(const std::string& relative_path) {
    return std::filesystem::path(relative_path).lexically_normal();
}

std::string SeverityToString_(DataIssueSeverity severity) {
    switch (severity) {
    case DataIssueSeverity::Info: return "info";
    case DataIssueSeverity::Warning: return "warning";
    case DataIssueSeverity::Error: return "error";
    }
    return "info";
}

std::string BuildTextResourceId_(const std::filesystem::path& path) {
    return "data:" + path.generic_string();
}

} // namespace

bool CsvRow::Has(std::string_view key) const {
    return values.contains(std::string(key));
}

std::string CsvRow::Get(std::string_view key, std::string default_value) const {
    const auto it = values.find(std::string(key));
    return it != values.end() ? it->second : std::move(default_value);
}

int CsvRow::GetInt(std::string_view key, int default_value) const {
    try {
        return std::stoi(Get(key));
    } catch (...) {
        return default_value;
    }
}

float CsvRow::GetFloat(std::string_view key, float default_value) const {
    try {
        return std::stof(Get(key));
    } catch (...) {
        return default_value;
    }
}

bool CsvRow::GetBool(std::string_view key, bool default_value) const {
    const std::string value = DataRegistry::Trim(Get(key));
    if (value.empty()) {
        return default_value;
    }
    if (value == "1" || value == "true" || value == "TRUE" || value == "yes") {
        return true;
    }
    if (value == "0" || value == "false" || value == "FALSE" || value == "no") {
        return false;
    }
    return default_value;
}

std::vector<std::string> CsvRow::GetStringList(
    std::string_view key,
    char delimiter) const {
    const std::string raw = Get(key);
    if (raw.empty()) {
        return {};
    }
    if (!raw.empty() && raw.front() == '[') {
        return DataRegistry::ParseJsonStringArray(raw);
    }

    std::vector<std::string> values_out;
    std::string current;
    std::istringstream in(raw);
    while (std::getline(in, current, delimiter)) {
        const std::string trimmed = DataRegistry::Trim(current);
        if (!trimmed.empty()) {
            values_out.push_back(trimmed);
        }
    }
    return values_out;
}

void DataRegistry::RegisterPath(const std::string& category, const std::string& path) {
    entries_.push_back({category, path});
}

void DataRegistry::SetDataRoots(std::vector<std::filesystem::path> data_roots) {
    data_roots_ = std::move(data_roots);
}

void DataRegistry::AttachResourceManager(ResourceManager* resource_manager) {
    resource_manager_ = resource_manager;
}

std::vector<std::string> DataRegistry::ValidateAndDescribe() const {
    std::vector<std::string> logs;
    for (const auto& entry : entries_) {
        const std::filesystem::path dir(entry.path);
        if (!std::filesystem::exists(dir)) {
            logs.push_back("missing|" + entry.category + "|" + entry.path);
            continue;
        }
        std::size_t file_count = 0;
        std::size_t content_hash = 0;
        for (const auto& item : std::filesystem::directory_iterator(dir)) {
            if (!item.is_regular_file()) {
                continue;
            }
            ++file_count;
            std::ifstream in(item.path(), std::ios::binary);
            std::ostringstream ss;
            ss << in.rdbuf();
            content_hash ^= std::hash<std::string>{}(ss.str());
        }
        logs.push_back("ok|" + entry.category + "|" + entry.path + "|files=" + std::to_string(file_count)
                       + "|hash=" + std::to_string(content_hash));
    }

    std::unordered_set<std::string> error_tables;
    std::unordered_set<std::string> warning_tables;

    for (const auto& issue : issues_) {
        std::ostringstream line;
        if (issue.severity == DataIssueSeverity::Error) {
            line << "[ERROR] ";
        } else if (issue.severity == DataIssueSeverity::Warning) {
            line << "[WARN] ";
        } else {
            line << "[INFO] ";
        }
        line << "table=" << issue.table_id;
        if (!issue.path.empty()) {
            line << " file=" << issue.path;
        }
        if (issue.line_number > 0) {
            line << " line=" << issue.line_number;
        }
        if (!issue.field_name.empty()) {
            line << " field=" << issue.field_name;
        }
        if (!issue.bad_value.empty()) {
            line << " value='" << issue.bad_value << "'";
        }
        line << " | " << issue.message;
        logs.push_back(line.str());

        if (issue.severity == DataIssueSeverity::Error) {
            error_tables.insert(issue.table_id);
        } else if (issue.severity == DataIssueSeverity::Warning) {
            warning_tables.insert(issue.table_id);
        }
    }

    if (!error_tables.empty()) {
        std::ostringstream summary;
        summary << "[SUMMARY] 错误表: ";
        bool first = true;
        for (const auto& t : error_tables) {
            if (!first) summary << ", ";
            summary << t;
            first = false;
        }
        logs.push_back(summary.str());
    }
    if (!warning_tables.empty()) {
        std::ostringstream summary;
        summary << "[SUMMARY] 警告表: ";
        bool first = true;
        for (const auto& t : warning_tables) {
            if (!first) summary << ", ";
            summary << t;
            first = false;
        }
        logs.push_back(summary.str());
    }

    return logs;
}

std::filesystem::path DataRegistry::ResolveDataPath(const std::string& relative_path) const {
    const auto normalized = NormalizeRelativePath_(relative_path);
    if (normalized.is_absolute() && std::filesystem::exists(normalized)) {
        return normalized;
    }

    for (const auto& root : data_roots_) {
        const auto candidate = (root / normalized).lexically_normal();
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    if (std::filesystem::exists(normalized)) {
        return normalized;
    }
    return normalized;
}

bool DataRegistry::ReadTextFile_(
    const std::filesystem::path& path,
    std::string& out_text) const {
    if (resource_manager_ != nullptr) {
        const std::string resource_id = BuildTextResourceId_(path);
        const auto result = resource_manager_->LoadTextResult(resource_id, path.string());
        if (result.Ok()) {
            out_text = resource_manager_->GetText(resource_id);
            return true;
        }
    }

    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    out_text = ss.str();
    return true;
}

bool DataRegistry::LoadCsvDocument(
    const std::string& relative_path,
    CsvDocument& out_document,
    std::vector<DataIssue>* out_issues) const {
    out_document = {};

    const auto resolved_path = ResolveDataPath(relative_path);
    std::string csv_text;
    if (!ReadTextFile_(resolved_path, csv_text)) {
        if (out_issues != nullptr) {
            out_issues->push_back(DataIssue{
                .severity = DataIssueSeverity::Error,
                .path = resolved_path.string(),
                .message = "无法读取数据表",
            });
        }
        return false;
    }

    std::istringstream input(csv_text);
    std::string line;
    bool header_loaded = false;
    std::size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        const std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }
        const auto cells = SplitCsvLine(line);
        if (!header_loaded) {
            const std::string first_cell = cells.empty() ? "" : Trim(cells[0]);
            const bool explicit_header = (first_cell == "id" || first_cell == "Id");
            out_document.header.reserve(cells.size());
            if (explicit_header) {
                for (const auto& cell : cells) {
                    out_document.header.push_back(Trim(cell));
                }
                header_loaded = true;
                continue;
            }
            for (std::size_t index = 0; index < cells.size(); ++index) {
                out_document.header.push_back("col" + std::to_string(index));
            }
            header_loaded = true;
        }

        CsvRow row;
        row.line_number = line_number;
        for (std::size_t index = 0; index < out_document.header.size(); ++index) {
            row.values[out_document.header[index]] = index < cells.size()
                ? Trim(cells[index])
                : "";
        }
        out_document.rows.push_back(std::move(row));
    }

    if (!header_loaded && out_issues != nullptr) {
        out_issues->push_back(DataIssue{
            .severity = DataIssueSeverity::Error,
            .path = resolved_path.string(),
            .message = "CSV 缺少表头",
        });
    }

    return header_loaded;
}

bool DataRegistry::LoadAll() {
    issues_.clear();
    bool all_ok = true;
    for (const auto& registration : csv_table_loaders_) {
        CsvDocument document;
        std::vector<DataIssue> local_issues;
        const bool loaded = LoadCsvDocument(
            registration.relative_path,
            document,
            &local_issues);
        for (auto& issue : local_issues) {
            issue.table_id = registration.table_id;
        }
        issues_.insert(issues_.end(), local_issues.begin(), local_issues.end());
        if (!loaded) {
            all_ok = false;
            continue;
        }

        const bool table_ok = registration.loader(document, issues_);
        if (!table_ok) {
            all_ok = false;
        }
    }
    return all_ok;
}

std::string DataRegistry::Trim(std::string_view text) {
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }
    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return std::string(text.substr(start, end - start));
}

std::vector<std::string> DataRegistry::SplitCsvLine(const std::string& line) {
    std::vector<std::string> cells;
    std::string current;
    bool in_quotes = false;
    size_t i = 0;

    while (i < line.size()) {
        char ch = line[i];
        char next_ch = (i + 1 < line.size()) ? line[i + 1] : '\0';

        if (in_quotes) {
            // 在引号内部
            if (ch == '"') {
                if (next_ch == '"') {
                    // `""` 转义序列：字面引号
                    current.push_back('"');
                    i += 2; // 跳过两个引号
                    continue;
                } else {
                    // 单个 `"`：字段结束
                    in_quotes = false;
                    i++;
                    continue;
                }
            } else {
                current.push_back(ch);
                i++;
                continue;
            }
        } else {
            // 不在引号内
            if (ch == '"') {
                // 引号开始
                in_quotes = true;
                i++;
                continue;
            } else if (ch == ',') {
                // 分隔符
                cells.push_back(current);
                current.clear();
                i++;
                continue;
            } else {
                current.push_back(ch);
                i++;
                continue;
            }
        }
    }

    // 最后一个字段
    cells.push_back(current);

    return cells;
}

std::vector<std::string> DataRegistry::ParseJsonStringArray(
    const std::string& encoded_value) {
    const JsonValue parsed = JsonValue::Parse(encoded_value);
    std::vector<std::string> values;
    if (!parsed.IsArray()) {
        return values;
    }
    for (const auto& item : parsed.AsArray()) {
        if (item.IsString()) {
            values.push_back(item.AsString());
        }
    }
    return values;
}

void DataRegistry::RegisterTableSchema(const TableSchema& schema) {
    schemas_[schema.table_id] = schema;
}

void DataRegistry::RegisterExternalIdSet(const std::string& table_id, std::unordered_set<std::string> ids) {
    external_id_sets_[table_id] = std::move(ids);
}

bool DataRegistry::ValidateAllReferences() {
    bool all_valid = true;
    for (const auto& [table_id, schema] : schemas_) {
        const auto& valid_ids = GetExternalIds_(table_id);
        const std::string& path = GetRegisteredPath_(table_id);

        for (const auto& fk : schema.foreign_keys) {
            auto doc = CsvDocument{};
            std::vector<DataIssue> local_issues;
            if (!LoadCsvDocument(path, doc, &local_issues)) {
                continue;
            }

            const auto& target_ids = GetExternalIds_(fk.target_table);
            if (target_ids.empty() && !external_id_sets_.contains(fk.target_table)) {
                continue;
            }

            for (const auto& row : doc.rows) {
                const std::string value = row.Get(fk.source_field);
                if (value.empty()) {
                    if (fk.required) {
                        issues_.push_back(DataTableValidation::MakeError(
                            table_id, path, row.line_number, fk.source_field, value,
                            "外键引用不能为空，期望值来自表 " + fk.target_table));
                        all_valid = false;
                    }
                    continue;
                }

                if (target_ids.contains(value)) {
                    continue;
                }

                auto json_array = ParseJsonStringArray(value);
                if (!json_array.empty()) {
                    bool all_found = true;
                    for (const auto& item : json_array) {
                        if (!target_ids.contains(item)) {
                            all_found = false;
                            issues_.push_back(DataTableValidation::MakeError(
                                table_id, path, row.line_number, fk.source_field, item,
                                "无效的外键引用 '" + item + "'，期望值来自表 " + fk.target_table));
                        }
                    }
                    if (!all_found) {
                        all_valid = false;
                    }
                    continue;
                }

                issues_.push_back(DataTableValidation::MakeError(
                    table_id, path, row.line_number, fk.source_field, value,
                    "无效的外键引用 '" + value + "'，期望值来自表 " + fk.target_table + " 的 " + fk.target_field + " 字段"));
                all_valid = false;
            }
        }
    }
    return all_valid;
}

void DataRegistry::ClearValidationCache() {
    external_id_sets_.clear();
}

std::string DataRegistry::GetRegisteredPath_(const std::string& table_id) const {
    auto it = table_to_path_.find(table_id);
    return it != table_to_path_.end() ? it->second : "";
}

const std::unordered_set<std::string>& DataRegistry::GetExternalIds_(const std::string& table_id) const {
    static const std::unordered_set<std::string> empty{};
    auto it = external_id_sets_.find(table_id);
    return it != external_id_sets_.end() ? it->second : empty;
}

bool DataTableValidation::CheckDuplicateIds(
    const CsvDocument& document,
    const std::string& id_field,
    std::vector<DataIssue>& out_issues,
    const std::string& table_id,
    const std::string& path) {
    std::unordered_set<std::string> seen_ids;
    bool success = true;
    for (const auto& row : document.rows) {
        const std::string id = row.Get(id_field);
        if (id.empty()) {
            continue;
        }
        if (seen_ids.contains(id)) {
            out_issues.push_back(MakeError(
                table_id, path, row.line_number, id_field, id,
                "重复的 ID: '" + id + "'"));
            success = false;
        }
        seen_ids.insert(id);
    }
    return success;
}

bool DataTableValidation::ValidateRow(
    const CsvRow& row,
    const TableSchema& schema,
    const std::unordered_set<std::string>& valid_ids,
    std::vector<DataIssue>& out_issues,
    const std::string& table_id,
    const std::string& path) {
    bool success = true;

    for (const auto& req : schema.required_fields) {
        const std::string value = row.Get(req.field_name);
        if (value.empty()) {
            if (req.required) {
                out_issues.push_back(MakeError(
                    table_id, path, row.line_number, req.field_name, "",
                    "必填字段 '" + req.field_name + "' 不能为空"));
                success = false;
            }
            continue;
        }

        if (!req.enum_values.empty() && !req.enum_values.contains(value)) {
            out_issues.push_back(MakeWarning(
                table_id, path, row.line_number, req.field_name, value,
                "字段值 '" + value + "' 不在枚举值列表中"));
        }
    }

    if (!valid_ids.empty()) {
        const std::string id_field = schema.id_field.empty() ? "Id" : schema.id_field;
        const std::string id_value = row.Get(id_field);
        if (!id_value.empty() && !valid_ids.contains(id_value)) {
            out_issues.push_back(MakeError(
                table_id, path, row.line_number, id_field, id_value,
                "ID '" + id_value + "' 不在有效 ID 集合中"));
            success = false;
        }
    }

    return success;
}

} // namespace CloudSeamanor::infrastructure
