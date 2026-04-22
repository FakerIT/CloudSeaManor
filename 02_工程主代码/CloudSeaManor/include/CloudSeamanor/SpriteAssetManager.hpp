#pragma once

// ============================================================================
// 【SpriteAssetManager】美术资源管理器
// ============================================================================
// Responsibilities:
// - 统一管理所有纹理资源（游戏精灵、UI 图集、字体 atlas）
// - 支持图集（Atlas）+ 元数据（Metadata）方式加载 sprite
// - 与现有 ResourceManager 集成，复用字体/音频管理
// - 提供引用计数、批量预加载、LRU 卸载
//
// 纹理来源:
// - 游戏精灵: assets/sprites/ 下的 PNG 图集
// - UI 图集:  assets/sprites/ui/ 下的 UI 元素合并图
// - 地形瓦片: assets/sprites/tiles/ 下的地形瓦片集
//
// 元数据格式: assets/sprites/*.atlas.json（同名的 JSON 配置文件）
// ============================================================================

#include "CloudSeamanor/ResourceManager.hpp"

#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace CloudSeamanor::infrastructure {

// ============================================================================
// 【SpriteFrame】精灵帧定义
// ============================================================================
// 单帧: 图集内的一个子区域，带唯一 ID
struct SpriteFrame {
    std::string id;           // 全局唯一 ID，如 "player_idle_down_0"
    int x = 0;                // 左上角 X（像素）
    int y = 0;                // 左上角 Y（像素）
    int width = 0;            // 帧宽度
    int height = 0;           // 帧高度
    float pivot_x = 0.5f;     // 旋转/缩放中心 X（0-1，0.5=中心）
    float pivot_y = 1.0f;     // 旋转/缩放中心 Y（0-1，1.0=底部对齐）
    int duration_ms = 100;     // 帧持续时间（ms），用于动画
};

// ============================================================================
// 【SpriteAnimation】精灵动画定义
// ============================================================================
struct SpriteAnimation {
    std::string id;           // 动画 ID，如 "player_walk_down"
    std::string name;         // 显示名，如 "行走·下"
    std::vector<SpriteFrame> frames;  // 帧序列
    bool loop = true;         // 是否循环
    float speed = 1.0f;       // 播放速度倍率
};

// ============================================================================
// 【SpriteMetadata】图集元数据（从 .atlas.json 加载）
// ============================================================================
struct SpriteMetadata {
    std::string atlas_id;         // 图集 ID，如 "player"
    std::string texture_path;      // 纹理路径，如 "assets/sprites/player.png"
    int atlas_width = 0;          // 图集总宽
    int atlas_height = 0;         // 图集总高
    std::vector<SpriteFrame> frames;     // 所有帧定义
    std::vector<SpriteAnimation> animations;  // 所有动画定义
};

// ============================================================================
// 【SpriteAssetDesc】资源描述符（引擎层使用）
// ============================================================================
struct SpriteAssetDesc {
    std::string id;               // 资源 ID
    std::string atlas_id;          // 所属图集
    std::string texture_path;      // 纹理路径
    sf::IntRect region;            // 图集内区域
    sf::Vector2u size;            // 原始尺寸
    sf::Vector2f pivot;            // 支点
    std::string category;          // 分类: "player", "npc", "item", "ui", "tile"
};

// ============================================================================
// 【AtlasLoadConfig】图集加载配置
// ============================================================================
struct AtlasLoadConfig {
    std::string atlas_path;        // .atlas.json 路径
    std::string base_dir;          // 资源根目录（用于解析相对路径）
    bool generate_mipmap = false; // 是否生成 mipmap
    bool smooth = false;           // 是否平滑插值（像素风格用 false）
};

// ============================================================================
// 【SpriteAssetManager】美术资源管理器
// ============================================================================
class SpriteAssetManager {
public:
    explicit SpriteAssetManager(ResourceManager* base_resource_manager);

    // ========================================================================
    // 【生命周期】
    // ========================================================================
    void Shutdown();

    // ========================================================================
    // 【图集加载】
    // ========================================================================

    /**
     * @brief 加载单个图集（含 JSON 元数据 + PNG 纹理）
     * @param config 加载配置
     * @return true 成功
     */
    bool LoadAtlas(const AtlasLoadConfig& config);

    /**
     * @brief 批量加载多个图集
     * @param configs 配置列表
     * @return 成功加载数量
     */
    int LoadAtlases(const std::vector<AtlasLoadConfig>& configs);

