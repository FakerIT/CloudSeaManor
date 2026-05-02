#pragma once

// ============================================================================
// 【PixelToolUpgradePanel】工具升级面板
// ============================================================================
// 展示工具升级、洒水器建造界面。
// 使用现有的ToolSystem作为数据源。
// ============================================================================

#include "CloudSeamanor/engine/PixelUiPanel.hpp"
#include "CloudSeamanor/domain/ToolSystem.hpp"

#include <string>
#include <functional>

namespace CloudSeamanor::engine {

// ============================================================================
// 【TabType】标签页类型
// ============================================================================
enum class ToolPanelTab : std::uint8_t {
    Tools = 0,       // 工具升级
    Sprinklers = 1,  // 洒水器
    Fertilizer = 2,   // 肥料
    COUNT = 3
};

// ============================================================================
// 【ToolUpgradeViewData】工具升级面板数据
// ============================================================================
struct ToolUpgradeViewData {
    std::string panel_title = "工具工坊";
    std::string tab_tools = "工具升级";
    std::string tab_sprinklers = "洒水器";
    std::string tab_fertilizer = "肥料";

    struct ToolInfo {
        std::string name;
        std::string tier_name;
        std::string level_text;
        std::string exp_text;
        float exp_ratio = 0.0f;
        std::string effect_text;
        bool can_upgrade = false;
        bool is_max_level = false;
    };
    std::array<ToolInfo, 7> tools;  // 7种工具

    struct SprinklerInfo {
        std::string name;
        std::string coverage;
        std::string cost;
        bool can_build = false;
    };
    std::array<SprinklerInfo, 3> sprinklers;  // 3种洒水器
    int installed_count = 0;

    std::string player_gold_text;
    std::string hint_text;
};

// ============================================================================
// 【PixelToolUpgradePanel】工具升级面板类
// ============================================================================
class PixelToolUpgradePanel : public PixelUiPanel {
public:
    PixelToolUpgradePanel();

    // ========================================================================
    // 【配置】
    // ========================================================================
    void SetFontRenderer(const PixelFontRenderer* renderer);
    void UpdateData(const ToolUpgradeViewData& data);

    // ========================================================================
    // 【交互】
    // ========================================================================
    void SetOnUpgradeCallback(std::function<bool(int tool_index)> callback);
    void SetOnTabChanged(std::function<void(ToolPanelTab)> callback);
    void HandleKeyPress(int key);
    void SetActiveTab(ToolPanelTab tab);

    // ========================================================================
    // 【数据】
    // ========================================================================
    [[nodiscard]] ToolPanelTab GetActiveTab() const { return active_tab_; }
    [[nodiscard]] int GetSelectedToolIndex() const { return selected_tool_index_; }

private:
    // ========================================================================
    // 【渲染】
    // ========================================================================
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;

    // 渲染标签页
    void RenderTabs(sf::RenderWindow& window, const sf::FloatRect& inner_rect);

    // 渲染工具升级内容
    void RenderToolsContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect);

    // 渲染洒水器内容
    void RenderSprinklersContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect);

    // 渲染肥料内容
    void RenderFertilizerContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect);

    // ========================================================================
    // 【常量】
    // ========================================================================
    static constexpr float kTabHeight = 28.0f;
    static constexpr float kRowHeight = 36.0f;
    static constexpr float kLeftMargin = 12.0f;
    static constexpr float kTopMargin = 40.0f;

    // ========================================================================
    // 【成员】
    // ========================================================================
    const PixelFontRenderer* m_font_renderer = nullptr;
    ToolUpgradeViewData m_data{};
    ToolPanelTab active_tab_ = ToolPanelTab::Tools;
    int selected_tool_index_ = 0;

    std::function<bool(int tool_index)> on_upgrade_callback_;
    std::function<void(ToolPanelTab)> on_tab_changed_callback_;
};

}  // namespace CloudSeamanor::engine
