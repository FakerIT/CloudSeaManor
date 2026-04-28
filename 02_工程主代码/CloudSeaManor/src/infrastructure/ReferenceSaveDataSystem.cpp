#include "CloudSeamanor/infrastructure/ReferenceSaveDataSystem.hpp"

#include "CloudSeamanor/Logger.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace CloudSeamanor::infrastructure {

namespace {

bool StartsWith_(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

std::string Trim_(const std::string& in) {
    std::size_t b = 0;
    std::size_t e = in.size();
    while (b < e && std::isspace(static_cast<unsigned char>(in[b])) != 0) {
        ++b;
    }
    while (e > b && std::isspace(static_cast<unsigned char>(in[e - 1])) != 0) {
        --e;
    }
    return in.substr(b, e - b);
}

std::string EscapeJsonString_(const std::string& in) {
    std::string out;
    out.reserve(in.size() + 16);
    for (char ch : in) {
        switch (ch) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += ch; break;
        }
    }
    return out;
}

std::string ToJsonText_(const JsonValue& value, int indent) {
    const std::string pad(static_cast<std::size_t>(indent), ' ');
    const std::string pad2(static_cast<std::size_t>(indent + 2), ' ');
    switch (value.Type()) {
    case JsonValueType::Null:
        return "null";
    case JsonValueType::Bool:
        return value.AsBool() ? "true" : "false";
    case JsonValueType::Int:
        return std::to_string(value.AsInt());
    case JsonValueType::Float: {
        std::ostringstream oss;
        oss << value.AsFloat();
        return oss.str();
    }
    case JsonValueType::String:
        return "\"" + EscapeJsonString_(value.AsString()) + "\"";
    case JsonValueType::Array: {
        std::string out = "[";
        const auto& arr = value.AsArray();
        if (!arr.empty()) {
            out += "\n";
            for (std::size_t i = 0; i < arr.size(); ++i) {
                out += pad2 + ToJsonText_(arr[i], indent + 2);
                if (i + 1 < arr.size()) {
                    out += ",";
                }
                out += "\n";
            }
            out += pad;
        }
        out += "]";
        return out;
    }
    case JsonValueType::Object: {
        std::string out = "{";
        const auto& obj = value.AsObject();
        if (!obj.empty()) {
            out += "\n";
            std::size_t i = 0;
            for (const auto& [k, v] : obj) {
                out += pad2 + "\"" + EscapeJsonString_(k) + "\": " + ToJsonText_(v, indent + 2);
                if (i + 1 < obj.size()) {
                    out += ",";
                }
                out += "\n";
                ++i;
            }
            out += pad;
        }
        out += "}";
        return out;
    }
    }
    return "null";
}

}  // namespace

bool ReferenceSaveDataSystem::LoadFromFile(const std::string& file_path) {
    source_path_ = file_path;
    raw_root_ = JsonValue();

    std::ifstream in(file_path, std::ios::binary);
    if (!in.is_open()) {
        Logger::Warning("ReferenceSaveDataSystem: 无法打开文件: " + file_path);
        return false;
    }

    std::ostringstream ss;
    ss << in.rdbuf();
    const std::string text = ss.str();
    if (text.empty()) {
        Logger::Warning("ReferenceSaveDataSystem: 文件为空: " + file_path);
        return false;
    }

    raw_root_ = JsonValue::Parse(text);
    if (!raw_root_.IsObject()) {
        Logger::Warning("ReferenceSaveDataSystem: 顶层不是 JSON 对象: " + file_path);
        return false;
    }

    Logger::Info("ReferenceSaveDataSystem: 加载成功 path=" + file_path);
    return true;
}

JsonValue ReferenceSaveDataSystem::UnwrapEs3TypedNode_(const JsonValue& node) {
    if (node.IsArray()) {
        JsonValue out = JsonValue::Array();
        for (const auto& child : node.AsArray()) {
            out.PushBack(UnwrapEs3TypedNode_(child));
        }
        return out;
    }

    if (!node.IsObject()) {
        return node;
    }

    // ES3 风格：{ "__type": "...", "value": <actual> }
    const JsonValue* type_field = node.Get("__type");
    const JsonValue* value_field = node.Get("value");
    if (type_field != nullptr && type_field->IsString() && value_field != nullptr) {
        return UnwrapEs3TypedNode_(*value_field);
    }

    JsonValue out = JsonValue::Object();
    for (const auto& [k, v] : node.AsObject()) {
        out.Insert(k, UnwrapEs3TypedNode_(v));
    }
    return out;
}

