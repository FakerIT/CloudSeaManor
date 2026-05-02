#include "CloudSeamanor/engine/rendering/UiPanelInitializer.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"

namespace CloudSeamanor::engine {

namespace {

sf::Color Uint32ToSfColor(std::uint32_t rgba) {
    return {
        static_cast<std::uint8_t>((rgba >> 24) & 0xFF),
        static_cast<std::uint8_t>((rgba >> 16) & 0xFF),
        static_cast<std::uint8_t>((rgba >> 8) & 0xFF),
        static_cast<std::uint8_t>(rgba & 0xFF)
    };
}

sf::Vector2f ToVec2(const std::array<float, 2>& arr) {
    return {arr[0], arr[1]};
}

void ApplyPanelConfig(sf::RectangleShape& shape, const infrastructure::PanelLayout& cfg) {
    shape.setPosition(ToVec2(cfg.position));
    shape.setSize(ToVec2(cfg.size));
    shape.setFillColor(Uint32ToSfColor(cfg.fill_color));
    shape.setOutlineThickness(cfg.outline_thickness);
    shape.setOutlineColor(Uint32ToSfColor(cfg.outline_color));
}

void ApplyTextStyle(sf::Text& text, const infrastructure::TextStyleLayout& cfg) {
    text.setPosition(ToVec2(cfg.position));
    text.setCharacterSize(cfg.font_size);
    text.setFillColor(Uint32ToSfColor(cfg.color));
    text.setOutlineThickness(cfg.outline_thickness);
    text.setOutlineColor(Uint32ToSfColor(cfg.outline_color));
}

const infrastructure::PanelLayout& GetPanelOrFallback(
    const infrastructure::UiLayoutData& layout,
    const char* key) {
    const auto it = layout.panels.find(key);
    if (it != layout.panels.end()) {
        return it->second;
    }
    infrastructure::Logger::Warning(std::string("UiPanelInitializer: 缺少 panels 配置项: ") + key);
    static const infrastructure::PanelLayout kEmpty{};
    return kEmpty;
}

const infrastructure::TextStyleLayout& GetTextOrFallback(
    const infrastructure::UiLayoutData& layout,
    const char* key) {
    const auto it = layout.texts.find(key);
    if (it != layout.texts.end()) {
        return it->second;
    }
    infrastructure::Logger::Warning(std::string("UiPanelInitializer: 缺少 texts 配置项: ") + key);
    static const infrastructure::TextStyleLayout kEmpty{};
    return kEmpty;
}

}  // namespace

// ============================================================================
// 【UiPanelInitializer::InitializePanels】初始化所有面板
// ============================================================================
void UiPanelInitializer::InitializePanels(
    UiPanels& panels,
    const infrastructure::UiLayoutConfig* config) {
    infrastructure::UiLayoutData layout = infrastructure::UiLayoutConfig::GetDefaults();
    if (config) {
        layout = config->Data();
    }

    ApplyPanelConfig(panels.main_panel, GetPanelOrFallback(layout, "main_panel"));
    ApplyPanelConfig(panels.inventory_panel, GetPanelOrFallback(layout, "inventory_panel"));
    ApplyPanelConfig(panels.dialogue_panel, GetPanelOrFallback(layout, "dialogue_panel"));
    ApplyPanelConfig(panels.hint_panel, GetPanelOrFallback(layout, "hint_panel"));
    ApplyPanelConfig(panels.stamina_bar_bg, GetPanelOrFallback(layout, "stamina_bar"));
    ApplyPanelConfig(panels.workshop_progress_bg, GetPanelOrFallback(layout, "workshop_progress"));
    ApplyPanelConfig(panels.aura_overlay, GetPanelOrFallback(layout, "aura_overlay"));
    ApplyPanelConfig(panels.festival_notice_panel, GetPanelOrFallback(layout, "festival_notice"));

    auto stamina_cfg = GetPanelOrFallback(layout, "stamina_bar");
    panels.stamina_bar_fill.setPosition(ToVec2(stamina_cfg.position));
    panels.stamina_bar_fill.setSize(ToVec2(stamina_cfg.size));
    panels.stamina_bar_fill.setFillColor(Uint32ToSfColor(GetPanelOrFallback(layout, "stamina_fill").fill_color));

    auto workshop_cfg = GetPanelOrFallback(layout, "workshop_progress");
    panels.workshop_progress_fill.setPosition(ToVec2(workshop_cfg.position));
    panels.workshop_progress_fill.setSize({0.0f, workshop_cfg.size[1]});
    panels.workshop_progress_fill.setFillColor(Uint32ToSfColor(GetPanelOrFallback(layout, "workshop_fill").fill_color));
}

// ============================================================================
// 【UiPanelInitializer::InitializeTexts】初始化所有文本
// ============================================================================
void UiPanelInitializer::InitializeTexts(
    const sf::Font& font,
    UiTexts& texts,
    const infrastructure::UiLayoutConfig* config) {
    infrastructure::UiLayoutData layout = infrastructure::UiLayoutConfig::GetDefaults();
    if (config) {
        layout = config->Data();
    }

    auto make_text = [&](const std::string& key) -> std::unique_ptr<sf::Text> {
        auto t = std::make_unique<sf::Text>(font);
        ApplyTextStyle(*t, GetTextOrFallback(layout, key.c_str()));
        t->setString(sf::String{});
        return t;
    };

    texts.hud_text = make_text("hud");
    texts.inventory_text = make_text("inventory");
    texts.hint_text = make_text("hint");
    texts.dialogue_text = make_text("dialogue");
    texts.debug_text = make_text("debug");
    texts.world_tip_text = make_text("world_tip");
    texts.festival_notice_text = make_text("festival_notice");
    texts.level_up_text = make_text("level_up");
    texts.level_up_text->setString(sf::String{});
}

}  // namespace CloudSeamanor::engine
