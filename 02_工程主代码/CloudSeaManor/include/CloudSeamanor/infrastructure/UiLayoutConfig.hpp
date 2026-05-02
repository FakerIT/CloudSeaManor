#pragma once

// ============================================================================
// 【UiLayoutConfig】UI 布局配置加载器
// ============================================================================
// 从 configs/ui_layout.json 读取 UI 面板和文本的布局参数。
//
// 主要职责：
// - 解析 JSON 格式的 UI 布局配置文件
// - 提供面板位置、大小、颜色等参数的查询接口
// - 文件缺失或解析失败时返回内置默认值
//
// 设计原则：
// - 纯数据加载，不依赖 SFML（颜色用 uint32_t 表示）
// - 一次加载，全局使用
// - 默认值与当前硬编码值一致，确保无配置文件时行为不变
// ============================================================================

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace CloudSeamanor::infrastructure {

struct PanelLayout {
    std::array<float, 2> position{0.0f, 0.0f};
    std::array<float, 2> size{0.0f, 0.0f};
    std::uint32_t fill_color = 0x000000FF;
    std::uint32_t outline_color = 0xFFFFFFFF;
    float outline_thickness = 0.0f;
    float alpha = 255.0f;
};

struct TextStyleLayout {
    std::array<float, 2> position{0.0f, 0.0f};
    unsigned int font_size = 16;
    std::uint32_t color = 0xFFFFFFFF;
    float outline_thickness = 0.0f;
    std::uint32_t outline_color = 0x000000FF;
};

struct CloudColorSet {
    std::uint32_t background = 0x000000FF;
    std::uint32_t aura = 0x00000000;
};

struct UiLayoutData {
    std::unordered_map<std::string, PanelLayout> panels;
    std::unordered_map<std::string, TextStyleLayout> texts;
    std::unordered_map<std::string, CloudColorSet> cloud_colors;
    std::unordered_map<std::string, std::uint32_t> semantic_colors;
    std::unordered_map<std::string, float> semantic_numbers;
    std::string primary_font;
    std::string fallback_font;
};

class UiLayoutConfig {
public:
    /**
     * @brief 从 JSON 文件加载 UI 布局配置。
     * @param path 配置文件路径（如 "configs/ui_layout.json"）。
     * @return 加载成功返回 true。
     */
    bool LoadFromFile(const std::string& path);

    /**
     * @brief 获取已加载的布局数据。
     */
    [[nodiscard]] const UiLayoutData& Data() const noexcept { return data_; }

    /**
     * @brief 获取面板布局，缺失时返回默认值。
     */
    [[nodiscard]] PanelLayout GetPanel(const std::string& name) const;

    /**
     * @brief 获取文本样式，缺失时返回默认值。
     */
    [[nodiscard]] TextStyleLayout GetText(const std::string& name) const;

    /**
     * @brief 获取云海颜色集，缺失时返回默认值。
     */
    [[nodiscard]] CloudColorSet GetCloudColor(const std::string& name) const;

    /**
     * @brief 获取通用语义色，缺失时返回 fallback。
     */
    [[nodiscard]] std::uint32_t GetSemanticColor(
        const std::string& name,
        std::uint32_t fallback) const;

    /**
     * @brief 获取通用语义数值，缺失时返回 fallback。
     */
    [[nodiscard]] float GetSemanticNumber(
        const std::string& name,
        float fallback) const;

    /**
     * @brief 是否已成功加载配置。
     */
    [[nodiscard]] bool IsLoaded() const noexcept { return loaded_; }

    /**
     * @brief 获取内置默认布局数据。
     */
    [[nodiscard]] static UiLayoutData GetDefaults();

private:
    UiLayoutData data_;
    bool loaded_ = false;
};

/**
 * @brief 将十六进制颜色字符串解析为 RGBA uint32_t。
 * @param hex 支持 "#RRGGBB" 和 "#RRGGBBAA" 格式。
 * @return RGBA 打包的 uint32_t（R 在高位，A 在低位）。
 */
[[nodiscard]] std::uint32_t ParseHexColor(const std::string& hex);

}  // namespace CloudSeamanor::infrastructure
