#include "CloudSeamanor/infrastructure/SaveSlotManager.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"
#include "CloudSeamanor/infrastructure/ReferenceSaveDataSystem.hpp"

#include <cctype>
#include <exception>
#include <fstream>
#include <sstream>

namespace CloudSeamanor::infrastructure {

namespace {

std::string Trim(const std::string& text) {
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start])) != 0) {
        ++start;
    }
    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    return text.substr(start, end - start);
}

std::string EscapeLinePipes(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char ch : text) {
        out.push_back(ch == '|' ? '/' : ch);
    }
    return out;
}

enum class ImportCompatibilityLevel {
    Low,
    Medium,
    High,
    Critical
};

std::string CompatibilityLevelText(ImportCompatibilityLevel level) {
    switch (level) {
    case ImportCompatibilityLevel::Low: return "LOW";
    case ImportCompatibilityLevel::Medium: return "MEDIUM";
    case ImportCompatibilityLevel::High: return "HIGH";
    case ImportCompatibilityLevel::Critical: return "CRITICAL";
    }
    return "UNKNOWN";
}

ImportCompatibilityLevel AssessPlayerLogRisk(const std::filesystem::path& player_log_path,
                                             std::vector<std::string>* out_hints) {
    if (out_hints != nullptr) {
        out_hints->clear();
    }

    std::ifstream in(player_log_path);
    if (!in.is_open()) {
        if (out_hints != nullptr) {
            out_hints->push_back("未找到 Player.log，按中风险处理并仅导入安全字段。");
        }
        return ImportCompatibilityLevel::Medium;
    }

    bool has_missing_script = false;
    bool has_layout_mismatch = false;
    bool has_unknown_behavior = false;
    std::string line;
    while (std::getline(in, line)) {
        if (line.find("The referenced script") != std::string::npos) {
            has_missing_script = true;
        }
        if (line.find("different serialization layout") != std::string::npos) {
            has_layout_mismatch = true;
        }
        if (line.find("script unknown or not yet loaded") != std::string::npos) {
            has_unknown_behavior = true;
        }
    }

    ImportCompatibilityLevel level = ImportCompatibilityLevel::Low;
    if (has_layout_mismatch) {
        level = ImportCompatibilityLevel::Critical;
    } else if (has_missing_script || has_unknown_behavior) {
        level = ImportCompatibilityLevel::High;
    }

    if (out_hints != nullptr) {
        if (level == ImportCompatibilityLevel::Critical) {
            out_hints->push_back("检测到序列化布局不一致，启用严格降级：仅导入 meta/system_settings/日购记录。");
            out_hints->push_back("跳过角色、库存、装备、复杂对象载荷，避免错误反序列化污染。");
        } else if (level == ImportCompatibilityLevel::High) {
            out_hints->push_back("检测到缺失脚本/未知行为，启用高等级降级：跳过高耦合 payload。");
        } else if (level == ImportCompatibilityLevel::Medium) {
            out_hints->push_back("日志不可用，采用中等级降级策略。");
        } else {
            out_hints->push_back("未发现高风险序列化信号，可导入完整归一化快照。");
        }
    }
    return level;
}

JsonValue BuildDowngradedSnapshot(const JsonValue& normalized, ImportCompatibilityLevel level) {
    if (!normalized.IsObject()) {
        return JsonValue();
    }
    if (level == ImportCompatibilityLevel::Low) {
        return normalized;
    }

    JsonValue safe = JsonValue::Object();
    if (const JsonValue* source = normalized.Get("source_path")) {
        safe.Insert("source_path", *source);
    }
    if (const JsonValue* meta = normalized.Get("meta")) {
        safe.Insert("meta", *meta);
    }
    if (const JsonValue* system_settings = normalized.Get("system_settings")) {
        safe.Insert("system_settings", *system_settings);
    }
    if (const JsonValue* records = normalized.Get("global_daily_shop_records")) {
        safe.Insert("global_daily_shop_records", *records);
    }
    if (const JsonValue* slots = normalized.Get("slots"); slots != nullptr && slots->IsArray()) {
        JsonValue slim_slots = JsonValue::Array();
        for (const auto& slot : slots->AsArray()) {
            if (!slot.IsObject()) {
                continue;
            }
            JsonValue slim = JsonValue::Object();
            if (const JsonValue* slot_index = slot.Get("slot_index")) {
                slim.Insert("slot_index", *slot_index);
            }
            if (const JsonValue* have_save = slot.Get("have_save")) {
                slim.Insert("have_save", *have_save);
            }
            if (const JsonValue* deleted_save = slot.Get("deleted_save")) {
                slim.Insert("deleted_save", *deleted_save);
            }
            slim_slots.PushBack(std::move(slim));
        }
        safe.Insert("slots", std::move(slim_slots));
    }
    return safe;
}

