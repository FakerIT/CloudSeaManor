#include "CloudSeamanor/UiLayoutSystem.hpp"

#include "CloudSeamanor/JsonValue.hpp"
#include "CloudSeamanor/Logger.hpp"

#include <fstream>

namespace CloudSeamanor::engine {

namespace {

std::string JsonEscape_(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (const char c : s) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"':  out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:   out += c; break;
        }
    }
    return out;
}

} // namespace

std::string UiLayoutSerializer::SerializePage(const UiPageConfig& config) {
    // 仅覆盖当前测试与运行时最常用的页头字段；元素细节由后续 UI 编辑器再扩展。
    std::string json;
    json.reserve(512);
    json += "{\n";
    json += "  \"page_id\": \"" + JsonEscape_(config.page_id) + "\",\n";
    json += "  \"title\": \"" + JsonEscape_(config.title) + "\",\n";
    json += "  \"width\": " + std::to_string(config.width) + ",\n";
    json += "  \"height\": " + std::to_string(config.height) + ",\n";
    json += "  \"modal\": " + std::string(config.modal ? "true" : "false") + ",\n";
    json += "  \"pause_game\": " + std::string(config.pause_game ? "true" : "false") + ",\n";
    json += "  \"root_element_ids\": [";
    for (std::size_t i = 0; i < config.root_element_ids.size(); ++i) {
        if (i != 0) json += ", ";
        json += "\"" + JsonEscape_(config.root_element_ids[i]) + "\"";
    }
    json += "]\n";
    json += "}\n";
    return json;
}

bool UiLayoutSerializer::WritePageToFile(const std::string& path, const UiPageConfig& config) {
    std::ofstream out(path);
    if (!out.is_open()) {
        infrastructure::Logger::Warning("UiLayoutSerializer: 无法写入文件: " + path);
        return false;
    }
    out << SerializePage(config);
    return true;
}

std::optional<UiPageConfig> UiLayoutSerializer::ParsePage(const std::string& json) {
    const auto root = infrastructure::JsonValue::Parse(json);
    if (!root.IsObject()) {
        return std::nullopt;
    }

    UiPageConfig cfg;

    if (const auto* v = root.Get("page_id"); v && v->IsString()) cfg.page_id = v->AsString();
    if (const auto* v = root.Get("title"); v && v->IsString()) cfg.title = v->AsString();
    if (const auto* v = root.Get("width"); v && (v->IsInt() || v->IsFloat())) cfg.width = static_cast<int>(v->AsIntOrDefault(cfg.width));
    if (const auto* v = root.Get("height"); v && (v->IsInt() || v->IsFloat())) cfg.height = static_cast<int>(v->AsIntOrDefault(cfg.height));
    if (const auto* v = root.Get("modal"); v && v->IsBool()) cfg.modal = v->AsBool();
    if (const auto* v = root.Get("pause_game"); v && v->IsBool()) cfg.pause_game = v->AsBool();

    if (const auto* v = root.Get("root_element_ids"); v && v->IsArray()) {
        cfg.root_element_ids.clear();
        cfg.root_element_ids.reserve(v->Size());
        for (std::size_t i = 0; i < v->Size(); ++i) {
            const auto& item = (*v)[i];
            if (item.IsString()) {
                cfg.root_element_ids.push_back(item.AsString());
            }
        }
    }

    if (cfg.page_id.empty()) {
        return std::nullopt;
    }
    return cfg;
}

std::optional<UiPageConfig> UiLayoutSerializer::LoadPageFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        infrastructure::Logger::Warning("UiLayoutSerializer: 无法打开文件: " + path);
        return std::nullopt;
    }
    const std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return ParsePage(content);
}

std::vector<UiPageConfig> UiLayoutSerializer::ScanPagesFromDirectory(const std::string& /*dir*/) {
    // 运行时扫描由上层的文件系统工具/AssetBridge 负责；这里保持最小实现，避免在 core 引入平台相关遍历逻辑。
    return {};
}

UiPageConfig UiLayoutSerializer::CreateEmptyPage(const std::string& page_id) {
    UiPageConfig cfg;
    cfg.page_id = page_id;
    cfg.title = page_id;
    cfg.width = 480;
    cfg.height = 360;
    cfg.modal = false;
    cfg.pause_game = false;
    return cfg;
}

std::string UiLayoutSerializer::ExportPageAsJson(const UiPage* page) {
    (void)page;
    // 当前未暴露 UiPageConfig 访问器时，导出能力保持占位。
    return "{}\n";
}

} // namespace CloudSeamanor::engine

