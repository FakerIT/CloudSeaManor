#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace CloudSeamanor::infrastructure {

class ResourceManager;

struct DataRegistryEntry {
    std::string category;
    std::string path;
};

enum class DataIssueSeverity {
    Info,
    Warning,
    Error,
};

struct DataIssue {
    DataIssueSeverity severity = DataIssueSeverity::Info;
    std::string table_id;
    std::string path;
    std::size_t line_number = 0;
    std::string field_name;
    std::string bad_value;
    std::string message;
};

struct CsvRow {
    std::size_t line_number = 0;
    std::unordered_map<std::string, std::string> values;

    [[nodiscard]] bool Has(std::string_view key) const;
    [[nodiscard]] std::string Get(std::string_view key, std::string default_value = {}) const;
    [[nodiscard]] int GetInt(std::string_view key, int default_value = 0) const;
    [[nodiscard]] float GetFloat(std::string_view key, float default_value = 0.0f) const;
    [[nodiscard]] bool GetBool(std::string_view key, bool default_value = false) const;
    [[nodiscard]] std::vector<std::string> GetStringList(
        std::string_view key,
        char delimiter = '|') const;
};

struct CsvDocument {
    std::vector<std::string> header;
    std::vector<CsvRow> rows;
};

struct ForeignKeyReference {
    std::string source_table;
    std::string source_field;
    std::string target_table;
    std::string target_field;
    bool required = true;
};

struct FieldRequirement {
    std::string field_name;
    bool required = false;
    std::unordered_set<std::string> enum_values;
};

struct TableSchema {
    std::string table_id;
    std::string id_field = "Id";
    std::vector<FieldRequirement> required_fields;
    std::vector<ForeignKeyReference> foreign_keys;
};

class DataTableValidation {
public:
    static DataIssue MakeError(
        std::string_view table_id,
        std::string_view path,
        std::size_t line,
        std::string_view field,
        std::string_view bad_value,
        std::string_view message) {
        return DataIssue{
            .severity = DataIssueSeverity::Error,
            .table_id = std::string(table_id),
            .path = std::string(path),
            .line_number = line,
            .field_name = std::string(field),
            .bad_value = std::string(bad_value),
            .message = std::string(message),
        };
    }

    static DataIssue MakeWarning(
        std::string_view table_id,
        std::string_view path,
        std::size_t line,
        std::string_view field,
        std::string_view bad_value,
        std::string_view message) {
        return DataIssue{
            .severity = DataIssueSeverity::Warning,
            .table_id = std::string(table_id),
            .path = std::string(path),
            .line_number = line,
            .field_name = std::string(field),
            .bad_value = std::string(bad_value),
            .message = std::string(message),
        };
    }

    static bool ValidateRow(
        const CsvRow& row,
        const TableSchema& schema,
        const std::unordered_set<std::string>& valid_ids,
        std::vector<DataIssue>& out_issues,
        const std::string& table_id,
        const std::string& path);

    static bool CheckDuplicateIds(
        const CsvDocument& document,
        const std::string& id_field,
        std::vector<DataIssue>& out_issues,
        const std::string& table_id,
        const std::string& path);
};

template <typename T>
class DataTable {
public:
    using IdGetter = std::function<std::string(const T&)>;

    explicit DataTable(IdGetter id_getter = {})
        : id_getter_(std::move(id_getter)) {}

    void Clear() {
        rows_.clear();
        id_index_.clear();
    }

    void SetRows(std::vector<T> rows) {
        rows_ = std::move(rows);
        RebuildIndex_();
    }

    void SetIdGetter(IdGetter id_getter) {
        id_getter_ = std::move(id_getter);
        RebuildIndex_();
    }

    [[nodiscard]] const std::vector<T>& Rows() const noexcept { return rows_; }
    [[nodiscard]] std::size_t Size() const noexcept { return rows_.size(); }
    [[nodiscard]] bool Empty() const noexcept { return rows_.empty(); }

    [[nodiscard]] const T* FindById(const std::string& id) const {
        const auto it = id_index_.find(id);
        if (it == id_index_.end() || it->second >= rows_.size()) {
            return nullptr;
        }
        return &rows_[it->second];
    }

    [[nodiscard]] std::unordered_set<std::string> GetAllIds() const {
        std::unordered_set<std::string> ids;
        for (const auto& [id, _] : id_index_) {
            ids.insert(id);
        }
        return ids;
    }

private:
    void RebuildIndex_() {
        id_index_.clear();
        if (!id_getter_) {
            return;
        }
        for (std::size_t index = 0; index < rows_.size(); ++index) {
            id_index_[id_getter_(rows_[index])] = index;
        }
    }

