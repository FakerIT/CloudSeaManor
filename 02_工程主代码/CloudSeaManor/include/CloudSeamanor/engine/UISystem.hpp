#pragma once

// ============================================================================
// 【UISystem】UI 管理器
// ============================================================================
// 统一管理游戏所有 UI 面板和文本的初始化、更新、渲染。
//
// 主要职责：
// - 初始化所有 UI 面板（位置、大小、颜色、描边）
// - 初始化所有 UI 文本对象（字体、大小、颜色）
// - 根据游戏状态更新 UI 文本内容（脏标记优化）
// - 渲染所有 UI 层（按固定顺序）
//
// 设计原则：
// - 不实现游戏逻辑，不直接修改领域对象
// - 所有 UI 文本通过 UpdateText* 系列方法更新
// - 使用脏标记避免无变化时重复构建文本
// - 字体由外部注入（ResourceManager 提供）
//
// 渲染顺序（从上到下）：
//  1. 灵气覆盖层（aura_overlay）
//  2. 节日通知面板
//  3. 主信息面板（HUD）
//  4. 背包面板
//  5. 对话面板
//  6. 底部提示栏
//  7. 体力条
//  8. 工坊进度条
//  9. 世界提示
// 10. 升级提示（条件显示）
// 11. 调试文本（条件显示）
// ============================================================================

#include "CloudSeamanor/engine/GameWorldState.hpp"
#include "CloudSeamanor/infrastructure/UiLayoutConfig.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <memory>

namespace CloudSeamanor::engine {

// ============================================================================
// 【UiRenderOptions】UI 渲染选项
// ============================================================================
struct UiRenderOptions {
    bool show_debug = true;
    bool show_level_up = true;
    bool show_festival = true;
};

// ============================================================================
// 【UISystem】UI 管理器类
// ============================================================================
class UISystem {
public:
    // ========================================================================
    // 【UiTextType】脏标记文本类型（放在类顶部供所有成员函数使用）
    // ========================================================================
    enum class UiTextType {
        Hud, Inventory, Hint, Dialogue, Debug, WorldTip, FestivalNotice, LevelUp, COUNT
    };

    // ========================================================================
    // 【初始化】
    // ========================================================================

    /**
     * @brief 初始化所有 UI 面板（不依赖字体）。
     * @param config UI 布局配置（可选，为 nullptr 时使用内置默认值）。
     */
    void InitializePanels(const infrastructure::UiLayoutConfig* config = nullptr);

    /**
     * @brief 初始化所有 UI 文本对象（依赖字体）。
     * @param font 字体引用（由 ResourceManager 提供）。
     * @param config UI 布局配置（可选，为 nullptr 时使用内置默认值）。
     */
    void InitializeTexts(const sf::Font& font, const infrastructure::UiLayoutConfig* config = nullptr);

    /**
     * @brief 重新设置字体（运行时切换字体时调用）。
     * @param font 字体引用。
     */
    void ReloadFont(const sf::Font& font);

    // ========================================================================
    // 【文本更新（脏标记优化）】
    // ========================================================================

    /**
     * @brief 标记指定文本类型需要刷新。
     */
    void MarkDirty(UiTextType type);
    void MarkAllDirty();

    /**
     * @brief 更新 HUD 信息面板文本。
     */
    void UpdateHudText(const std::string& text);

    /**
     * @brief 更新背包面板文本。
     */
    void UpdateInventoryText(const std::string& text);

    /**
     * @brief 更新提示文本。
     */
    void UpdateHintText(const std::string& text);

    /**
     * @brief 更新对话面板文本。
     */
    void UpdateDialogueText(const std::string& text);

    /**
     * @brief 更新调试文本。
     */
    void UpdateDebugText(const std::string& text);

    /**
     * @brief 更新世界提示文本。
     */
    void UpdateWorldTipText(const std::string& text);

    /**
     * @brief 更新节日通知文本。
     */
    void UpdateFestivalNoticeText(const std::string& text);

    /**
     * @brief 更新升级提示文本。
     */
    void UpdateLevelUpText(const std::string& text);

    // ========================================================================
    // 【状态更新】
    // ========================================================================

    /**
     * @brief 更新体力条。
     * @param ratio 当前体力占总体的比例（0.0~1.0）。
     */
    void UpdateStaminaBar(float ratio);

    /**
     * @brief 更新工坊进度条。
     * @param ratio 当前进度比例（0.0~1.0），<0 表示无进度（隐藏）。
     */
    void UpdateWorkshopProgressBar(float ratio);

    /**
     * @brief 更新灵气覆盖层颜色。
     * @param color 新的填充色。
     */
    void UpdateAuraOverlay(const sf::Color& color);

    /**
     * @brief 更新世界提示脉冲效果（脉冲透明度）。
     * @param pulse 累积脉冲值（每帧递增）。
     */
    void UpdateWorldTipPulse(float pulse);

    /**
     * @brief 更新升级提示动画（缩放 + 上浮）。
     * @param remaining_time 剩余显示时间。
     */
    void UpdateLevelUpAnimation(float remaining_time);

    /**
     * @brief 隐藏升级提示。
     */
    void HideLevelUpText();

    // ========================================================================
    // 【渲染】
    // ========================================================================

    /**
     * @brief 渲染所有 UI 层（固定顺序）。
     * @param window 渲染目标。
     * @param opts 渲染选项。
     */
    void Render(sf::RenderWindow& window, const UiRenderOptions& opts);

    /**
     * @brief 仅渲染 HUD 面板（不含背景，用于调试）。
     */
    void RenderHudOnly(sf::RenderWindow& window);

    // ========================================================================
    // 【访问器】
    // ========================================================================

    [[nodiscard]] const UiPanels& GetPanels() const { return panels_; }
    [[nodiscard]] UiPanels& MutablePanels() { return panels_; }
    [[nodiscard]] const UiTexts& GetTexts() const { return texts_; }
    [[nodiscard]] UiTexts& MutableTexts() { return texts_; }

private:
    // 内部渲染方法
    void RenderBackgroundLayer_(sf::RenderWindow& window);
    void RenderPanels_(sf::RenderWindow& window);
    void RenderTexts_(sf::RenderWindow& window);
    void RenderProgressBars_(sf::RenderWindow& window);

    UiPanels panels_;
    UiTexts texts_;
    std::array<bool, static_cast<std::size_t>(UiTextType::COUNT)> dirty_flags_{};

    bool font_loaded_ = false;
    const sf::Font* font_ptr_ = nullptr;

    infrastructure::UiLayoutData layout_data_ = infrastructure::UiLayoutConfig::GetDefaults();
};

// ============================================================================
// 【MakeDefaultUiRenderOptions】创建默认 UI 渲染选项
// ============================================================================
[[nodiscard]] UiRenderOptions MakeDefaultUiRenderOptions();

}  // namespace CloudSeamanor::engine
