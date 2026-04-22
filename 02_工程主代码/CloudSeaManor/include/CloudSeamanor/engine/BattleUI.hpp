#pragma once

// ============================================================================
// 【BattleUI.hpp】战斗UI渲染
// ============================================================================
// 渲染所有战斗相关的UI元素：血条、能量条、技能按钮、战斗日志、结算面板。
//
// 主要职责：
// - 渲染污染灵体的血条/污染值条
// - 渲染玩家能量条
// - 渲染技能按钮（Q/W/E/R）和冷却状态
// - 渲染灵兽伙伴状态栏
// - 渲染战斗日志面板
// - 渲染战斗结算面板
// - 渲染暂停菜单
//
// 与其他系统的关系：
// - 依赖：BattleField（数据源）、SFML Graphics（渲染）
// - 被依赖：BattleManager（每帧调用 Draw）
// ============================================================================

#include "CloudSeamanor/engine/BattleField.hpp"
#include "CloudSeamanor/engine/BattleEntities.hpp"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <optional>

namespace CloudSeamanor::engine {

namespace detail {
[[nodiscard]] const sf::Font& BattleUiFallbackFont();
}

// ============================================================================
// 【SkillButton】技能按钮
// ============================================================================
struct SkillButton {
    sf::RectangleShape background;   // 背景
    sf::RectangleShape cooldown_overlay; // 冷却遮罩
    sf::Text key_label{detail::BattleUiFallbackFont()};              // 按键标签（Q/W/E/R）
    sf::Text cooldown_label{detail::BattleUiFallbackFont()};         // 冷却倒计时
    sf::Text name_label{detail::BattleUiFallbackFont()};             // 技能名称
    sf::Text energy_cost_label{detail::BattleUiFallbackFont()};      // 能量消耗

    std::string skill_id;           // 关联技能ID
    int slot_index = -1;             // 槽位（0-3）
    bool is_ready = true;            // 是否就绪
    bool is_locked = false;          // 是否未解锁
    float cooldown_remaining = 0.0f; // 当前冷却剩余
    float cooldown_total = 0.0f;      // 总冷却时间
    float energy_cost = 0.0f;        // 能量消耗
    bool is_hovered = false;         // 鼠标悬停
};

// ============================================================================
// 【SpiritHealthBar】灵体血条
// ============================================================================
struct SpiritHealthBar {
    sf::RectangleShape background;
    sf::RectangleShape pollution_fill;  // 污染值条（满=被污染）
    sf::RectangleShape purity_fill;     // 纯净值条（满=已净化）
    sf::Text name_label{detail::BattleUiFallbackFont()};
    sf::Text type_label{detail::BattleUiFallbackFont()};  // 普通/精英/BOSS标签

    std::string spirit_id;
    float bar_width = 120.0f;
    float bar_height = 8.0f;
};

// ============================================================================
// 【BattleUI】战斗UI渲染器
// ============================================================================
class BattleUI {
public:
    // ========================================================================
    // 【初始化】
    // ========================================================================

    /**
     * @brief 初始化所有UI元素
     * @param font 字体引用
     * @param window_width 窗口宽度
     * @param window_height 窗口高度
     */
    void Initialize(const sf::Font& font, unsigned int window_width, unsigned int window_height);

    // ========================================================================
    // 【更新】
    // ========================================================================

    /** 每帧更新UI状态（冷却动画、血条变化等） */
    void Update(float delta_seconds);

    // ========================================================================
    // 【渲染】
    // ========================================================================

    /** 渲染所有UI元素到窗口 */
    void Draw(sf::RenderWindow& window) const;

    // ========================================================================
    // 【输入处理】
    // ========================================================================

    /**
     * @brief 鼠标移动（更新悬停状态）
     */
    void OnMouseMove(float mouse_x, float mouse_y);

    /**
     * @brief 点击技能按钮
     * @return 点击的技能槽位（-1表示未点击技能按钮）
     */
    int OnMouseClick(float mouse_x, float mouse_y);