std::string ExtractSeasonFromSlotPayload(const JsonValue& slot_payload) {
    if (!slot_payload.IsObject()) {
        return "Unknown";
    }
    if (const JsonValue* scene = slot_payload.Get("当前场景");
        scene != nullptr && (scene->IsInt() || scene->IsFloat())) {
        const auto scene_id = static_cast<int>(scene->AsIntOrDefault(0));
        return "Scene#" + std::to_string(scene_id);
    }
    return "Unknown";
}

int ExtractDayFromSlotPayload(const JsonValue& slot_payload) {
    if (!slot_payload.IsObject()) {
        return 0;
    }
    if (const JsonValue* day = slot_payload.Get("记录天")) {
        return static_cast<int>(day->AsIntOrDefault(0));
    }
    return 0;
}

}  // namespace

SaveSlotManager::SaveSlotManager(std::filesystem::path root_dir)
    : root_dir_(std::move(root_dir)) {
}

bool SaveSlotManager::IsValidSlot(int slot_index) const {
    return slot_index >= 1 && slot_index <= 3;
}

std::filesystem::path SaveSlotManager::BuildSlotPath(int slot_index) const {
    return root_dir_ / ("save_slot_0" + std::to_string(slot_index) + ".txt");
}

std::filesystem::path SaveSlotManager::BuildMetaPath(int slot_index) const {
    return root_dir_ / ("save_slot_0" + std::to_string(slot_index) + ".meta");
}

void SaveSlotManager::EnsureDirectories() const {
    std::filesystem::create_directories(root_dir_);
}

