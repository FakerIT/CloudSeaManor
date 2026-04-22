// ============================================================================
// 【SpriteAssetManager】美术资源管理器实现
// ============================================================================

#include "CloudSeamanor/SpriteAssetManager.hpp"

#include "CloudSeamanor/JsonValue.hpp"
#include "CloudSeamanor/Logger.hpp"
#include "CloudSeamanor/ResourceManager.hpp"

#include <fstream>
#include <sstream>

namespace {

// 读取整个文件到字符串
std::optional<std::string> ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return std::nullopt;
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

}  // namespace

namespace CloudSeamanor::infrastructure {

SpriteAssetManager::SpriteAssetManager(ResourceManager* base_resource_manager)
    : base_resource_manager_(base_resource_manager) {}

void SpriteAssetManager::Shutdown() {
    atlases_.clear();
    frame_registry_.clear();
    category_registry_.clear();
    ref_counts_.clear();
    failed_loads_.clear();
}

bool SpriteAssetManager::LoadAtlas(const AtlasLoadConfig& config) {
    if (atlases_.count(config.atlas_path)) {
        // 已加载，增加引用计数
        ref_counts_[config.atlas_path]++;
        return true;
    }

    if (failed_loads_.count(config.atlas_path)) {
        return false;
    }

    // 解析 JSON 元数据
    SpriteMetadata meta;
    if (!ParseAtlasJson_(config.atlas_path, meta)) {
        failed_loads_.insert(config.atlas_path);
        return false;
    }

    // 加载纹理
    std::string texture_path = config.base_dir.empty()
        ? meta.texture_path
        : config.base_dir + "/" + meta.texture_path;

    // 尝试加载纹理（复用 ResourceManager）
    if (base_resource_manager_) {
        if (!base_resource_manager_->LoadTexture(meta.atlas_id, texture_path)) {
            infrastructure::Logger::Error("SpriteAssetManager: Failed to load texture: " + texture_path);
            failed_loads_.insert(config.atlas_path);
            return false;
        }
    }

    // 注册所有帧
    RegisterAllFrames_(meta);

    // 存储元数据
    atlases_[config.atlas_path] = std::move(meta);
    ref_counts_[config.atlas_path] = 1;

    infrastructure::Logger::Info("SpriteAssetManager: Loaded atlas '" + config.atlas_path +
                                   "' with " + std::to_string(atlases_[config.atlas_path].frames.size()) +
                                   " frames");
    return true;
}

int SpriteAssetManager::LoadAtlases(const std::vector<AtlasLoadConfig>& configs) {
    int success = 0;
    for (const auto& cfg : configs) {
        if (LoadAtlas(cfg)) success++;
    }
    return success;
}

int SpriteAssetManager::ScanAndLoadAtlases(const std::string& sprites_dir) {
    namespace fs = std::filesystem;
    std::vector<AtlasLoadConfig> configs;

    fs::path base(sprites_dir);
    if (!fs::exists(base)) {
        infrastructure::Logger::Warning("SpriteAssetManager: Directory not found: " + sprites_dir);
        return 0;
    }

    // 扫描所有 .atlas.json 文件
    for (const auto& entry : fs::recursive_directory_iterator(base)) {
        if (!entry.is_regular_file()) continue;
        auto path = entry.path().string();
        if (path.ends_with(".atlas.json")) {
            AtlasLoadConfig cfg;
            cfg.atlas_path = path;
            cfg.base_dir = base.parent_path().string();
            configs.push_back(cfg);
        }
    }

    return LoadAtlases(configs);
}

void SpriteAssetManager::UnloadAtlas(const std::string& atlas_id) {
    auto it = atlases_.find(atlas_id);
    if (it == atlases_.end()) return;

    ref_counts_[atlas_id]--;
    if (ref_counts_[atlas_id] <= 0) {
        // 卸载纹理
        if (base_resource_manager_) {
            base_resource_manager_->UnloadTexture(atlas_id);
        }

        // 清理注册表
        for (auto& [frame_id, pair] : frame_registry_) {
            if (pair.first == atlas_id) {
                frame_id.clear();  // 标记为无效
            }
        }

        atlases_.erase(it);
        ref_counts_.erase(atlas_id);
    }
}

std::optional<SpriteAssetDesc> SpriteAssetManager::GetSprite(const std::string& frame_id) const {
    auto it = frame_registry_.find(frame_id);
    if (it == frame_registry_.end()) return std::nullopt;

    const auto& [atlas_id, frame_index] = it->second;
    auto atlas_it = atlases_.find(atlas_id);
    if (atlas_it == atlases_.end()) return std::nullopt;

    const auto& atlas = atlas_it->second;
    if (frame_index >= atlas.frames.size()) return std::nullopt;

    const auto& frame = atlas.frames[frame_index];

    SpriteAssetDesc desc;
    desc.id = frame_id;
    desc.atlas_id = atlas_id;
    desc.texture_path = atlas.texture_path;
    desc.region = sf::IntRect(frame.x, frame.y, frame.width, frame.height);
    desc.size = sf::Vector2u(static_cast<unsigned>(frame.width),
                               static_cast<unsigned>(frame.height));
    desc.pivot = sf::Vector2f(frame.pivot_x, frame.pivot_y);
    return desc;
}

std::optional<SpriteAssetDesc> SpriteAssetManager::GetSpriteByName(
    const std::string& atlas_id,
    const std::string& frame_name) const {

    std::string frame_id = BuildFrameId_(atlas_id, frame_name);
    return GetSprite(frame_id);
}

std::optional<SpriteAnimation> SpriteAssetManager::GetAnimation(
    const std::string& atlas_id,
    const std::string& anim_id) const {

    auto atlas_it = atlases_.find(atlas_id);
    if (atlas_it == atlases_.end()) return std::nullopt;

    const auto& animations = atlas_it->second.animations;
    for (const auto& anim : animations) {
        if (anim.id == anim_id) return anim;
    }
    return std::nullopt;
}

const sf::Texture* SpriteAssetManager::GetAtlasTexture(const std::string& atlas_id) const {
    if (!base_resource_manager_) return nullptr;
    return base_resource_manager_->GetTexture(atlas_id);
}

std::vector<std::string> SpriteAssetManager::GetFramesByCategory(
    const std::string& category) const {
    auto it = category_registry_.find(category);
    if (it == category_registry_.end()) return {};
    return it->second;
}

const SpriteMetadata* SpriteAssetManager::GetAtlasMetadata(
    const std::string& atlas_id) const {
    auto it = atlases_.find(atlas_id);
    if (it == atlases_.end()) return nullptr;
    return &it->second;
}

std::vector<std::string> SpriteAssetManager::GetLoadedAtlasIds() const {
    std::vector<std::string> ids;
    for (const auto& [id, _] : atlases_) {
        ids.push_back(id);
    }
    return ids;
}

void SpriteAssetManager::PreloadCriticalAssets() {
    // 按优先级加载关键资源
    std::vector<AtlasLoadConfig> configs = {
        {"assets/sprites/player_main.atlas.json"},    // 玩家
        {"assets/sprites/npc_villagers.atlas.json"},   // NPC
        {"assets/sprites/items_crop.atlas.json"},     // 作物物品
        {"assets/sprites/tiles_farm.atlas.json"},     // 农场地形
    };
    LoadAtlases(configs);
}

void SpriteAssetManager::PreloadUiAssets() {
    std::vector<AtlasLoadConfig> configs = {
        {"assets/sprites/ui/ui_main.atlas.json"},
        {"assets/sprites/ui/ui_icons.atlas.json"},
        {"assets/sprites/ui/ui_borders.atlas.json"},
    };
    LoadAtlases(configs);
}

void SpriteAssetManager::UnloadUnusedByCategory(
    const std::unordered_set<std::string>& keep_categories) {

    for (const auto& [atlas_id, _] : atlases_) {
        bool should_keep = false;
        for (const auto& cat : keep_categories) {
            if (atlas_id.find(cat) != std::string::npos) {
                should_keep = true;
                break;
            }
        }
        if (!should_keep) {
            UnloadAtlas(atlas_id);
        }
    }
}

float SpriteAssetManager::EstimateMemoryUsageMB() const {
    float total = 0.0f;
    for (const auto& [id, atlas] : atlases_) {
        // 估算: 宽 × 高 × 4 bytes (RGBA)
        total += static_cast<float>(atlas.atlas_width * atlas.atlas_height * 4) / (1024.0f * 1024.0f);
    }
    return total;
}

bool SpriteAssetManager::IsLoaded(const std::string& atlas_id) const {
    return atlases_.count(atlas_id) > 0;
}

bool SpriteAssetManager::ParseAtlasJson_(const std::string& json_path,
                                        SpriteMetadata& out_meta) {
    auto content = ReadFile(json_path);
    if (!content) {
        infrastructure::Logger::Error("SpriteAssetManager: Cannot read atlas file: " + json_path);
        return false;
    }

    auto json = JsonValue::Parse(*content);
    if (!json || !json->IsObject()) {
        infrastructure::Logger::Error("SpriteAssetManager: Invalid JSON in: " + json_path);
        return false;
    }

    const auto& obj = json->AsObject();

    // 解析基础字段
    out_meta.atlas_id = obj.GetString("id", "unknown");
    out_meta.texture_path = obj.GetString("texture", "");
    out_meta.atlas_width = obj.GetInt("width", 0);
    out_meta.atlas_height = obj.GetInt("height", 0);

    // 解析帧数组
    if (auto* frames = obj.GetArray("frames")) {
        for (std::size_t i = 0; i < frames->Size(); ++i) {
            const auto& frame_obj = (*frames)[i]->AsObject();
            SpriteFrame frame;
            frame.id = frame_obj.GetString("id", "");
            frame.x = frame_obj.GetInt("x", 0);
            frame.y = frame_obj.GetInt("y", 0);
            frame.width = frame_obj.GetInt("w", 0);
            frame.height = frame_obj.GetInt("h", 0);
            frame.pivot_x = frame_obj.GetFloat("pivotX", 0.5f);
            frame.pivot_y = frame_obj.GetFloat("pivotY", 1.0f);
            frame.duration_ms = frame_obj.GetInt("duration", 100);
            out_meta.frames.push_back(frame);
        }
    }

    // 解析动画数组
    if (auto* animations = obj.GetArray("animations")) {
        for (std::size_t i = 0; i < animations->Size(); ++i) {
            const auto& anim_obj = (*animations)[i]->AsObject();
            SpriteAnimation anim;
            anim.id = anim_obj.GetString("id", "");
            anim.name = anim_obj.GetString("name", anim.id);
            anim.loop = anim_obj.GetBool("loop", true);
            anim.speed = anim_obj.GetFloat("speed", 1.0f);

            if (auto* anim_frames = anim_obj.GetArray("frames")) {
                for (std::size_t j = 0; j < anim_frames->Size(); ++j) {
                    const auto& af = (*anim_frames)[j];
                    if (af->IsString()) {
                        // 引用帧 ID
                        SpriteFrame f;
                        f.id = af->AsString();
                        anim.frames.push_back(f);
                    } else if (af->IsObject()) {
                        // 内联帧定义
                        SpriteFrame f;
                        f.id = af->AsObject().GetString("id", "");
                        f.duration_ms = af->AsObject().GetInt("duration", 100);
                        anim.frames.push_back(f);
                    }
                }
            }
            out_meta.animations.push_back(std::move(anim));
        }
    }

    return true;
}

void SpriteAssetManager::RegisterAllFrames_(const SpriteMetadata& meta) {
    for (std::size_t i = 0; i < meta.frames.size(); ++i) {
        const auto& frame = meta.frames[i];
        std::string frame_id = BuildFrameId_(meta.atlas_id, frame.id);
        frame_registry_[frame_id] = {meta.atlas_id, i};

        // 从帧 ID 提取分类
        auto cat_start = frame_id.find('_');
        if (cat_start != std::string::npos) {
            std::string category = frame_id.substr(0, cat_start);
            category_registry_[category].push_back(frame_id);
        }
    }
}

std::string SpriteAssetManager::BuildFrameId_(const std::string& atlas_id,
                                             const std::string& frame_name) const {
    // 如果帧名已包含图集前缀，跳过
    if (frame_name.find(atlas_id) == 0) {
        return frame_name;
    }
    return atlas_id + "_" + frame_name;
}

}  // namespace CloudSeamanor::infrastructure
