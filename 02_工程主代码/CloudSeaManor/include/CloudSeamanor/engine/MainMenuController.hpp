#pragma once

// ============================================================================
// 【MainMenuController】主菜单控制器
// ============================================================================
// 负责管理主菜单的状态、初始化、布局和渲染。
//
// 主要职责：
// - 管理主菜单可见性、选中项、过渡动画
// - 初始化菜单 UI 元素（标题、按钮、背景）
// - 处理菜单输入（键盘/手柄）
// - 执行菜单动作（开始游戏、继续、设置等）
//
// 设计原则：
// - 单一职责：只负责主菜单相关逻辑
// - 依赖注入：通过构造函数接收所需依赖
// - 无状态存储：UI 状态由 GameApp 持有
//
// 架构决策：
// - 当前版本：作为框架存在，主菜单逻辑仍在 GameApp 中
// - 后续计划：将 MainMenuScreen 嵌套 struct 的成员迁移到此控制器
// ============================================================================

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Event.hpp>

#include <array>
#include <functional>
#include <memory>
#include <string>

namespace CloudSeamanor::engine {

// ============================================================================
// 【MainMenuAction】菜单动作枚举
// ============================================================================
enum class MainMenuAction {
    None = 0,
    StartGame = 1,
    ContinueGame = 2,
    Settings = 3,
    About = 4,
    ExitGame = 5
};

// ============================================================================
// 【MainMenuCallbacks】菜单回调函数
// ============================================================================
struct MainMenuCallbacks {
    std::function<void(const std::string&, float)> push_hint;
    std::function<void()> load_game;
    std::function<void()> exit_game;
};

// ============================================================================
// 【MainMenuController】主菜单控制器类
// ============================================================================
class MainMenuController {
public:
    // ========================================================================
    // 【常量】
    // ========================================================================
    static constexpr int kMainMenuItemCount = 5;

    // ========================================================================
    // 【构造函数】
    // ========================================================================
    MainMenuController();

    // ========================================================================
    // 【初始化】
    // ========================================================================
    // @param font 字体指针
    // @param window_size 窗口大小
    // @param callbacks 回调函数集合
    void Initialize(const sf::Font* font, sf::Vector2u window_size, MainMenuCallbacks callbacks);

    // ========================================================================
    // 【布局更新】
    // ========================================================================
    // @param window_size 窗口大小
    void UpdateLayout(sf::Vector2u window_size);

    // ========================================================================
    // 【输入处理】
    // ========================================================================
    // @param key_event 键盘事件
    // @return 是否有动作需要执行
    bool HandleInput(const sf::Event::KeyPressed& key_event);

    // ========================================================================
    // 【渲染】
    // ========================================================================
    // @param window 渲染窗口
    void Render(sf::RenderWindow& window);

    // ========================================================================
    // 【动作执行】
    // ========================================================================
    void ExecuteAction(MainMenuAction action);

    // ========================================================================
    // 【状态访问器】
    // ========================================================================
    [[nodiscard]] bool IsVisible() const { return visible_; }
    [[nodiscard]] int SelectedIndex() const { return selected_index_; }
    [[nodiscard]] MainMenuAction PendingAction() const { return pending_action_; }
    [[nodiscard]] bool HasSave() const { return has_save_; }

    // ========================================================================
    // 【状态修改器】
    // ========================================================================
    void SetVisible(bool visible) { visible_ = visible; }
    void ClearPendingAction() { pending_action_ = MainMenuAction::None; }

    // ========================================================================
    // 【UI 元素访问器】
    // ========================================================================
    [[nodiscard]] const sf::RectangleShape& Panel() const { return panel_; }
    [[nodiscard]] const sf::Sprite* BackgroundSprite() const { return background_sprite_.get(); }
    [[nodiscard]] const std::array<sf::FloatRect, kMainMenuItemCount>& ButtonRects() const { return button_rects_; }

private:
    // ========================================================================
    // 【内部辅助】
    // ========================================================================
    void InitializeFont_(const sf::Font* font);
    void InitializeLayout_();
    void UpdateHoverAnimation_(float delta_seconds);
    [[nodiscard]] static sf::String MakeUtf8String(const char8_t* literal);

    // ========================================================================
    // 【成员变量】
    // ========================================================================
    MainMenuCallbacks callbacks_;

    bool visible_ = true;
    int selected_index_ = 0;
    bool has_save_ = false;
    MainMenuAction pending_action_ = MainMenuAction::None;
    bool transition_out_ = false;
    float fade_alpha_ = 0.0f;
    float anim_time_ = 0.0f;
    bool background_loaded_ = false;

    std::array<float, kMainMenuItemCount> hover_lerp_{};

    sf::RectangleShape panel_;
    std::unique_ptr<sf::Sprite> background_sprite_;
    std::array<sf::RectangleShape, 4> corner_blocks_;
    std::array<sf::FloatRect, kMainMenuItemCount> button_rects_{};

    std::unique_ptr<sf::Text> title_;
    std::unique_ptr<sf::Text> items_[kMainMenuItemCount];
    std::unique_ptr<sf::Text> save_preview_text_;
    std::unique_ptr<sf::Text> status_text_;
};

}  // namespace CloudSeamanor::engine
