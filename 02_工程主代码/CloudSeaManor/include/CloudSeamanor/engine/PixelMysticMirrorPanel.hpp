#pragma once

// ============================================================================
// 【PixelMysticMirrorPanel】观云镜UI面板
// ============================================================================

#include <string>
#include <vector>
#include <functional>

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

namespace CloudSeamanor::engine {

// ============================================================================
// 【MysticMirrorUI】观云镜UI管理器（单例）
// ============================================================================
class MysticMirrorUI {
public:
    static MysticMirrorUI& Instance();

    // 初始化
    void Initialize();

    // 显示/隐藏
    void Show();
    void Hide();
    [[nodiscard]] bool IsVisible() const;

    // 更新
    void Update(float delta_seconds);

    // 渲染
    void Render(sf::RenderWindow& window);

    // 输入处理
    bool HandleInput(const sf::Event& event);

private:
    MysticMirrorUI() = default;
    ~MysticMirrorUI() = default;
    MysticMirrorUI(const MysticMirrorUI&) = delete;
    MysticMirrorUI& operator=(const MysticMirrorUI&) = delete;

    MysticMirrorUI* panel_ = nullptr;  // 内部面板指针（可扩展）

    bool is_visible_ = false;
    float animation_time_ = 0.0f;
};

// ============================================================================
// 【PixelMysticMirrorPanel】观云镜面板组件
// ============================================================================
class PixelMysticMirrorPanel {
public:
    static constexpr float kWidth = 400.0f;
    static constexpr float kHeight = 480.0f;
    static constexpr int kMargin = 20;
    static constexpr int kLineHeight = 24;

    PixelMysticMirrorPanel();

    // 初始化
    void Initialize();

    // 显示/隐藏
    void Show();
    void Hide();
    [[nodiscard]] bool IsVisible() const { return is_visible_; }

    // 更新
    void Update(float delta_seconds);

    // 渲染
    void Render(sf::RenderWindow& window);

    // 输入处理
    bool HandleInput(const sf::Event& event);

    // 设置内容
    void SetFortuneText(const std::string& text);
    void SetWeatherForecast(const std::string& today, const std::string& tomorrow, const std::string& day_after);
    void SetRecipeText(const std::string& text);
    void SetRecipeIngredients(const std::string& text);
    void SetCloudDensity(float density);
    void SetAuraStatus(const std::string& status);

private:
    void UpdateLayout_();
    void RenderBackground_(sf::RenderWindow& window);
    void RenderContent_(sf::RenderWindow& window);
    void RenderTitle_(sf::RenderWindow& window);
    void RenderFortuneSection_(sf::RenderWindow& window);
    void RenderWeatherSection_(sf::RenderWindow& window);
    void RenderRecipeSection_(sf::RenderWindow& window);
    void RenderCloudDensity_(sf::RenderWindow& window);
    void RenderFooter_(sf::RenderWindow& window);

    sf::Vector2f GetTextPosition_(int line) const;

    // 位置和状态
    float x_ = 0.0f;
    float y_ = 0.0f;
    bool is_visible_ = false;
    bool is_closing_ = false;
    float close_animation_ = 0.0f;

    // 内容
    std::string fortune_text_;
    std::string weather_today_;
    std::string weather_tomorrow_;
    std::string weather_day_after_;
    std::string recipe_text_;
    std::string recipe_ingredients_;
    float cloud_density_ = 0.0f;
    std::string aura_status_;

    // 样式
    static constexpr sf::Color kBackgroundColor = sf::Color(20, 25, 40, 230);
    static constexpr sf::Color kBorderColor = sf::Color(100, 140, 180, 255);
    static constexpr sf::Color kTitleColor = sf::Color(200, 220, 255, 255);
    static constexpr sf::Color kTextColor = sf::Color(180, 200, 220, 255);
    static constexpr sf::Color kAccentColor = sf::Color(100, 180, 255, 255);
    static constexpr sf::Color kHintColor = sf::Color(150, 170, 190, 200);
};

}  // namespace CloudSeamanor::engine