    IdGetter id_getter_;
    std::vector<T> rows_;
    std::unordered_map<std::string, std::size_t> id_index_;
};

class DataRegistry {
public:
    using CsvTableLoader = std::function<bool(
        const CsvDocument&,
        std::vector<DataIssue>&)>;

    void RegisterPath(const std::string& category, const std::string& path);
    void SetDataRoots(std::vector<std::filesystem::path> data_roots);
    void AttachResourceManager(ResourceManager* resource_manager);

    [[nodiscard]] const std::vector<DataRegistryEntry>& Entries() const noexcept { return entries_; }
    [[nodiscard]] const std::vector<DataIssue>& Issues() const noexcept { return issues_; }
    [[nodiscard]] std::vector<std::string> ValidateAndDescribe() const;
    [[nodiscard]] std::filesystem::path ResolveDataPath(const std::string& relative_path) const;
    [[nodiscard]] bool LoadCsvDocument(
        const std::string& relative_path,
        CsvDocument& out_document,
        std::vector<DataIssue>* out_issues = nullptr) const;
    [[nodiscard]] bool LoadAll();

    void RegisterTableSchema(const TableSchema& schema);
    void RegisterExternalIdSet(const std::string& table_id, std::unordered_set<std::string> ids);
    bool ValidateAllReferences();
    void ClearValidationCache();

    template <typename T, typename Parser>
    void RegisterCsvTable(
        const std::string& table_id,
        const std::string& relative_path,
        DataTable<T>* target_table,
        Parser parser) {
        csv_table_loaders_.push_back(CsvLoaderRegistration{
            .table_id = table_id,
            .relative_path = relative_path,
            .loader = [target_table, parser, this](const CsvDocument& document,
                                             std::vector<DataIssue>& issues) {
                std::vector<T> rows;
                rows.reserve(document.rows.size());
                bool success = true;

                if (auto schema_it = schemas_.find(table_id); schema_it != schemas_.end()) {
                    if (!DataTableValidation::CheckDuplicateIds(
                            document, schema_it->second.id_field, issues, table_id,
                            GetRegisteredPath_(table_id))) {
                        success = false;
                    }
                }

                const auto& valid_ids = GetExternalIds_(table_id);

                for (const auto& row : document.rows) {
                    if (!schema_it->second.required_fields.empty() || !valid_ids.empty()) {
                        if (!DataTableValidation::ValidateRow(
                                row, schema_it->second, valid_ids, issues, table_id,
                                GetRegisteredPath_(table_id))) {
                            success = false;
                        }
                    }

                    auto parsed = parser(row, issues);
                    if (!parsed.has_value()) {
                        success = false;
                        continue;
                    }
                    rows.push_back(std::move(*parsed));
                }
                if (success && target_table != nullptr) {
                    target_table->SetRows(std::move(rows));
                    if (auto schema_it2 = schemas_.find(table_id); schema_it2 != schemas_.end()) {
                        external_id_sets_[table_id] = target_table->GetAllIds();
                    }
                }
                return success;
            },
        });
    }

    [[nodiscard]] static std::string Trim(std::string_view text);
    [[nodiscard]] static std::vector<std::string> SplitCsvLine(const std::string& line);
    [[nodiscard]] static std::vector<std::string> ParseJsonStringArray(
        const std::string& encoded_value);

private:
    [[nodiscard]] std::string GetRegisteredPath_(const std::string& table_id) const;
    [[nodiscard]] const std::unordered_set<std::string>& GetExternalIds_(const std::string& table_id) const;

    struct CsvLoaderRegistration {
        std::string table_id;
        std::string relative_path;
        CsvTableLoader loader;
    };

    [[nodiscard]] bool ReadTextFile_(
        const std::filesystem::path& path,
        std::string& out_text) const;

    std::vector<DataRegistryEntry> entries_;
    std::vector<std::filesystem::path> data_roots_;
    std::vector<CsvLoaderRegistration> csv_table_loaders_;
    std::vector<DataIssue> issues_;
    ResourceManager* resource_manager_ = nullptr;

    std::unordered_map<std::string, TableSchema> schemas_;
    std::unordered_map<std::string, std::unordered_set<std::string>> external_id_sets_;
    std::unordered_map<std::string, std::string> table_to_path_;
};

} // namespace CloudSeamanor::infrastructure
