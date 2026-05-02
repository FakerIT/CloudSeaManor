#pragma once

#include "CloudSeamanor/engine/GameLoopCoordinator.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <memory>
#include <sstream>
#include <vector>
#include <string>

namespace CloudSeamanor::engine {

// ============================================================================
// 【LoopDebugPanel】循环性能调试面板
// ============================================================================
class LoopDebugPanel {
public:
    // ========================================================================
    // 【构造函数】
    // ========================================================================
    LoopDebugPanel();

    // ========================================================================
    // 【初始化】
    // ========================================================================
    void Initialize(const sf::Font& font);

    // ========================================================================
    // 【更新】
    // ========================================================================
    void Update(const GameLoopCoordinator& coordinator);

    // ========================================================================
    // 【渲染】
    // ========================================================================
    void Render(sf::RenderWindow& window);

    // ========================================================================
    // 【显示控制】
    // ========================================================================
    void SetVisible(bool visible) { is_visible_ = visible; }
    [[nodiscard]] bool IsVisible() const { return is_visible_; }

    void Toggle() { is_visible_ = !is_visible_; }

private:
    void RebuildTexts_(const sf::Font& font);
    void UpdateStats_();
    void UpdateSystemStats_();

    bool is_visible_ = false;
    bool needs_rebuild_ = true;
    const sf::Font* font_ = nullptr;

    // 面板背景
    sf::RectangleShape background_;
    sf::RectangleShape header_bar_;

    // 标题
    std::unique_ptr<sf::Text> title_text_;

    // 阶段统计文本
    std::vector<std::unique_ptr<sf::Text>> phase_texts_;

    // 分隔线
    sf::RectangleShape separator_;

    // 系统统计文本
    std::vector<std::unique_ptr<sf::Text>> system_texts_;

    // 底部统计
    std::unique_ptr<sf::Text> total_frame_text_;
    std::unique_ptr<sf::Text> busy_frame_text_;

    // 面板尺寸
    static constexpr float kPanelWidth = 320.0f;
    static constexpr float kPanelHeight = 400.0f;
    static constexpr float kPadding = 12.0f;
    static constexpr float kLineHeight = 14.0f;
    static constexpr float kHeaderHeight = 24.0f;
};

} // namespace CloudSeamanor::engine