bool SaveSlotManager::WriteMetadata(int slot_index, const SaveSlotMetadata& metadata) const {
    if (!IsValidSlot(slot_index)) {
        return false;
    }
    EnsureDirectories();
    std::ofstream out(BuildMetaPath(slot_index), std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    out << "display_name=" << metadata.display_name << '\n';
    out << "saved_at=" << metadata.saved_at_text << '\n';
    out << "day=" << metadata.day << '\n';
    out << "season=" << metadata.season_text << '\n';
    out << "thumbnail=" << metadata.thumbnail_path << '\n';
    out << "summary=" << metadata.summary_text << '\n';
    out << "main_plot_stage=" << metadata.main_plot_stage << '\n';
    out << "key_achievement_count=" << metadata.key_achievement_count << '\n';
    out << "last_battle_outcome=" << metadata.last_battle_outcome << '\n';
    return out.good();
}

SaveSlotMetadata SaveSlotManager::ReadMetadata(int slot_index) const {
    SaveSlotMetadata metadata;
    metadata.slot_index = slot_index;
    if (!IsValidSlot(slot_index)) {
        return metadata;
    }
    metadata.exists = std::filesystem::exists(BuildSlotPath(slot_index));

    std::ifstream in(BuildMetaPath(slot_index));
    if (!in.is_open()) {
        return metadata;
    }

    std::string line;
    while (std::getline(in, line)) {
        const std::size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        const std::string key = Trim(line.substr(0, pos));
        const std::string value = Trim(line.substr(pos + 1));
        if (key == "saved_at") {
            metadata.saved_at_text = value;
        } else if (key == "display_name") {
            metadata.display_name = value;
        } else if (key == "day") {
            try {
                metadata.day = std::stoi(value);
            } catch (const std::invalid_argument&) {
                metadata.metadata_corrupted = true;
                metadata.day = 0;
                Logger::Warning(
                    "SaveSlotManager: 元数据解析失败，slot=" + std::to_string(slot_index) +
                    "，field=day，value=" + value + "（invalid_argument）");
            } catch (const std::out_of_range&) {
                metadata.metadata_corrupted = true;
                metadata.day = 0;
                Logger::Warning(
                    "SaveSlotManager: 元数据解析失败，slot=" + std::to_string(slot_index) +
                    "，field=day，value=" + value + "（out_of_range）");
            }
        } else if (key == "season") {
            metadata.season_text = value;
        } else if (key == "thumbnail") {
            metadata.thumbnail_path = value;
        } else if (key == "summary") {
            metadata.summary_text = value;
        } else if (key == "main_plot_stage") {
            try {
                metadata.main_plot_stage = std::stoi(value);
            } catch (const std::exception&) {
                metadata.metadata_corrupted = true;
                metadata.main_plot_stage = 0;
            }
        } else if (key == "key_achievement_count") {
            try {
                metadata.key_achievement_count = std::stoi(value);
            } catch (const std::exception&) {
                metadata.metadata_corrupted = true;
                metadata.key_achievement_count = 0;
            }
        } else if (key == "last_battle_outcome") {
            metadata.last_battle_outcome = value;
        }
    }
    return metadata;
}

std::vector<SaveSlotMetadata> SaveSlotManager::ReadAllMetadata() const {
    std::vector<SaveSlotMetadata> result;
    result.reserve(3);
    std::vector<int> corrupted_slots;
    corrupted_slots.reserve(3);
    for (int slot = 1; slot <= 3; ++slot) {
        SaveSlotMetadata metadata = ReadMetadata(slot);
        if (metadata.metadata_corrupted) {
            corrupted_slots.push_back(slot);
        }
        result.push_back(std::move(metadata));
    }

    if (!corrupted_slots.empty()) {
        std::string summary = "SaveSlotManager: 检测到损坏元数据槽位: ";
        for (std::size_t i = 0; i < corrupted_slots.size(); ++i) {
            if (i > 0) {
                summary += ", ";
            }
            summary += std::to_string(corrupted_slots[i]);
        }
        Logger::Warning(summary);
    }
    return result;
}

bool SaveSlotManager::DeleteSlot(int slot_index) const {
    if (!IsValidSlot(slot_index)) {
        return false;
    }
    std::error_code ec;
    const auto slot_path = BuildSlotPath(slot_index);
    const auto meta_path = BuildMetaPath(slot_index);
    std::filesystem::remove(slot_path, ec);
    ec.clear();
    std::filesystem::remove(meta_path, ec);
    ec.clear();
    const auto thumb_dir = root_dir_ / ("slot_" + std::to_string(slot_index));
    if (std::filesystem::exists(thumb_dir, ec)) {
        std::filesystem::remove_all(thumb_dir, ec);
    }
    return true;
}

bool SaveSlotManager::CopySlot(int from_slot_index, int to_slot_index, bool overwrite_target) const {
    if (!IsValidSlot(from_slot_index) || !IsValidSlot(to_slot_index) || from_slot_index == to_slot_index) {
        return false;
    }
    EnsureDirectories();
    const auto from_slot = BuildSlotPath(from_slot_index);
    const auto to_slot = BuildSlotPath(to_slot_index);
    if (!std::filesystem::exists(from_slot)) {
        return false;
    }
    if (!overwrite_target && std::filesystem::exists(to_slot)) {
        return false;
    }

    std::error_code ec;
    std::filesystem::copy_file(
        from_slot,
        to_slot,
        overwrite_target ? std::filesystem::copy_options::overwrite_existing
                         : std::filesystem::copy_options::none,
        ec);
    if (ec) {
        return false;
    }

    const auto from_meta = ReadMetadata(from_slot_index);
    SaveSlotMetadata to_meta = from_meta;
    to_meta.slot_index = to_slot_index;
    if (to_meta.display_name.empty()) {
        to_meta.display_name = "槽位 " + std::to_string(to_slot_index);
    }

    const auto from_thumb_dir = root_dir_ / ("slot_" + std::to_string(from_slot_index));
    const auto to_thumb_dir = root_dir_ / ("slot_" + std::to_string(to_slot_index));
    if (std::filesystem::exists(from_thumb_dir)) {
        std::filesystem::create_directories(to_thumb_dir);
        const auto from_thumb = from_thumb_dir / "thumbnail.png";
        const auto to_thumb = to_thumb_dir / "thumbnail.png";
        if (std::filesystem::exists(from_thumb)) {
            std::filesystem::copy_file(
                from_thumb,
                to_thumb,
                std::filesystem::copy_options::overwrite_existing,
                ec);
            if (!ec) {
                to_meta.thumbnail_path = to_thumb.generic_string();
            }
        }
    }

    return WriteMetadata(to_slot_index, to_meta);
}

bool SaveSlotManager::RenameSlotDisplayName(int slot_index, const std::string& display_name) const {
    if (!IsValidSlot(slot_index)) {
        return false;
    }
    SaveSlotMetadata metadata = ReadMetadata(slot_index);
    metadata.slot_index = slot_index;
    metadata.display_name = display_name;
    if (metadata.saved_at_text.empty()) {
        metadata.saved_at_text = "N/A";
    }
    return WriteMetadata(slot_index, metadata);
}

bool SaveSlotManager::ImportReferenceSaveToSlot(
    int slot_index,
    const std::filesystem::path& es3_path,
    const std::filesystem::path& player_log_path,
    const std::filesystem::path& normalized_export_dir) const {
    if (!IsValidSlot(slot_index)) {
        Logger::Warning("SaveSlotManager: 导入失败，非法槽位: " + std::to_string(slot_index));
        return false;
    }

    ReferenceSaveDataSystem reference_loader;
    if (!reference_loader.LoadFromFile(es3_path.string())) {
        Logger::Warning("SaveSlotManager: 参考存档解析失败: " + es3_path.string());
        return false;
    }

    const JsonValue normalized = reference_loader.BuildNormalizedSnapshot();
    if (!normalized.IsObject()) {
        Logger::Warning("SaveSlotManager: 归一化失败，导入中止。");
        return false;
    }

    std::filesystem::create_directories(normalized_export_dir);
    const std::filesystem::path normalized_path =
        normalized_export_dir / ("reference_slot_" + std::to_string(slot_index) + ".normalized.json");
    if (!reference_loader.ExportNormalizedSnapshot(normalized_path)) {
        Logger::Warning("SaveSlotManager: 归一化导出失败: " + normalized_path.string());
        return false;
    }

    std::vector<std::string> compatibility_hints;
    const ImportCompatibilityLevel compatibility_level =
        AssessPlayerLogRisk(player_log_path, &compatibility_hints);
    const JsonValue downgraded = BuildDowngradedSnapshot(normalized, compatibility_level);

    // 导出降级快照（用于审计和回溯）
    const std::filesystem::path downgraded_path =
        normalized_export_dir / ("reference_slot_" + std::to_string(slot_index) + ".downgraded.json");
    {
        // 简单复用：创建一个轻量序列化文件，供后续审计
        std::ofstream out(downgraded_path, std::ios::trunc);
        if (!out.is_open()) {
            Logger::Warning("SaveSlotManager: 无法写入降级快照: " + downgraded_path.string());
            return false;
        }
        // 借助 ReferenceSaveDataSystem 的文本导出能力：用临时实例导出不方便，直接写最小结构
        // 这里保留关键摘要，完整结构由 normalized.json 提供。
        out << "{\n";
        out << "  \"compatibility_level\": \"" << CompatibilityLevelText(compatibility_level) << "\",\n";
        out << "  \"note\": \"See normalized snapshot for full data.\"\n";
        out << "}\n";
    }

    // 生成槽位导入摘要文件（放在标准 slot 路径）
    EnsureDirectories();
    std::ofstream slot_out(BuildSlotPath(slot_index), std::ios::trunc);
    if (!slot_out.is_open()) {
        Logger::Warning("SaveSlotManager: 无法写入导入槽位文件。");
        return false;
    }

    slot_out << "reference_import|1\n";
    slot_out << "source_es3|" << EscapeLinePipes(es3_path.string()) << "\n";
    slot_out << "normalized_export|" << EscapeLinePipes(normalized_path.string()) << "\n";
    slot_out << "downgraded_export|" << EscapeLinePipes(downgraded_path.string()) << "\n";
    slot_out << "compatibility_level|" << CompatibilityLevelText(compatibility_level) << "\n";
    for (const auto& hint : compatibility_hints) {
        slot_out << "compatibility_hint|" << EscapeLinePipes(hint) << "\n";
    }
    const auto validation = reference_loader.BuildValidationReport();
    for (const auto& issue : validation) {
        slot_out << "validation|" << EscapeLinePipes(issue) << "\n";
    }
    slot_out.flush();
    if (!slot_out.good()) {
        Logger::Warning("SaveSlotManager: 导入摘要写入失败。");
        return false;
    }

    SaveSlotMetadata metadata;
    metadata.slot_index = slot_index;
    metadata.exists = true;
    metadata.saved_at_text = "REFERENCE_IMPORT";
    metadata.season_text = "Imported(" + CompatibilityLevelText(compatibility_level) + ")";
    metadata.day = 0;
    if (const JsonValue* slots = downgraded.Get("slots");
        slots != nullptr && slots->IsArray()
        && static_cast<std::size_t>(slot_index - 1) < slots->AsArray().size()) {
        const JsonValue& imported_slot = slots->AsArray()[static_cast<std::size_t>(slot_index - 1)];
        if (const JsonValue* payload = imported_slot.Get("save_payload")) {
            metadata.day = ExtractDayFromSlotPayload(*payload);
            metadata.season_text = ExtractSeasonFromSlotPayload(*payload);
        }
    }
    metadata.thumbnail_path = normalized_path.string();
    if (!WriteMetadata(slot_index, metadata)) {
        Logger::Warning("SaveSlotManager: 导入后元数据写入失败。");
        return false;
    }

    Logger::Info(
        "SaveSlotManager: 参考存档导入完成 slot=" + std::to_string(slot_index)
        + ", level=" + CompatibilityLevelText(compatibility_level));
    return true;
}

}  // namespace CloudSeamanor::infrastructure
