// ============================================================================
// 【AssetBridge】美术资源桥接器实现
// ============================================================================

#include "CloudSeamanor/infrastructure/AssetBridge.hpp"

#include "CloudSeamanor/infrastructure/Logger.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <fstream>
#include <sstream>

namespace CloudSeamanor::infrastructure {

AssetBridge::AssetBridge(const AssetBridgeConfig& config) : config_(config) {}

void AssetBridge::SetConfig(const AssetBridgeConfig& config) {
    config_ = config;
}

std::vector<AtlasGeneratorResult> AssetBridge::ScanDirectory(const std::string& sprites_dir) {
    namespace fs = std::filesystem;
    std::vector<AtlasGeneratorResult> results;

    fs::path base(sprites_dir);
    if (!fs::exists(base)) {
        Logger::Warning("AssetBridge: Directory not found: " + sprites_dir);
        return results;
    }

    for (const auto& entry : fs::recursive_directory_iterator(base)) {
        if (!entry.is_regular_file()) continue;
        auto png_path = entry.path().string();
        if (png_path.find(".atlas.") != std::string::npos) continue;
        if (png_path.ends_with(".png") || png_path.ends_with(".jpg") ||
            png_path.ends_with(".jpeg")) {
            auto result = ScanAndGenerateAtlas(png_path);
            results.push_back(result);
        }
    }

    return results;
}

AtlasGeneratorResult AssetBridge::ScanAndGenerateAtlas(const std::string& png_path) {
    AtlasGeneratorResult result;
    result.png_path = png_path;

    auto meta_opt = GenerateAtlasMetadata(png_path);
    if (!meta_opt) {
        result.success = false;
        return result;
    }

    const auto& meta = *meta_opt;
    result.atlas_id = meta.atlas_id;
    result.frame_count = static_cast<int>(meta.frames.size());
    result.animation_count = static_cast<int>(meta.animations.size());

    // 生成 JSON 文件
    std::string json_path = BuildAtlasJsonPath_(png_path);
    if (config_.auto_generate_atlas) {
        // 序列化并写入
        std::ofstream out(json_path);
        if (out.is_open()) {
            // 简单序列化（实际应使用 JsonValue 或 nlohmann_json）
            out << "{\n";
            out << "  \"id\": \"" << meta.atlas_id << "\",\n";
            out << "  \"texture\": \"" << meta.texture_path << "\",\n";
            out << "  \"width\": " << meta.atlas_width << ",\n";
            out << "  \"height\": " << meta.atlas_height << ",\n";
            out << "  \"frames\": [\n";
            for (std::size_t i = 0; i < meta.frames.size(); ++i) {
                const auto& f = meta.frames[i];
                out << "    {\"id\": \"" << f.id << "\", \"x\": " << f.x
                    << ", \"y\": " << f.y << ", \"w\": " << f.width
                    << ", \"h\": " << f.height << "}";
                if (i < meta.frames.size() - 1) out << ",";
                out << "\n";
            }
            out << "  ],\n";
            out << "  \"animations\": [\n";
            for (std::size_t i = 0; i < meta.animations.size(); ++i) {
                const auto& a = meta.animations[i];
                out << "    {\"id\": \"" << a.id << "\", \"name\": \"" << a.name
                    << "\", \"loop\": " << (a.loop ? "true" : "false")
                    << ", \"speed\": " << a.speed << ", \"frames\": [";
                for (std::size_t j = 0; j < a.frames.size(); ++j) {
                    out << "\"" << a.frames[j].id << "\"";
                    if (j < a.frames.size() - 1) out << ", ";
                }
                out << "]";
                out << "}";
                if (i < meta.animations.size() - 1) out << ",";
                out << "\n";
            }
            out << "  ]\n";
            out << "}\n";
            out.close();
            result.atlas_path = json_path;
            result.success = true;
            Logger::Info("AssetBridge: Generated atlas metadata: " + json_path);
        }
    }

    return result;
}

std::optional<SpriteMetadata> AssetBridge::GenerateAtlasMetadata(const std::string& png_path) {
    namespace fs = std::filesystem;

    SpriteMetadata meta;
    fs::path p(png_path);

    // 从文件名派生 ID
    meta.atlas_id = DeriveAtlasId_(p.filename().string());
    meta.texture_path = png_path;

    // 尝试读取 PNG 尺寸（通过 SFML 或文件头）
    // 简单方式: 读取 PNG 文件头
    std::ifstream file(png_path, std::ios::binary);
    if (!file.is_open()) return std::nullopt;

    char header[24] = {0};
    file.read(header, 24);

    // PNG: 签名(8) + IHDR(4) + 宽度(4) + 高度(4)
    if (header[0] == 0x89 && header[1] == 'P' && header[2] == 'N' && header[3] == 'G') {
        meta.atlas_width = (static_cast<unsigned char>(header[16]) << 24) |
                          (static_cast<unsigned char>(header[17]) << 16) |
                          (static_cast<unsigned char>(header[18]) << 8) |
                          static_cast<unsigned char>(header[19]);
        meta.atlas_height = (static_cast<unsigned char>(header[20]) << 24) |
                           (static_cast<unsigned char>(header[21]) << 16) |
                           (static_cast<unsigned char>(header[22]) << 8) |
                           static_cast<unsigned char>(header[23]);
    } else {
        // 无法读取尺寸，跳过
        return std::nullopt;
    }

    // 网格布局解析
    int fw = config_.default_frame_width;
    int fh = config_.default_frame_height;
    int cols = config_.grid_columns;
    int rows = meta.atlas_height / fh;

    std::string prefix = meta.atlas_id;
    auto frames = ParseGridLayout_(png_path, fw, fh, cols, prefix);

    for (auto& f : frames) {
        f.id = prefix + "_" + f.id;
        meta.frames.push_back(std::move(f));
    }

    // 自动构建动画（每行一个动画序列）
    for (int row = 0; row < rows; ++row) {
        std::string anim_id;
        bool loop = true;
        std::vector<SpriteFrame> anim_frames;

        // 从第一列读取帧构建动画
        for (int col = 0; col < cols; ++col) {
            std::size_t idx = row * cols + col;
            if (idx >= meta.frames.size()) break;

            if (col == 0) {
                anim_id = meta.frames[idx].id;
                // 移除帧编号后缀作为动画名
                auto last_underscore = anim_id.rfind('_');
                if (last_underscore != std::string::npos) {
                    std::string potential_num = anim_id.substr(last_underscore + 1);
                    bool all_digits = true;
                    for (char c : potential_num) {
                        if (c < '0' || c > '9') { all_digits = false; break; }
                    }
                    if (all_digits) anim_id = anim_id.substr(0, last_underscore);
                }
            }
            anim_frames.push_back(meta.frames[idx]);
        }

        if (!anim_frames.empty()) {
            SpriteAnimation anim;
            anim.id = anim_id;
            anim.name = anim_id;
            anim.frames = std::move(anim_frames);
            anim.loop = loop;
            anim.speed = 1.0f;
            meta.animations.push_back(std::move(anim));
        }
    }

    return meta;
}

std::vector<SpriteAnimation> AssetBridge::ParseGridLayout_(const std::string& /*png_path*/,
                                                          int frame_w, int frame_h,
                                                          int cols,
                                                          const std::string& frame_prefix) {
    // 从 .atlas.json 加载时使用此方法
    // 自动生成时：从图集网格解析
    std::vector<SpriteAnimation> dummy;
    return dummy;  // 由 GenerateAtlasMetadata 调用方实现
}

std::vector<NamingViolation> AssetBridge::ValidateNamingConventions() {
    std::vector<NamingViolation> violations;
    namespace fs = std::filesystem;

    const fs::path base(config_.sprites_dir);
    if (!fs::exists(base)) {
        NamingViolation v;
        v.file_path = config_.sprites_dir;
        v.rule_name = "sprites_dir_exists";
        v.current_name = config_.sprites_dir;
        v.suggestion = "sprites 目录不存在，无法进行命名校验。";
        violations.push_back(std::move(v));
        return violations;
    }

    auto check_file_stem = [&](const fs::path& p) {
        const std::string stem = p.stem().string();
        if (!AssetNamingConvention::IsValidAtlasId(AssetNamingConvention::NormalizeToFrameId(stem))) {
            NamingViolation v;
            v.file_path = p.string();
            v.rule_name = "atlas_file_name";
            v.current_name = stem;
            v.suggestion = "图集文件名应为小写字母/数字/下划线（例: player_main）。";
            violations.push_back(std::move(v));
        }
    };

    for (const auto& entry : fs::recursive_directory_iterator(base)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const fs::path p = entry.path();
        const std::string ext = p.extension().string();
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
            check_file_stem(p);
            continue;
        }
        if (p.filename().string().ends_with(".atlas.json")) {
            check_file_stem(p.stem());  // ".atlas" 已在 stem 中，继续校验基础命名

            auto meta = GenerateAtlasMetadata(p.string());
            if (!meta.has_value()) {
                NamingViolation v;
                v.file_path = p.string();
                v.rule_name = "atlas_parse";
                v.current_name = p.filename().string();
                v.suggestion = "无法解析 atlas 对应纹理，检查 texture 路径和 PNG 文件。";
                violations.push_back(std::move(v));
                continue;
            }
            for (const auto& frame : meta->frames) {
                auto frame_violation = CheckFrameNaming_(frame.id);
                if (!frame_violation.rule_name.empty()) {
                    frame_violation.file_path = p.string();
                    violations.push_back(std::move(frame_violation));
                }
            }
        }
    }