bool ReferenceSaveDataSystem::ParseDailyShopKey_(
    const std::string& raw_key,
    int* out_slot,
    std::string* out_item_name,
    std::string* out_field_name) {
    if (out_slot == nullptr || out_item_name == nullptr || out_field_name == nullptr) {
        return false;
    }

    const std::string prefix = "商店日购_槽";
    if (!StartsWith_(raw_key, prefix)) {
        return false;
    }

    const std::size_t slot_begin = prefix.size();
    const std::size_t slot_end = raw_key.find('_', slot_begin);
    if (slot_end == std::string::npos) {
        return false;
    }

    const std::string slot_token = raw_key.substr(slot_begin, slot_end - slot_begin);
    try {
        *out_slot = std::stoi(slot_token);
    } catch (...) {
        return false;
    }

    const std::size_t field_sep = raw_key.rfind('_');
    if (field_sep == std::string::npos || field_sep <= slot_end + 1) {
        return false;
    }

    *out_item_name = Trim_(raw_key.substr(slot_end + 1, field_sep - slot_end - 1));
    *out_field_name = Trim_(raw_key.substr(field_sep + 1));
    return !out_item_name->empty() && !out_field_name->empty();
}

JsonValue ReferenceSaveDataSystem::BuildNormalizedSnapshot() const {
    if (!raw_root_.IsObject()) {
        return JsonValue();
    }

    const JsonValue unwrapped = UnwrapEs3TypedNode_(raw_root_);
    if (!unwrapped.IsObject()) {
        return JsonValue();
    }

    JsonValue normalized = JsonValue::Object();
    normalized.Insert("source_path", JsonValue(source_path_));

    JsonValue meta = JsonValue::Object();
    JsonValue slots = JsonValue::Array();
    JsonValue global_daily_shop = JsonValue::Array();

    // 先构造 3 个槽位（与参考数据一致），没有则留空对象
    for (int i = 0; i < 3; ++i) {
        JsonValue slot_obj = JsonValue::Object();
        slot_obj.Insert("slot_index", JsonValue(static_cast<std::int64_t>(i)));
        slot_obj.Insert("have_save", JsonValue(false));
        slot_obj.Insert("deleted_save", JsonValue(false));
        slot_obj.Insert("daily_shop", JsonValue::Object());
        slots.PushBack(std::move(slot_obj));
    }

    for (const auto& [key, value] : unwrapped.AsObject()) {
        if (key == "SelectSave") {
            meta.Insert("selected_save", value);
            continue;
        }
        if (key == "语言") {
            meta.Insert("language", value);
            continue;
        }
        if (key == "离线收益_上次时间戳") {
            meta.Insert("offline_last_timestamp", value);
            continue;
        }
        if (key == "SaveS") {
            normalized.Insert("system_settings", value);
            continue;
        }

        if (StartsWith_(key, "HaveSave")) {
            const std::string idx = key.substr(std::string("HaveSave").size());
            try {
                const int slot_index = std::stoi(idx);
                if (slot_index >= 0 && static_cast<std::size_t>(slot_index) < slots.Size()) {
                    JsonValue slot_obj = slots[static_cast<std::size_t>(slot_index)];
                    if (slot_obj.IsObject()) {
                        slot_obj.Insert("have_save", value);
                        // overwrite back: rebuild array entry
                        JsonValue rewritten = JsonValue::Array();
                        for (std::size_t i = 0; i < slots.Size(); ++i) {
                            rewritten.PushBack(i == static_cast<std::size_t>(slot_index) ? slot_obj : slots[i]);
                        }
                        slots = std::move(rewritten);
                    }
                }
            } catch (...) {
            }
            continue;
        }

        if (StartsWith_(key, "DeletedSave")) {
            const std::string idx = key.substr(std::string("DeletedSave").size());
            try {
                const int slot_index = std::stoi(idx);
                if (slot_index >= 0 && static_cast<std::size_t>(slot_index) < slots.Size()) {
                    JsonValue slot_obj = slots[static_cast<std::size_t>(slot_index)];
                    if (slot_obj.IsObject()) {
                        slot_obj.Insert("deleted_save", value);
                        JsonValue rewritten = JsonValue::Array();
                        for (std::size_t i = 0; i < slots.Size(); ++i) {
                            rewritten.PushBack(i == static_cast<std::size_t>(slot_index) ? slot_obj : slots[i]);
                        }
                        slots = std::move(rewritten);
                    }
                }
            } catch (...) {
            }
            continue;
        }

        if (StartsWith_(key, "Save")) {
            // Save0 / Save1 / Save2
            if (key.size() == 5 && std::isdigit(static_cast<unsigned char>(key[4])) != 0) {
                const int slot_index = key[4] - '0';
                if (slot_index >= 0 && static_cast<std::size_t>(slot_index) < slots.Size()) {
                    JsonValue slot_obj = slots[static_cast<std::size_t>(slot_index)];
                    if (slot_obj.IsObject()) {
                        slot_obj.Insert("save_payload", value);
                        JsonValue rewritten = JsonValue::Array();
                        for (std::size_t i = 0; i < slots.Size(); ++i) {
                            rewritten.PushBack(i == static_cast<std::size_t>(slot_index) ? slot_obj : slots[i]);
                        }
                        slots = std::move(rewritten);
                    }
                }
                continue;
            }
        }

        if (StartsWith_(key, "家园存档_新版_")) {
            // 家园存档_新版_0
            const std::string idx = key.substr(std::string("家园存档_新版_").size());
            try {
                const int slot_index = std::stoi(idx);
                if (slot_index >= 0 && static_cast<std::size_t>(slot_index) < slots.Size()) {
                    JsonValue slot_obj = slots[static_cast<std::size_t>(slot_index)];
                    if (slot_obj.IsObject()) {
                        slot_obj.Insert("home_payload", value);
                        JsonValue rewritten = JsonValue::Array();
                        for (std::size_t i = 0; i < slots.Size(); ++i) {
                            rewritten.PushBack(i == static_cast<std::size_t>(slot_index) ? slot_obj : slots[i]);
                        }
                        slots = std::move(rewritten);
                    }
                }
            } catch (...) {
            }
            continue;
        }

        int slot_index = -1;
        std::string item_name;
        std::string field_name;
        if (ParseDailyShopKey_(key, &slot_index, &item_name, &field_name)) {
            JsonValue entry = JsonValue::Object();
            entry.Insert("slot_index", JsonValue(static_cast<std::int64_t>(slot_index)));
            entry.Insert("item_name", JsonValue(item_name));
            entry.Insert("field_name", JsonValue(field_name));
            entry.Insert("value", value);
            global_daily_shop.PushBack(std::move(entry));
            continue;
        }
    }

    normalized.Insert("meta", std::move(meta));
    normalized.Insert("slots", std::move(slots));
    normalized.Insert("global_daily_shop_records", std::move(global_daily_shop));
    return normalized;
}

