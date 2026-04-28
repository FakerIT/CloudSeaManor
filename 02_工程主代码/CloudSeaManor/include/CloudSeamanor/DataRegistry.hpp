#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
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

    template <typename T, typename Parser>
    void RegisterCsvTable(
        const std::string& table_id,
        const std::string& relative_path,
        DataTable<T>* target_table,
        Parser parser) {
        csv_table_loaders_.push_back(CsvLoaderRegistration{
            .table_id = table_id,
            .relative_path = relative_path,
            .loader = [target_table, parser](const CsvDocument& document,
                                             std::vector<DataIssue>& issues) {
                std::vector<T> rows;
                rows.reserve(document.rows.size());
                bool success = true;
                for (const auto& row : document.rows) {
                    auto parsed = parser(row, issues);
                    if (!parsed.has_value()) {
                        success = false;
                        continue;
                    }
                    rows.push_back(std::move(*parsed));
                }
                if (success && target_table != nullptr) {
                    target_table->SetRows(std::move(rows));
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
};

} // namespace CloudSeamanor::infrastructure
