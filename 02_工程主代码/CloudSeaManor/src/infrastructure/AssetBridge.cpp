// ============================================================================
// 【AssetBridge】美术资源桥接器实现
// ============================================================================

#include "CloudSeamanor/AssetBridge.hpp"

#include "CloudSeamanor/Logger.hpp"

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
    // TODO: 实现命名规范检查
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
    result.valid = true;
    // TODO: 验证 PNG 尺寸与 JSON 一致
    return result;
}

AssetBridge::ValidationResult AssetBridge::ValidatePageReferences(
    const UiPageConfig& page_config) {
    ValidationResult result;
    result.valid = true;
    // TODO: 验证页面引用的 sprite_id 是否存在
    (void)page_config;
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
        v.valid = false;
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
