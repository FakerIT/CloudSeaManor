#pragma once

// ============================================================================
// 【PixelFestivalMiniGamePanel】节日小游戏面板
// ============================================================================
// 展示和控制节日小游戏的界面。
// 支持多种小游戏类型的渲染。
// ============================================================================

#include "CloudSeamanor/engine/PixelUiPanel.hpp"
#include "CloudSeamanor/domain/FestivalMiniGameSystem.hpp"

#include <string>
#include <vector>

namespace CloudSeamanor::engine {

// ============================================================================
// 【PixelFestivalMiniGamePanel】节日小游戏面板类
// ============================================================================
class PixelFestivalMiniGamePanel : public PixelUiPanel {
public:
    PixelFestivalMiniGamePanel();

    // ========================================================================
    // 【配置】
    // ========================================================================
    void SetFontRenderer(const PixelFontRenderer* renderer);

    // ========================================================================
    // 【游戏控制】
    // ========================================================================
    void StartGame(domain::MiniGameType type);
    void Update(float delta_seconds);
    bool HandleInput(int key);
    bool HandleClick(float x, float y);

    // ========================================================================
    // 【状态查询】
    // ========================================================================
    [[nodiscard]] bool IsGameRunning() const;
    [[nodiscard]] bool IsGameEnded() const;
    [[nodiscard]] const domain::GameResult& GetResult() const;

private:
    // ========================================================================
    // 【渲染】
    // ========================================================================
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;

    // 渲染游戏标题
    void RenderGameHeader(sf::RenderWindow& window, const sf::FloatRect& inner_rect, float x, float y);

    // 渲染计时器
    void RenderTimer(sf::RenderWindow& window, float x, float y, float time_left, float max_time);

    // 渲染分数
    void RenderScore(sf::RenderWindow& window, float x, float y, int score);

    // 渲染吃月饼游戏
    void RenderCatchMooncakeGame(sf::RenderWindow& window, const sf::FloatRect& inner_rect);

    // 渲染猜花游戏
    void RenderGuessFlowerGame(sf::RenderWindow& window, const sf::FloatRect& inner_rect);

    // 渲染放风筝游戏
    void RenderFlyKiteGame(sf::RenderWindow& window, const sf::FloatRect& inner_rect);

    // 渲染采茶游戏
    void RenderHarvestTeaGame(sf::RenderWindow& window, const sf::FloatRect& inner_rect);

    // 渲染茶艺表演游戏
    void RenderTeaCeremonyGame(sf::RenderWindow& window, const sf::FloatRect& inner_rect);

    // 渲染结果
    void RenderResult(sf::RenderWindow& window, const sf::FloatRect& inner_rect);

    // ========================================================================
    // 【常量】
    // ========================================================================
    static constexpr float kGameAreaPadding = 20.0f;
    static constexpr float kInfoHeight = 60.0f;
    static constexpr float kResultPanelWidth = 300.0f;
    static constexpr float kResultPanelHeight = 200.0f;

    // ========================================================================
    // 【成员】
    // ========================================================================
    const PixelFontRenderer* m_font_renderer = nullptr;
    domain::MiniGameType current_game_ = domain::MiniGameType::None;
    bool game_running_ = false;
    bool game_ended_ = false;
    domain::GameResult result_{};

    // 按钮状态
    bool start_button_hovered_ = false;
    bool continue_button_hovered_ = false;
};

}  // namespace CloudSeamanor::engine