    /**
     * @brief 从目录扫描并加载所有图集
     * @param sprites_dir assets/sprites/ 目录路径
     * @return 加载的图集数量
     */
    int ScanAndLoadAtlases(const std::string& sprites_dir);

    /**
     * @brief 卸载指定图集
     */
    void UnloadAtlas(const std::string& atlas_id);

    // ========================================================================
    // 【精灵查询】
    // ========================================================================

    /**
     * @brief 通过全局帧 ID 获取精灵描述
     * @param frame_id 如 "player_idle_down_0"
     * @return 精灵描述（不存在则返回空）
     */
    [[nodiscard]] std::optional<SpriteAssetDesc> GetSprite(const std::string& frame_id) const;

    /**
     * @brief 通过图集 ID + 帧名获取精灵描述
     * @param atlas_id 图集 ID
     * @param frame_name 帧名（不含图集前缀）
     * @return 精灵描述
     */
    [[nodiscard]] std::optional<SpriteAssetDesc> GetSpriteByName(
        const std::string& atlas_id,
        const std::string& frame_name) const;

    /**
     * @brief 获取动画定义
     * @param atlas_id 图集 ID
     * @param anim_id 动画 ID
     * @return 动画定义（不存在则返回空）
     */
    [[nodiscard]] std::optional<SpriteAnimation> GetAnimation(
        const std::string& atlas_id,
        const std::string& anim_id) const;

    /**
     * @brief 获取图集的纹理指针
     */
    [[nodiscard]] const sf::Texture* GetAtlasTexture(const std::string& atlas_id) const;

    /**
     * @brief 获取分类下的所有帧 ID
     */
    [[nodiscard]] std::vector<std::string> GetFramesByCategory(const std::string& category) const;

    // ========================================================================
    // 【元数据访问】
    // ========================================================================

    /**
     * @brief 获取图集元数据
     */
    [[nodiscard]] const SpriteMetadata* GetAtlasMetadata(const std::string& atlas_id) const;

    /**
     * @brief 获取所有已加载的图集 ID
     */
    [[nodiscard]] std::vector<std::string> GetLoadedAtlasIds() const;

    // ========================================================================
    // 【预加载与内存管理】
    // ========================================================================

    /**
     * @brief 预加载一组常用图集（游戏启动时调用）
     * 优先级: player > npcs > items > tiles > ui
     */
    void PreloadCriticalAssets();

    /**
     * @brief 预加载 UI 图集（进入游戏后立即调用）
     */
    void PreloadUiAssets();

    /**
     * @brief 卸载当前场景不需要的图集（按分类优先级）
     * @param keep_categories 保留的分类
     */
    void UnloadUnusedByCategory(const std::unordered_set<std::string>& keep_categories);

    /**
     * @brief 估算当前纹理内存占用（MB）
     */
    [[nodiscard]] float EstimateMemoryUsageMB() const;

    // ========================================================================
    // 【状态】
    // ========================================================================
    [[nodiscard]] bool IsLoaded(const std::string& atlas_id) const;
    [[nodiscard]] std::size_t GetLoadedAtlasCount() const { return atlases_.size(); }

private:
    bool ParseAtlasJson_(const std::string& json_path, SpriteMetadata& out_meta);
    void RegisterAllFrames_(const SpriteMetadata& meta);
    std::string BuildFrameId_(const std::string& atlas_id, const std::string& frame_name) const;

    ResourceManager* base_resource_manager_ = nullptr;

    // 图集元数据
    std::unordered_map<std::string, SpriteMetadata> atlases_;

    // 全局帧 ID → (图集 ID, 帧索引)
    std::unordered_map<std::string, std::pair<std::string, std::size_t>> frame_registry_;

    // 分类 → 帧 ID 列表
    std::unordered_map<std::string, std::vector<std::string>> category_registry_;

    // 引用计数
    std::unordered_map<std::string, int> ref_counts_;

    // 加载失败的图集（避免重复尝试）
    std::unordered_set<std::string> failed_loads_;
};

// ============================================================================
// 【内联工具函数】
// ============================================================================

/**
 * @brief 从帧 ID 解析图集 ID 和帧名
 * @param frame_id 如 "player_idle_down_0"
 * @return (atlas_id, frame_name)
 */
[[nodiscard]] inline std::pair<std::string, std::string> ParseFrameId(const std::string& frame_id) {
    auto sep = frame_id.find('_');
    if (sep == std::string::npos) return {"", frame_id};
    return {frame_id.substr(0, sep), frame_id.substr(sep + 1)};
}

}  // namespace CloudSeamanor::infrastructure