bool ReferenceSaveDataSystem::ExportNormalizedSnapshot(const std::filesystem::path& output_path) const {
    const JsonValue normalized = BuildNormalizedSnapshot();
    if (!normalized.IsObject()) {
        Logger::Warning("ReferenceSaveDataSystem: 归一化快照无效，导出失败。");
        return false;
    }
    std::error_code ec;
    std::filesystem::create_directories(output_path.parent_path(), ec);
    std::ofstream out(output_path, std::ios::trunc);
    if (!out.is_open()) {
        Logger::Warning("ReferenceSaveDataSystem: 无法写入归一化导出文件: " + output_path.string());
        return false;
    }
    out << ToJsonText_(normalized, 0) << "\n";
    return out.good();
}

std::vector<std::string> ReferenceSaveDataSystem::BuildValidationReport() const {
    std::vector<std::string> report;
    if (!raw_root_.IsObject()) {
        report.push_back("根节点不是对象，无法构建验证报告。");
        return report;
    }

    const JsonValue normalized = BuildNormalizedSnapshot();
    if (!normalized.IsObject()) {
        report.push_back("归一化失败，未生成有效对象。");
        return report;
    }

    const JsonValue* meta = normalized.Get("meta");
    if (meta == nullptr || !meta->IsObject()) {
        report.push_back("缺少 meta 区块。");
    } else {
        if (meta->Get("selected_save") == nullptr) {
            report.push_back("缺少 SelectSave（selected_save）字段。");
        }
        if (meta->Get("language") == nullptr) {
            report.push_back("缺少 语言（language）字段。");
        }
    }

    const JsonValue* slots = normalized.Get("slots");
    if (slots == nullptr || !slots->IsArray()) {
        report.push_back("缺少 slots 数组。");
        return report;
    }

    for (const auto& slot : slots->AsArray()) {
        if (!slot.IsObject()) {
            report.push_back("slots 中存在非对象条目。");
            continue;
        }
        const JsonValue* have_save = slot.Get("have_save");
        const JsonValue* deleted_save = slot.Get("deleted_save");
        if (have_save == nullptr || !have_save->IsBool()) {
            report.push_back("槽位 have_save 缺失或类型不是 bool。");
        }
        if (deleted_save == nullptr || !deleted_save->IsBool()) {
            report.push_back("槽位 deleted_save 缺失或类型不是 bool。");
        }
    }

    if (report.empty()) {
        report.push_back("验证通过：未发现结构性问题。");
    }
    return report;
}

}  // namespace CloudSeamanor::infrastructure

