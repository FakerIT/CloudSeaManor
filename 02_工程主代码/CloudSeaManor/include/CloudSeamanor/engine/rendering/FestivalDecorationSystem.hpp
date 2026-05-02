#pragma once

// ============================================================================
// 【FestivalDecorationSystem】节日装饰系统
// ============================================================================
// 数据驱动的节日视觉装饰渲染系统，支持 JSON 配置节日装饰元素。
//
// 主要职责：
// - 从 JSON 配置加载节日装饰定义
// - 根据当前节日 ID 生成对应的 SFML 绘制对象
// - 支持多种装饰类型：横幅、花瓣、灯笼、月亮、山峰等
//
// 设计原则：
// - 装饰对象按需创建，由调用方负责生命周期
// - 支持动画参数（振幅、频率等）
// - 配置文件路径：assets/data/FestivalDecorations.json
// ============================================================================

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/System/Vector2.hpp>

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace CloudSeamanor::engine::rendering {

// ============================================================================
// 【DecorationType】装饰类型枚举
// ============================================================================
enum class DecorationType {
    Banner,      // 横幅
    Particles,   // 粒子（花瓣/星星等）
    Moon,        // 月亮
    Lantern,     // 灯笼
    Peak,        // 山峰
    Wave,        // 波浪
    Mist,        // 雾气
    Overlay,     // 全屏覆盖层
    Steam,       // 蒸汽
    Bundle       // 捆束
};

// ============================================================================
// 【DecorationElement】单个装饰元素定义
// ============================================================================
struct DecorationElement {
    DecorationType type = DecorationType::Banner;
    std::vector<std::string> festival_ids;  // 适用的节日 ID 列表
    float position_x = 0.0f;
    float position_y = 0.0f;
    float width = 100.0f;
    float height = 20.0f;
    float radius = 10.0f;           // 圆形装饰用
    float point_count = 6;          // 多边形点数

    // 颜色 (RGBA)
    std::uint8_t fill_r = 255;
    std::uint8_t fill_g = 200;
    std::uint8_t fill_b = 100;
    std::uint8_t fill_a = 255;
    std::uint8_t outline_r = 255;
    std::uint8_t outline_g = 200;
    std::uint8_t outline_b = 100;
    std::uint8_t outline_a = 255;
    float outline_thickness = 2.0f;

    // 动画参数
    float anim_amplitude = 0.0f;   // 振幅（像素）
    float anim_frequency = 1.0f;     // 频率（弧度/秒）
    float anim_phase_offset = 0.0f; // 相位偏移
};

// ============================================================================
// 【FestivalDecorationSystem】节日装饰系统类
// ============================================================================
class FestivalDecorationSystem {
public:
    // ========================================================================
    // 【构造函数】
    // ========================================================================
    FestivalDecorationSystem() = default;

    // ========================================================================
    // 【LoadFromFile】从 JSON 配置文件加载装饰定义
    // ========================================================================
    // @param config_path 配置文件路径（相对于游戏工作目录）
    // @return true 表示加载成功
    bool LoadFromFile(const std::string& config_path);

    // ========================================================================
    // 【GetDecorations】获取指定节日的装饰元素
    // ========================================================================
    // @param festival_id 节日 ID
    // @param session_time 当前会话时间（用于动画计算）
    // @return 装饰对象指针向量（调用方负责删除）
    [[nodiscard]] std::vector<std::unique_ptr<sf::Drawable>> GetDecorations(
        const std::string& festival_id,
        float session_time) const;

    // ========================================================================
    // 【HasFestival】检查节日 ID 是否已配置
    // ========================================================================
    [[nodiscard]] bool HasFestival(const std::string& festival_id) const;

    // ========================================================================
    // 【GetSupportedFestivals】获取所有支持的节日 ID
    // ========================================================================
    [[nodiscard]] std::vector<std::string> GetSupportedFestivals() const;

private:
    // ========================================================================
    // 【CreateDrawable_】根据装饰元素类型创建 SFML 绘制对象
    // ========================================================================
    [[nodiscard]] std::unique_ptr<sf::Drawable> CreateDrawable_(
        const DecorationElement& element,
        float session_time) const;

    // 装饰元素列表
    std::vector<DecorationElement> decorations_;
};

}  // namespace CloudSeamanor::engine::rendering
