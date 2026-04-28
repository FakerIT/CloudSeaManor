#pragma once

// ============================================================================
// 【ResourceManager】资源管理器
// ============================================================================
// 统一管理游戏中所有运行时资源的加载、缓存和卸载。
//
// 主要职责：
// - 纹理、字体的加载与缓存（按 ID 访问，不重复加载）
// - 字体搜索链（系统字体 → 项目资源 → 默认兜底）
// - 资源组预加载与释放
// - 调试统计信息输出
//
// 设计原则：
// - 资源 ID 与文件路径分离，访问时只传 ID
// - 重复请求同一资源返回缓存，不重新加载
// - 资源缺失返回默认占位符，不崩溃
// - 热路径中禁止调用 Load（资源在初始化阶段预加载）
//
// 使用示例：
// @code
// ResourceManager rm;
// rm.LoadFont("chinese_main", "C:/Windows/Fonts/SourceHanSansCN-Normal.ttf");
// rm.LoadFont("chinese_fallback", "assets/fonts/msyh.ttc");
// auto& font = rm.GetFont("chinese_main");  // 返回引用
// @endcode
// ============================================================================

#include "CloudSeamanor/Result.hpp"
#include "CloudSeamanor/Logger.hpp"

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::infrastructure {

// ============================================================================
// 【ResourceStats】资源统计信息
// ============================================================================
struct ResourceStats {
    std::size_t texture_count = 0;
    std::size_t font_count = 0;
    std::size_t sound_buffer_count = 0;
    std::size_t total_texture_memory_bytes = 0;
    std::size_t total_font_memory_bytes = 0;
    std::size_t total_sound_memory_bytes = 0;
};

// ============================================================================
// 【ResourceManager】资源管理器类
// ============================================================================
class ResourceManager {
public:
    // ========================================================================
    // 【构造函数】
    // ========================================================================
    ResourceManager() = default;

    // ========================================================================
    // 【纹理管理】
    // ========================================================================

    /**
     * @brief 加载纹理（不重复加载同名 ID）。
     * @param id 资源唯一标识。
     * @param path 文件路径。
     * @return true 表示加载成功。
     */
    bool LoadTexture(const std::string& id, const std::string& path);
    [[nodiscard]] CloudSeamanor::Result<void> LoadTextureResult(const std::string& id, const std::string& path);

    /**
     * @brief 获取纹理引用。资源不存在时记录警告并返回默认占位符。
     */
    [[nodiscard]] sf::Texture& GetTexture(const std::string& id);

    /**
     * @brief 检查纹理是否已加载。
     */
    [[nodiscard]] bool HasTexture(const std::string& id) const;

    /**
     * @brief 获取已加载纹理数量。
     */
    [[nodiscard]] std::size_t GetTextureCount() const noexcept { return textures_.size(); }

    // ========================================================================
    // 【引用计数】用于精细化资源释放
    // ========================================================================

    /**
     * @brief 增加指定资源的引用计数（Acquire）。
     * @param id 资源 ID。
     * @note 已在 Load* 阶段设为 1，首次 Acquire 将其升为 2。
     */
    void Acquire(const std::string& id);

    /**
     * @brief 减少指定资源的引用计数（Release）。
     * @param id 资源 ID。
     * @note 计数降至 0 时资源进入"未使用"状态，可由 ReleaseUnused 释放。
     */
    void Release(const std::string& id);

    /**
     * @brief 卸载指定纹理。
     */
    void UnloadTexture(const std::string& id);

    // ========================================================================
    // 【字体管理】
    // ========================================================================

    /**
     * @brief 加载字体（不重复加载同名 ID）。
     * @param id 资源唯一标识。
     * @param path 文件路径。
     * @return true 表示加载成功。
     */
    bool LoadFont(const std::string& id, const std::string& path);
    [[nodiscard]] CloudSeamanor::Result<void> LoadFontResult(const std::string& id, const std::string& path);

    /**
     * @brief 获取字体引用。资源不存在时记录警告并返回默认占位符。
     */
    [[nodiscard]] sf::Font& GetFont(const std::string& id);

    /**
     * @brief 检查字体是否已加载。
     */
    [[nodiscard]] bool HasFont(const std::string& id) const;

    /**
     * @brief 获取已加载字体数量。
     */
    [[nodiscard]] std::size_t GetFontCount() const noexcept { return fonts_.size(); }

    /**
     * @brief 卸载指定字体。
     */
    void UnloadFont(const std::string& id);

    // ========================================================================
    // 【音频缓冲管理】
    // ========================================================================
    bool LoadSoundBuffer(const std::string& id, const std::string& path);
    [[nodiscard]] CloudSeamanor::Result<void> LoadSoundBufferResult(const std::string& id, const std::string& path);
    [[nodiscard]] sf::SoundBuffer& GetSoundBuffer(const std::string& id);
    [[nodiscard]] bool HasSoundBuffer(const std::string& id) const;
    void UnloadSoundBuffer(const std::string& id);
    [[nodiscard]] std::size_t GetSoundBufferCount() const noexcept { return sound_buffers_.size(); }

    // ========================================================================
    // 【文本资源管理】（JSON/CSV/脚本文本等）
    // ========================================================================
    /**
     * @brief 加载文本资源（不重复加载同名 ID）。
     * @param id 资源唯一标识。
     * @param path 文件路径。
     * @return true 表示加载成功。
     */
    bool LoadText(const std::string& id, const std::string& path);
    [[nodiscard]] CloudSeamanor::Result<void> LoadTextResult(const std::string& id, const std::string& path);

    /**
     * @brief 获取文本内容。资源不存在时记录警告并返回空字符串引用。
     */
    [[nodiscard]] const std::string& GetText(const std::string& id);

    /**
     * @brief 检查文本是否已加载。
     */
    [[nodiscard]] bool HasText(const std::string& id) const;

    /**
     * @brief 设置数据根目录搜索链，供数据表/脚本资源解析使用。
     */
    void SetDataRoots(std::vector<std::filesystem::path> data_roots);

    /**
     * @brief 根据相对路径解析数据资源的实际路径。
     */
    [[nodiscard]] std::filesystem::path ResolveDataPath(const std::string& relative_path) const;

    /**
     * @brief 通过数据根目录解析并加载文本资源。
     */
    [[nodiscard]] CloudSeamanor::Result<void> LoadResolvedText(
        const std::string& id,
        const std::string& relative_path);

    /**
     * @brief 卸载指定文本资源。
     */
    void UnloadText(const std::string& id);

    // ========================================================================
    // 【字体自动搜索】
    // ========================================================================

    /**
     * @brief 搜索并加载最佳可用字体。
     *
     * 按顺序尝试以下路径，找到第一个成功的即返回：
     * 1. SourceHanSansCN-Normal.ttf（思源黑体）
     * 2. simhei.ttf（黑体）
     * 3. simsun.ttc（宋体）
     * 4. msyh.ttc（雅黑）
     * 5. 项目 assets/fonts/ 下的对应文件
     * 6. arial.ttf（英文兜底）
     *
     * @param id 资源唯一标识。
     * @return true 表示找到并加载成功。
     */
    bool LoadBestAvailableFont(const std::string& id);

    /**
     * @brief 获取主字体引用（按 best_main_font_id 查找）。
     *
     * 如果 best_main_font_id 未设置，触发自动搜索。
     * 搜索成功后将 ID 保存为 best_main_font_id。
     */
    [[nodiscard]] sf::Font& GetMainFont();

    /**
     * @brief 检查主字体是否已加载。
     */
    [[nodiscard]] bool HasMainFont() const noexcept { return !best_main_font_id_.empty() && HasFont(best_main_font_id_); }

    /**
     * @brief 强制使用指定 ID 字体作为主字体（跳过搜索）。
     */
    void SetMainFontId(const std::string& id) { best_main_font_id_ = id; }

    // ========================================================================
    // 【批量操作】
    // ========================================================================

    /**
     * @brief 按资源组批量预加载。
     * @param bundle_name 资源组名称（决定搜索路径）。
     * @note 目前支持 "core"（基础核心）和默认（通用）两种组。
     */
    void PreloadBundle(const std::string& bundle_name);

    /**
     * @brief 释放所有未使用的资源（引用计数为 0 的资源）。
     */
    void ReleaseUnused();

    /**
     * @brief 释放所有资源。
     */
    void ReleaseAll();

    // ========================================================================
    // 【统计】
    // ========================================================================

    /**
     * @brief 输出当前资源缓存统计信息到日志。
     */
    void PrintStats() const;

    /**
     * @brief 获取资源统计信息。
     */
    [[nodiscard]] ResourceStats GetStats() const;

private:
    // ========================================================================
    // 【内部辅助】
    // ========================================================================

    // 字体搜索路径列表
    static std::vector<std::pair<std::string, std::string>> GetSystemFontSearchPaths_();

    static std::vector<std::pair<std::string, std::string>> GetProjectFontSearchPaths_();

    static std::vector<std::pair<std::string, std::string>> BuildFontCandidates_();

    // 资源引用计数
    std::unordered_map<std::string, int> ref_counts_;

    // 纹理缓存
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> textures_;

    // 字体缓存
    std::unordered_map<std::string, std::unique_ptr<sf::Font>> fonts_;

    // 音效缓冲缓存
    std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> sound_buffers_;

    // 文本缓存（JSON/CSV等）
    std::unordered_map<std::string, std::string> texts_;

    // 默认占位符
    std::unique_ptr<sf::Texture> default_texture_;
    std::unique_ptr<sf::Font> default_font_;
    std::unique_ptr<sf::SoundBuffer> default_sound_buffer_;
    std::string default_text_;

    // 已选定的主字体 ID
    std::string best_main_font_id_;

    // 数据搜索根目录（支持 MOD 覆盖链）
    std::vector<std::filesystem::path> data_roots_;
};

}  // namespace CloudSeamanor::infrastructure