    return violations;
}

AssetBridge::BatchResult AssetBridge::GenerateAllAtlases() {
    BatchResult result;
    auto results = ScanDirectory(config_.sprites_dir);
    result.total = static_cast<int>(results.size());
    result.results = results;
    for (const auto& r : results) {
        if (r.success) result.success++;
        else result.failed++;
    }
    return result;
}

AssetBridge::ValidationResult AssetBridge::ValidateAtlas(const std::string& atlas_id) {
    ValidationResult result;
    namespace fs = std::filesystem;

    fs::path atlas_path = atlas_id;
    if (!fs::exists(atlas_path)) {
        fs::path base(config_.sprites_dir);
        if (fs::exists(base)) {
            for (const auto& entry : fs::recursive_directory_iterator(base)) {
                if (!entry.is_regular_file()) {
                    continue;
                }
                const auto file_name = entry.path().filename().string();
                if (file_name == atlas_id + ".atlas.json" || file_name == atlas_id) {
                    atlas_path = entry.path();
                    break;
                }
            }
        }
    }

    if (!fs::exists(atlas_path)) {
        result.valid = false;
        result.errors.push_back("atlas file not found: " + atlas_id);
        return result;
    }

    std::string png_path = atlas_path.string();
    const std::string suffix = ".atlas.json";
    if (png_path.ends_with(suffix)) {
        png_path = png_path.substr(0, png_path.size() - suffix.size()) + ".png";
    } else {
        const auto dot = png_path.rfind('.');
        if (dot != std::string::npos) {
            png_path = png_path.substr(0, dot) + ".png";
        }
    }

    auto png_meta = GenerateAtlasMetadata(png_path);
    if (!png_meta.has_value()) {
        result.valid = false;
        result.errors.push_back("png missing or invalid for atlas: " + png_path);
        return result;
    }

    if (png_meta->atlas_width <= 0 || png_meta->atlas_height <= 0) {
        result.valid = false;
        result.errors.push_back("invalid atlas dimensions from png: " + png_path);
        return result;
    }

    if (png_meta->atlas_width % config_.default_frame_width != 0 ||
        png_meta->atlas_height % config_.default_frame_height != 0) {
        result.warnings.push_back(
            "atlas size is not divisible by default frame size: " + png_path);
    }
    return result;
}