    /**
     * @brief 按键按下
     * @return 按下的技能槽位（-1表示未按技能键）
     */
    int OnKeyPressed(int key_code); // key_code: sf::Keyboard::Q=0, W=1, E=2, R=3

    // ========================================================================
    // 【数据绑定】
    // ========================================================================

    /** 从战场获取数据更新UI */
    void SyncFromField(const BattleField& field);

    // ========================================================================
    // 【状态】
    // ========================================================================

    /** 显示/隐藏暂停菜单 */
    void SetPauseMenuVisible(bool visible);

    /** 显示结算面板 */
    void ShowResultPanel(const BattleResult& result, bool victory);

    /** 添加日志条目 */
    void AddLogEntry(const std::string& message, bool is_player_action = true);

private:
    // ========================================================================
    // 内部方法
    // ========================================================================

    void UpdateHealthBars_();
    void UpdateSkillButtons_();
    void UpdateEnergyBar_();
    void UpdateLogPanel_();
    void UpdateResultPanel_(float delta_seconds);

    void DrawHealthBars_(sf::RenderWindow& window) const;
    void DrawEnergyBar_(sf::RenderWindow& window) const;
    void DrawSkillButtons_(sf::RenderWindow& window) const;
    void DrawPartnerBars_(sf::RenderWindow& window) const;
    void DrawLogPanel_(sf::RenderWindow& window) const;
    void DrawTopBar_(sf::RenderWindow& window) const;
    void DrawPauseMenu_(sf::RenderWindow& window) const;
    void DrawResultPanel_(sf::RenderWindow& window) const;

    // 工具
    [[nodiscard]] sf::Color GetHealthBarColor_(float pollution_ratio) const;
    [[nodiscard]] std::string GetWeatherText_(CloudSeamanor::domain::CloudState state) const;

    // ========================================================================
    // 成员变量
    // ========================================================================

    const sf::Font* font_ = nullptr;
    unsigned int window_width_ = 1280;
    unsigned int window_height_ = 720;

    // 顶部状态栏
    sf::RectangleShape top_bar_bg_;
    sf::Text weather_text_{detail::BattleUiFallbackFont()};
    sf::Text battle_time_text_{detail::BattleUiFallbackFont()};
    sf::RectangleShape pause_button_;
    sf::RectangleShape retreat_button_;

    // 能量条
    sf::RectangleShape energy_bar_bg_;
    sf::RectangleShape energy_bar_fill_;
    sf::Text energy_text_{detail::BattleUiFallbackFont()};

    // 技能按钮（4个）
    std::vector<SkillButton> skill_buttons_;
    static constexpr int kSkillButtonCount = 4;

    // 灵体血条
    std::vector<SpiritHealthBar> health_bars_;

    // 灵兽伙伴栏
    std::vector<sf::RectangleShape> partner_bars_;
    std::vector<sf::Text> partner_names_;
    std::vector<sf::Text> partner_heart_labels_;

    // 战斗日志
    std::vector<sf::Text> log_lines_;
    static constexpr int kMaxLogLines = 6;
    sf::RectangleShape log_panel_bg_;

    // 暂停菜单
    bool pause_menu_visible_ = false;
    sf::RectangleShape pause_menu_bg_;
    sf::Text pause_title_{detail::BattleUiFallbackFont()};
    sf::RectangleShape resume_button_;
    sf::Text resume_text_{detail::BattleUiFallbackFont()};
    sf::RectangleShape quit_button_;
    sf::Text quit_text_{detail::BattleUiFallbackFont()};

    // 结算面板
    bool result_panel_visible_ = false;
    sf::RectangleShape result_panel_bg_;
    sf::Text result_title_{detail::BattleUiFallbackFont()};
    std::vector<sf::Text> result_lines_;
    float result_display_timer_ = 0.0f;
    static constexpr float kResultDisplayDuration = 3.0f;
    BattleResult cached_result_;
    bool cached_victory_ = false;

    // 工具数据
    static constexpr float kHealthBarYOffset = -40.0f;
};

}  // namespace CloudSeamanor::engine