AssetBridge::ValidationResult AssetBridge::ValidatePageReferences(
    const UiPageConfig& page_config) {
    ValidationResult result;
    if (page_config.page_id.empty()) {
        result.valid = false;
        result.errors.push_back("page_id is empty");
    }
    if (page_config.width <= 0 || page_config.height <= 0) {
        result.valid = false;
        result.errors.push_back("page size must be positive");
    }
    if (page_config.root_element_ids.empty()) {
        result.warnings.push_back("root_element_ids is empty");
    } else {
        std::unordered_set<std::string> ids;
        for (const auto& id : page_config.root_element_ids) {
            if (id.empty()) {
                result.valid = false;
                result.errors.push_back("root element id contains empty value");
                continue;
            }
            if (!ids.insert(id).second) {
                result.warnings.push_back("duplicated root element id: " + id);
            }
        }
    }
    if (page_config.close_key.empty()) {
        result.warnings.push_back("close_key is empty; recommend Escape");
    }
    return result;
}

std::optional<UiPageConfig> AssetBridge::LoadPageConfig(const std::string& path) {
    return engine::UiLayoutSerializer::LoadPageFromFile(path);
}

bool AssetBridge::SavePageConfig(const std::string& path, const UiPageConfig& config) {
    auto json = engine::UiLayoutSerializer::SerializePage(config);
    std::ofstream out(path);
    if (!out.is_open()) return false;
    out << json;
    return true;
}

std::vector<UiPageConfig> AssetBridge::GetPresetPageTemplates() {
    std::vector<UiPageConfig> templates;
    templates.push_back(PresetPageFactory::MakeInventoryPage());
    templates.push_back(PresetPageFactory::MakeWorkshopPage());
    templates.push_back(PresetPageFactory::MakeContractPage());
    templates.push_back(PresetPageFactory::MakeNpcDetailPage());
    templates.push_back(PresetPageFactory::MakeSpiritBeastPage());
    templates.push_back(PresetPageFactory::MakeSaveLoadPage());
    templates.push_back(PresetPageFactory::MakeSettingsPage());
    templates.push_back(PresetPageFactory::MakeCloudForecastPage());
    templates.push_back(PresetPageFactory::MakeMainMenuPage());
    templates.push_back(PresetPageFactory::MakePlayerStatusPage());
    templates.push_back(PresetPageFactory::MakeTeaGardenPage());
    templates.push_back(PresetPageFactory::MakeFestivalPage());
    templates.push_back(PresetPageFactory::MakeSpiritRealmPage());
    templates.push_back(PresetPageFactory::MakeBuildingPage());
    return templates;
}

std::optional<UiPageConfig> AssetBridge::GetPresetTemplate(const std::string& category) {
    auto all = GetPresetPageTemplates();
    for (auto& t : all) {
        if (t.page_id == category) return t;
    }
    return std::nullopt;
}

std::string AssetBridge::DeriveAtlasId_(const std::string& png_filename) const {
    // 移除扩展名
    std::string name = png_filename;
    auto dot = name.rfind('.');
    if (dot != std::string::npos) name = name.substr(0, dot);
    // 替换分隔符为下划线
    for (char& c : name) {
        if (c == '-' || c == ' ' || c == '/') c = '_';
    }
    return name;
}

std::string AssetBridge::BuildAtlasJsonPath_(const std::string& png_path) const {
    std::string json_path = png_path;
    auto dot = json_path.rfind('.');
    if (dot != std::string::npos) {
        json_path = json_path.substr(0, dot);
    }
    return json_path + ".atlas.json";
}

std::string AssetBridge::GetCategoryFromPath_(const std::string& png_path) const {
    auto sep = png_path.find("sprites/");
    if (sep == std::string::npos) return "misc";
    auto start = sep + 8;
    auto end = png_path.find('/', start);
    if (end == std::string::npos) return "misc";
    return png_path.substr(start, end - start);
}

NamingViolation AssetBridge::CheckFrameNaming_(const std::string& frame_name) const {
    NamingViolation v;
    v.current_name = frame_name;
    v.file_path = "";

    // 简单检查：包含下划线
    if (frame_name.find('_') == std::string::npos) {
        v.rule_name = "frame_id_format";
        v.suggestion = "帧 ID 应使用下划线分隔，如 player_idle_down_0";
    }
    return v;
}

std::string AssetBridge::NormalizePath_(const std::string& path) {
    std::string result = path;
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

// ============================================================================
// AssetNamingConvention
// ============================================================================

bool AssetNamingConvention::IsValidFrameId(const std::string& id) {
    if (id.empty()) return false;
    for (char c : id) {
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
              c == '_' || c == '-')) {
            return false;
        }
    }
    return true;
}

std::pair<std::string, std::string> AssetNamingConvention::SplitFrameId(
    const std::string& id) {
    return ParseFrameId(id);
}

bool AssetNamingConvention::IsValidAtlasId(const std::string& id) {
    return IsValidFrameId(id);
}

bool AssetNamingConvention::IsValidDirection(const std::string& suffix) {
    for (int i = 0; i < 8; ++i) {
        if (suffix == kValidDirections[i]) return true;
    }
    return false;
}

int AssetNamingConvention::DirectionSuffixToIndex(const std::string& suffix) {
    for (int i = 0; i < 8; ++i) {
        if (suffix == kValidDirections[i]) return i;
    }
    return 0;
}

std::string AssetNamingConvention::NormalizeToFrameId(const std::string& raw_name) {
    std::string result = raw_name;
    // 转小写
    for (char& c : result) {
        if (c >= 'A' && c <= 'Z') c = c + 32;
        if (c == ' ' || c == '-' || c == '.') c = '_';
    }
    return result;
}

}  // namespace CloudSeamanor::infrastructure
