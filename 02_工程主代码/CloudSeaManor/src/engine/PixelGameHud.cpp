#include "CloudSeamanor/PixelGameHud.hpp"

#include "CloudSeamanor/InputManager.hpp"
#include "CloudSeamanor/ResourceManager.hpp"
#include "CloudSeamanor/UiAtlasMappings.hpp"
#include "CloudSeamanor/UiVertexHelpers.hpp"

#include <SFML/Graphics/Sprite.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <string_view>

namespace CloudSeamanor::engine {

namespace {
using CloudSeamanor::engine::uivx::AddQuad;

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

sf::FloatRect ScaleRect(const sf::FloatRect& r, float s) {
    return {{std::round(r.position.x * s), std::round(r.position.y * s)},
            {std::round(r.size.x * s), std::round(r.size.y * s)}};
}

sf::Color SemanticColorOr(
    const infrastructure::UiLayoutData& layout,
    const char* key,
    const sf::Color& fallback) {
    const auto it = layout.semantic_colors.find(key);
    if (it != layout.semantic_colors.end()) {
        return Uint32ToSfColor(it->second);
    }
    return fallback;
}

TextStyle ToPixelTextStyle(const infrastructure::TextStyleLayout& cfg, TextStyle base) {
    base.character_size = cfg.font_size;
    base.fill_color = Uint32ToSfColor(cfg.color);
    if (cfg.outline_thickness > 0.0f) {
        base.outline_thickness = cfg.outline_thickness;
        base.outline_color = Uint32ToSfColor(cfg.outline_color);
    }
    return base;
}

struct SeasonThemeColors {
    sf::Color fill;
    sf::Color border;
};

constexpr float kNpcDetailPanelGap = 16.0f;
constexpr float kNpcDetailPanelWidth = 320.0f;

[[nodiscard]] std::string ToLowerAscii(std::string_view text) {
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

[[nodiscard]] bool ContainsAny(std::string_view text, std::initializer_list<std::string_view> keys) {
    for (const auto key : keys) {
        if (!key.empty() && text.find(key) != std::string_view::npos) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] SeasonThemeColors SeasonThemeFor(std::string_view season_text) {
    const std::string lower = ToLowerAscii(season_text);
    const std::string_view v = lower;
    if (ContainsAny(season_text, {"春"}) || ContainsAny(v, {"spring"})) {
        return {sf::Color(223, 238, 213), sf::Color(109, 140, 91)};
    }
    if (ContainsAny(season_text, {"夏"}) || ContainsAny(v, {"summer"})) {
        return {sf::Color(209, 235, 221), sf::Color(73, 124, 99)};
    }
    if (ContainsAny(season_text, {"秋"}) || ContainsAny(v, {"autumn", "fall"})) {
        return {sf::Color(241, 224, 196), sf::Color(140, 102, 66)};
    }
    if (ContainsAny(season_text, {"冬"}) || ContainsAny(v, {"winter"})) {
        return {sf::Color(217, 227, 237), sf::Color(90, 108, 126)};
    }
    return {ColorPalette::Cream, ColorPalette::BrownOutline};
}
}  // namespace

// ============================================================================
// 【PixelGameHud::Initialize】
// ============================================================================
void PixelGameHud::Initialize(const sf::Font& font) {
    Initialize(font, nullptr);
}

void PixelGameHud::Initialize(const sf::Font& font, const infrastructure::UiLayoutConfig* layout_config) {
    font_renderer_ = std::make_unique<PixelFontRenderer>(font);
    font_renderer_->SetUiScale(ui_scale_);
    settings_panel_.SetFontRenderer(font_renderer_.get());
    inventory_grid_.SetFontRenderer(font_renderer_.get());
    quest_menu_.SetFontRenderer(font_renderer_.get());
    minimap_.SetFontRenderer(font_renderer_.get());
    cloud_forecast_panel_.SetFontRenderer(font_renderer_.get());
    player_status_panel_.SetFontRenderer(font_renderer_.get());
    tutorial_bubble_.SetFontRenderer(font_renderer_.get());
    tea_garden_panel_.SetFontRenderer(font_renderer_.get());
    workshop_panel_.SetFontRenderer(font_renderer_.get());
    contract_panel_.SetFontRenderer(font_renderer_.get());
    npc_detail_panel_.SetFontRenderer(font_renderer_.get());
    spirit_beast_panel_.SetFontRenderer(font_renderer_.get());
    festival_panel_.SetFontRenderer(font_renderer_.get());
    spirit_realm_panel_.SetFontRenderer(font_renderer_.get());
    building_panel_.SetFontRenderer(font_renderer_.get());
    shop_panel_.SetFontRenderer(font_renderer_.get());
    mail_panel_.SetFontRenderer(font_renderer_.get());
    achievement_panel_.SetFontRenderer(font_renderer_.get());
    beastiary_panel_.SetFontRenderer(font_renderer_.get());
    context_menu_.SetFontRenderer(font_renderer_.get());

    ApplyLayout_(layout_config);

    // 默认隐藏所有面板
    inventory_grid_.SetVisible(false);
    quest_menu_.SetVisible(false);
    minimap_.SetVisible(false);
    settings_panel_.SetVisible(false);
    cloud_forecast_panel_.SetVisible(false);
    player_status_panel_.SetVisible(false);
    tutorial_bubble_.SetVisible(false);
    tea_garden_panel_.SetVisible(false);
    workshop_panel_.SetVisible(false);
    contract_panel_.SetVisible(false);
    npc_detail_panel_.SetVisible(false);
    spirit_beast_panel_.SetVisible(false);
    festival_panel_.SetVisible(false);
    spirit_realm_panel_.SetVisible(false);
    building_panel_.SetVisible(false);
    shop_panel_.SetVisible(false);
    mail_panel_.SetVisible(false);
    achievement_panel_.SetVisible(false);
    beastiary_panel_.SetVisible(false);
    context_menu_.SetVisible(false);

    top_right_bg_.setPrimitiveType(sf::PrimitiveType::Triangles);
    coin_bg_.setPrimitiveType(sf::PrimitiveType::Triangles);
    ui_atlas_loaded_ = LoadUiAtlas_();
    ApplyScaleLayout_();
    ApplySeasonTheme_(top_right_season_text_);
}

bool PixelGameHud::LoadUiAtlas_() {
    if (resource_manager_ == nullptr) {
        return false;
    }
    const std::filesystem::path atlas_texture = atlas::kTinyTownTilemapPath;
    std::error_code ec;
    if (!std::filesystem::exists(atlas_texture, ec)) {
        return false;
    }
    const std::string id = "pixel_hud_ui_atlas";
    atlas_texture_id_ = id;
    if (resource_manager_->LoadTexture(id, atlas_texture.string())) {
        resource_manager_->Acquire(id);
        return true;
    }
    return false;
}

void PixelGameHud::DrawUiFrame_(sf::RenderWindow& window,
                                const sf::IntRect& rect,
                                const sf::Vector2f& position,
                                const sf::Vector2f& size) const {
    if (!ui_atlas_loaded_ || resource_manager_ == nullptr || atlas_texture_id_.empty()) {
        return;
    }
    sf::Sprite sprite(resource_manager_->GetTexture(atlas_texture_id_), rect);
    if (rect.size.x > 0 && rect.size.y > 0) {
        const float uniform = std::min(
            size.x / static_cast<float>(rect.size.x),
            size.y / static_cast<float>(rect.size.y));
        const sf::Vector2f draw_size{
            static_cast<float>(rect.size.x) * uniform,
            static_cast<float>(rect.size.y) * uniform
        };
        sprite.setPosition({
            position.x + (size.x - draw_size.x) * 0.5f,
            position.y + (size.y - draw_size.y) * 0.5f
        });
        sprite.setScale({uniform, uniform});
    } else {
        sprite.setPosition(position);
    }
    window.draw(sprite);
}

void PixelGameHud::SetResourceManager(infrastructure::ResourceManager* rm) {
    resource_manager_ = rm;
    settings_panel_.SetResourceManager(rm);
    ui_atlas_loaded_ = LoadUiAtlas_();
}

void PixelGameHud::SetUiScale(float scale) {
    const float snapped = std::max(1.0f, std::floor(scale));
    if (std::abs(ui_scale_ - snapped) < 0.001f) return;
    ui_scale_ = snapped;
    if (font_renderer_) {
        font_renderer_->SetUiScale(ui_scale_);
    }
    ApplyScaleLayout_();
}

void PixelGameHud::ApplyLayout_(const infrastructure::UiLayoutConfig* layout_config) {
    layout_data_ = layout_config ? layout_config->Data()
                                 : infrastructure::UiLayoutConfig::GetDefaults();

    const auto get_panel = [&](const char* key) -> infrastructure::PanelLayout {
        const auto it = layout_data_.panels.find(key);
        return it != layout_data_.panels.end()
            ? it->second
            : infrastructure::UiLayoutConfig::GetDefaults().panels[key];
    };

    // Panels
    const auto toolbar = get_panel("pixel_toolbar");
    toolbar_.SetRect({ToVec2(toolbar.position), ToVec2(toolbar.size)});
    toolbar_.SetColors(Uint32ToSfColor(toolbar.fill_color), Uint32ToSfColor(toolbar.outline_color));

    const auto inv = get_panel("pixel_inventory");
    inventory_grid_.SetRect({ToVec2(inv.position), ToVec2(inv.size)});
    inventory_grid_.SetColors(Uint32ToSfColor(inv.fill_color), Uint32ToSfColor(inv.outline_color));

    const auto quest = get_panel("pixel_quest_menu");
    quest_menu_.SetRect({ToVec2(quest.position), ToVec2(quest.size)});
    quest_menu_.SetColors(Uint32ToSfColor(quest.fill_color), Uint32ToSfColor(quest.outline_color));
    quest_menu_.SetSemanticColors(
        SemanticColorOr(layout_data_, "button_hover_bg", ColorPalette::HighlightYellow),
        SemanticColorOr(layout_data_, "button_selected_bg", ColorPalette::ActiveGreen),
        SemanticColorOr(layout_data_, "button_default_bg", ColorPalette::BackgroundWhite),
        SemanticColorOr(layout_data_, "button_selected_text", ColorPalette::SuccessGreen),
        SemanticColorOr(layout_data_, "button_selected_text", ColorPalette::ActiveGreen),
        SemanticColorOr(layout_data_, "button_disabled_bg", ColorPalette::LightGray));

    const auto minimap = get_panel("pixel_minimap");
    const auto minimap_inner = get_panel("pixel_minimap_inner");
    minimap_.SetRect({ToVec2(minimap.position), ToVec2(minimap.size)});
    minimap_.SetColors(Uint32ToSfColor(minimap.fill_color),
                       Uint32ToSfColor(minimap.outline_color),
                       Uint32ToSfColor(minimap_inner.fill_color));

    const auto dbox = get_panel("pixel_dialogue_box");
    dialogue_box_.SetRect({ToVec2(dbox.position), ToVec2(dbox.size)});
    dialogue_box_.SetColors(Uint32ToSfColor(dbox.fill_color), Uint32ToSfColor(dbox.outline_color));

    // Progress bar
    const auto stamina_bg = get_panel("pixel_stamina_bar");
    const auto stamina_fill = get_panel("pixel_stamina_fill");
    const auto stamina_low = get_panel("pixel_stamina_low");
    const auto stamina_full = get_panel("pixel_stamina_full");
    stamina_bar_.SetPosition(ToVec2(stamina_bg.position));
    stamina_bar_.SetStaminaStyle();
    stamina_bar_.SetSize(ToVec2(stamina_bg.size));
    stamina_bar_.SetColors(Uint32ToSfColor(stamina_bg.fill_color),
                           Uint32ToSfColor(stamina_bg.outline_color),
                           Uint32ToSfColor(stamina_fill.fill_color),
                           Uint32ToSfColor(stamina_low.fill_color),
                           Uint32ToSfColor(stamina_full.fill_color));
    hunger_bar_.SetPosition({ToVec2(stamina_bg.position).x, ToVec2(stamina_bg.position).y + 16.0f});
    hunger_bar_.SetSize({ToVec2(stamina_bg.size).x, ToVec2(stamina_bg.size).y});
    hunger_bar_.SetColors(
        sf::Color(62, 45, 34),
        sf::Color(113, 78, 47),
        sf::Color(214, 164, 88),
        sf::Color(176, 89, 68),
        sf::Color(236, 196, 96));

    top_right_geometry_dirty_ = true;
    coin_geometry_dirty_ = true;
    ApplyScaleLayout_();
}

void PixelGameHud::ApplyScaleLayout_() {
    const auto base = infrastructure::UiLayoutConfig::GetDefaults();
    const auto panel_scaled = [&](const char* key) -> sf::FloatRect {
        const auto it = layout_data_.panels.find(key);
        const auto cfg = (it != layout_data_.panels.end()) ? it->second : base.panels.at(key);
        return ScaleRect({ToVec2(cfg.position), ToVec2(cfg.size)}, ui_scale_);
    };

    inventory_grid_.SetRect(panel_scaled("pixel_inventory"));
    quest_menu_.SetRect(panel_scaled("pixel_quest_menu"));
    minimap_.SetRect(panel_scaled("pixel_minimap"));
    toolbar_.SetRect(panel_scaled("pixel_toolbar"));
    const auto stamina_rect = panel_scaled("pixel_stamina_bar");
    stamina_bar_.SetPosition(stamina_rect.position);
    stamina_bar_.SetSize(stamina_rect.size);
    hunger_bar_.SetPosition({stamina_rect.position.x, stamina_rect.position.y + std::round(16.0f * ui_scale_)});
    hunger_bar_.SetSize(stamina_rect.size);
    top_right_geometry_dirty_ = true;
    coin_geometry_dirty_ = true;
}

void PixelGameHud::EmitUiEvent_(UiEventType event_type) {
    if (on_ui_event_) on_ui_event_(event_type);
}

void PixelGameHud::ApplySeasonTheme_(const std::string& season_text) {
    const SeasonThemeColors theme = SeasonThemeFor(season_text);
    toolbar_.SetColors(theme.fill, theme.border);
    inventory_grid_.SetColors(theme.fill, theme.border);
    quest_menu_.SetColors(theme.fill, theme.border);
    minimap_.SetColors(theme.fill, theme.border, ColorPalette::Lighten(theme.fill, 10));
    settings_panel_.SetColors(theme.fill, theme.border);
    cloud_forecast_panel_.SetColors(theme.fill, theme.border);
    player_status_panel_.SetColors(theme.fill, theme.border);
    tea_garden_panel_.SetColors(theme.fill, theme.border);
    workshop_panel_.SetColors(theme.fill, theme.border);
    contract_panel_.SetColors(theme.fill, theme.border);
    npc_detail_panel_.SetColors(theme.fill, theme.border);
    spirit_beast_panel_.SetColors(theme.fill, theme.border);
    festival_panel_.SetColors(theme.fill, theme.border);
    spirit_realm_panel_.SetColors(theme.fill, theme.border);
    building_panel_.SetColors(theme.fill, theme.border);
    shop_panel_.SetColors(theme.fill, theme.border);
    mail_panel_.SetColors(theme.fill, theme.border);
    achievement_panel_.SetColors(theme.fill, theme.border);
    beastiary_panel_.SetColors(theme.fill, theme.border);
}

// ============================================================================
// 【PixelGameHud::ToggleInventory】
// ============================================================================
void PixelGameHud::ToggleInventory() {
    if (panel_state_.dialogue_open) return;

    panel_state_.inventory_open = !panel_state_.inventory_open;
    inventory_grid_.SetVisible(panel_state_.inventory_open);
    if (panel_state_.inventory_open) {
        inventory_grid_.Open();
        EmitUiEvent_(UiEventType::Open);
    } else {
        inventory_grid_.Close();
        npc_detail_panel_.SetVisible(false);
        EmitUiEvent_(UiEventType::Close);
    }

    // 关闭其他面板
    if (panel_state_.inventory_open) {
        panel_state_.quest_menu_open = false;
        quest_menu_.SetVisible(false);
        panel_state_.map_open = false;
        minimap_.SetVisible(false);
    }
}

// ============================================================================
// 【PixelGameHud::ToggleQuestMenu】
// ============================================================================
void PixelGameHud::ToggleQuestMenu() {
    if (panel_state_.dialogue_open) return;

    panel_state_.quest_menu_open = !panel_state_.quest_menu_open;
    quest_menu_.SetVisible(panel_state_.quest_menu_open);
    if (panel_state_.quest_menu_open) {
        quest_menu_.Open();
        EmitUiEvent_(UiEventType::Open);
    } else {
        quest_menu_.Close();
        EmitUiEvent_(UiEventType::Close);
    }

    if (panel_state_.quest_menu_open) {
        panel_state_.inventory_open = false;
        inventory_grid_.SetVisible(false);
        panel_state_.map_open = false;
        minimap_.SetVisible(false);
    }
}

// ============================================================================
// 【PixelGameHud::ToggleMap】
// ============================================================================
void PixelGameHud::ToggleMap() {
    if (panel_state_.dialogue_open) return;

    panel_state_.map_open = !panel_state_.map_open;
    minimap_.SetVisible(panel_state_.map_open);
    if (panel_state_.map_open) {
        minimap_.Open();
        EmitUiEvent_(UiEventType::Open);
    } else {
        minimap_.Close();
        EmitUiEvent_(UiEventType::Close);
    }

    if (panel_state_.map_open) {
        panel_state_.inventory_open = false;
        inventory_grid_.SetVisible(false);
        panel_state_.quest_menu_open = false;
        quest_menu_.SetVisible(false);
    }
}

void PixelGameHud::ToggleSettings() {
    if (panel_state_.dialogue_open) return;
    const bool now_open = !settings_panel_.IsVisible();
    settings_panel_.SetVisible(now_open);
    EmitUiEvent_(now_open ? UiEventType::Open : UiEventType::Close);
    if (now_open) {
        panel_state_.inventory_open = false;
        panel_state_.quest_menu_open = false;
        panel_state_.map_open = false;
        inventory_grid_.SetVisible(false);
        quest_menu_.SetVisible(false);
        minimap_.SetVisible(false);
    }
}

void PixelGameHud::ToggleCloudForecast() {
    if (panel_state_.dialogue_open) return;
    panel_state_.cloud_forecast_open = !panel_state_.cloud_forecast_open;
    cloud_forecast_panel_.SetVisible(panel_state_.cloud_forecast_open);
    if (panel_state_.cloud_forecast_open) {
        cloud_forecast_panel_.FadeIn();
        EmitUiEvent_(UiEventType::Open);
        panel_state_.inventory_open = false;
        panel_state_.quest_menu_open = false;
        panel_state_.map_open = false;
        panel_state_.player_status_open = false;
        inventory_grid_.SetVisible(false);
        quest_menu_.SetVisible(false);
        minimap_.SetVisible(false);
        player_status_panel_.SetVisible(false);
    } else {
        cloud_forecast_panel_.FadeOut();
        EmitUiEvent_(UiEventType::Close);
    }
}

void PixelGameHud::TogglePlayerStatus() {
    if (panel_state_.dialogue_open) return;
    panel_state_.player_status_open = !panel_state_.player_status_open;
    player_status_panel_.SetVisible(panel_state_.player_status_open);
    if (panel_state_.player_status_open) {
        player_status_panel_.FadeIn();
        EmitUiEvent_(UiEventType::Open);
        panel_state_.inventory_open = false;
        panel_state_.quest_menu_open = false;
        panel_state_.map_open = false;
        panel_state_.cloud_forecast_open = false;
        inventory_grid_.SetVisible(false);
        quest_menu_.SetVisible(false);
        minimap_.SetVisible(false);
        cloud_forecast_panel_.SetVisible(false);
    } else {
        player_status_panel_.FadeOut();
        EmitUiEvent_(UiEventType::Close);
    }
}

namespace {
void ToggleModalPanel(PixelUiPanel& panel, bool now_open) {
    panel.SetVisible(now_open);
    if (now_open) {
        panel.FadeIn();
    } else {
        panel.FadeOut();
    }
}
}  // namespace

void PixelGameHud::ToggleTeaGarden() {
    if (panel_state_.dialogue_open) return;
    const bool now_open = !tea_garden_panel_.IsVisible();
    CloseAllPanels();
    ToggleModalPanel(tea_garden_panel_, now_open);
    EmitUiEvent_(now_open ? UiEventType::Open : UiEventType::Close);
}

void PixelGameHud::ToggleWorkshop() {
    if (panel_state_.dialogue_open) return;
    const bool now_open = !workshop_panel_.IsVisible();
    CloseAllPanels();
    ToggleModalPanel(workshop_panel_, now_open);
    EmitUiEvent_(now_open ? UiEventType::Open : UiEventType::Close);
}

void PixelGameHud::ToggleContract() {
    if (panel_state_.dialogue_open) return;
    const bool now_open = !contract_panel_.IsVisible();
    CloseAllPanels();
    ToggleModalPanel(contract_panel_, now_open);
    EmitUiEvent_(now_open ? UiEventType::Open : UiEventType::Close);
}

void PixelGameHud::ToggleNpcDetail() {
    if (panel_state_.dialogue_open) return;
    const bool now_open = !npc_detail_panel_.IsVisible();
    CloseAllPanels();
    ToggleModalPanel(npc_detail_panel_, now_open);
    EmitUiEvent_(now_open ? UiEventType::Open : UiEventType::Close);
}

void PixelGameHud::ToggleSpiritBeast() {
    if (panel_state_.dialogue_open) return;
    const bool now_open = !spirit_beast_panel_.IsVisible();
    CloseAllPanels();
    ToggleModalPanel(spirit_beast_panel_, now_open);
    EmitUiEvent_(now_open ? UiEventType::Open : UiEventType::Close);
}

void PixelGameHud::ToggleFestival() {
    if (panel_state_.dialogue_open) return;
    const bool now_open = !festival_panel_.IsVisible();
    CloseAllPanels();
    ToggleModalPanel(festival_panel_, now_open);
    EmitUiEvent_(now_open ? UiEventType::Open : UiEventType::Close);
}

void PixelGameHud::ToggleSpiritRealm() {
    if (panel_state_.dialogue_open) return;
    const bool now_open = !spirit_realm_panel_.IsVisible();
    CloseAllPanels();
    ToggleModalPanel(spirit_realm_panel_, now_open);
    EmitUiEvent_(now_open ? UiEventType::Open : UiEventType::Close);
}

void PixelGameHud::ToggleBuilding() {
    if (panel_state_.dialogue_open) return;
    const bool now_open = !building_panel_.IsVisible();
    CloseAllPanels();
    ToggleModalPanel(building_panel_, now_open);
    EmitUiEvent_(now_open ? UiEventType::Open : UiEventType::Close);
}

void PixelGameHud::ToggleShop() {
    if (panel_state_.dialogue_open) return;
    const bool now_open = !shop_panel_.IsVisible();
    CloseAllPanels();
    ToggleModalPanel(shop_panel_, now_open);
    EmitUiEvent_(now_open ? UiEventType::Open : UiEventType::Close);
}

void PixelGameHud::ToggleMail() {
    if (panel_state_.dialogue_open) return;
    const bool now_open = !mail_panel_.IsVisible();
    CloseAllPanels();
    ToggleModalPanel(mail_panel_, now_open);
    EmitUiEvent_(now_open ? UiEventType::Open : UiEventType::Close);
}

void PixelGameHud::ToggleAchievement() {
    if (panel_state_.dialogue_open) return;
    const bool now_open = !achievement_panel_.IsVisible();
    CloseAllPanels();
    ToggleModalPanel(achievement_panel_, now_open);
    EmitUiEvent_(now_open ? UiEventType::Open : UiEventType::Close);
}

void PixelGameHud::ToggleBeastiary() {
    if (panel_state_.dialogue_open) return;
    const bool now_open = !beastiary_panel_.IsVisible();
    CloseAllPanels();
    ToggleModalPanel(beastiary_panel_, now_open);
    EmitUiEvent_(now_open ? UiEventType::Open : UiEventType::Close);
}

void PixelGameHud::SettingsMoveSelection(int delta) {
    if (!settings_panel_.IsVisible()) return;
    settings_panel_.MoveSelection(delta);
}

void PixelGameHud::SettingsAdjustValue(int delta) {
    if (!settings_panel_.IsVisible()) return;
    settings_panel_.AdjustValue(delta);
}

void PixelGameHud::SetSettingsSlots(const std::vector<CloudSeamanor::infrastructure::SaveSlotMetadata>& slots) {
    settings_panel_.SetSlots(slots);
}

int PixelGameHud::SettingsSelectedSlot() const {
    return settings_panel_.SelectedSlot();
}

int PixelGameHud::SettingsSelectedRow() const {
    return settings_panel_.SelectedRow();
}

// ============================================================================
// 【PixelGameHud::CloseAllPanels】
// ============================================================================
void PixelGameHud::CloseAllPanels() {
    panel_state_.inventory_open = false;
    panel_state_.quest_menu_open = false;
    panel_state_.map_open = false;
    inventory_grid_.SetVisible(false);
    quest_menu_.SetVisible(false);
    minimap_.SetVisible(false);
    settings_panel_.SetVisible(false);
    panel_state_.cloud_forecast_open = false;
    panel_state_.player_status_open = false;
    cloud_forecast_panel_.SetVisible(false);
    player_status_panel_.SetVisible(false);
    tea_garden_panel_.SetVisible(false);
    workshop_panel_.SetVisible(false);
    contract_panel_.SetVisible(false);
    npc_detail_panel_.SetVisible(false);
    spirit_beast_panel_.SetVisible(false);
    festival_panel_.SetVisible(false);
    spirit_realm_panel_.SetVisible(false);
    building_panel_.SetVisible(false);
    shop_panel_.SetVisible(false);
    mail_panel_.SetVisible(false);
    achievement_panel_.SetVisible(false);
    beastiary_panel_.SetVisible(false);
    tooltip_.Hide();
    context_menu_.CloseMenu();
    EmitUiEvent_(UiEventType::Close);
}

void PixelGameHud::DialogueMoveChoice(int delta) {
    dialogue_box_.MoveChoiceHover(delta);
}

void PixelGameHud::DialogueConfirmChoice() {
    dialogue_box_.ConfirmHoveredChoice();
}

// ============================================================================
// 【PixelGameHud::IsAnyPanelOpen】
// ============================================================================
bool PixelGameHud::IsAnyPanelOpen() const {
    return panel_state_.inventory_open ||
           panel_state_.quest_menu_open ||
           panel_state_.map_open ||
           panel_state_.dialogue_open ||
           panel_state_.cloud_forecast_open ||
           panel_state_.player_status_open ||
           settings_panel_.IsVisible() ||
           tea_garden_panel_.IsVisible() ||
           workshop_panel_.IsVisible() ||
           contract_panel_.IsVisible() ||
           npc_detail_panel_.IsVisible() ||
           spirit_beast_panel_.IsVisible() ||
           festival_panel_.IsVisible() ||
           spirit_realm_panel_.IsVisible() ||
           building_panel_.IsVisible() ||
           shop_panel_.IsVisible() ||
           mail_panel_.IsVisible() ||
           achievement_panel_.IsVisible() ||
           beastiary_panel_.IsVisible();
}

// ============================================================================
// 【PixelGameHud::Update】
// ============================================================================
void PixelGameHud::Update(float delta_seconds, const DialogueEngine* dialogue_engine) {
    // 更新动画
    toolbar_.UpdateAnimation(delta_seconds);
    dialogue_box_.Update(delta_seconds);
    inventory_grid_.UpdateAnimation(delta_seconds);
    quest_menu_.UpdateAnimation(delta_seconds);
    minimap_.UpdateAnimation(delta_seconds);
    settings_panel_.UpdateAnimation(delta_seconds);
    cloud_forecast_panel_.UpdateAnimation(delta_seconds);
    player_status_panel_.UpdateAnimation(delta_seconds);
    tutorial_bubble_.UpdateAnimation(delta_seconds);
    tea_garden_panel_.UpdateAnimation(delta_seconds);
    workshop_panel_.UpdateAnimation(delta_seconds);
    contract_panel_.UpdateAnimation(delta_seconds);
    npc_detail_panel_.UpdateAnimation(delta_seconds);
    spirit_beast_panel_.UpdateAnimation(delta_seconds);
    festival_panel_.UpdateAnimation(delta_seconds);
    spirit_realm_panel_.UpdateAnimation(delta_seconds);
    building_panel_.UpdateAnimation(delta_seconds);
    shop_panel_.UpdateAnimation(delta_seconds);
    mail_panel_.UpdateAnimation(delta_seconds);
    achievement_panel_.UpdateAnimation(delta_seconds);
    beastiary_panel_.UpdateAnimation(delta_seconds);
    notification_banner_.Update(delta_seconds);
    tooltip_.Update(delta_seconds, last_mouse_pos_);

    // 从 DialogueEngine 同步对话状态
    if (dialogue_engine) {
        dialogue_box_.SyncFromDialogueEngine(*dialogue_engine, "", nullptr);
    } else if (dialogue_box_.IsActive()) {
        dialogue_box_.Hide();
    }
    panel_state_.dialogue_open = dialogue_box_.IsActive();

    // 更新体力条低状态动画
    stamina_bar_.UpdateLowStateAnimation(delta_seconds);

    // 更新右上角闪烁（任务感叹号）
    top_right_blink_timer_ = std::fmod(top_right_blink_timer_ + delta_seconds,
                                       AnimationConfig::BlinkInterval * 2.0f);

    top_right_geometry_dirty_ = true;
    coin_geometry_dirty_ = true;
    focus_breath_timer_ += delta_seconds;
}

void PixelGameHud::UpdateTutorialBubble(int step_index,
                                       const std::string& text,
                                       const sf::FloatRect& highlight_rect,
                                       bool visible) {
    tutorial_visible_ = visible;
    tutorial_highlight_rect_ = highlight_rect;
    tutorial_bubble_.SetStep(step_index, text);
    if (tutorial_visible_) {
        if (!tutorial_bubble_.IsVisible()) {
            tutorial_bubble_.SetVisible(true);
            tutorial_bubble_.FadeIn();
        }
    } else {
        if (tutorial_bubble_.IsVisible()) {
            tutorial_bubble_.FadeOut();
        }
    }
}

std::optional<int> PixelGameHud::ConsumeTutorialStepDelta() {
    if (tutorial_step_delta_ == 0) return std::nullopt;
    const int v = tutorial_step_delta_;
    tutorial_step_delta_ = 0;
    return v;
}

// ============================================================================
// 【PixelGameHud::UpdateTopRightInfo】
// ============================================================================
void PixelGameHud::UpdateTopRightInfo(const std::string& time_text,
                                      const std::string& season_text,
                                      const std::string& weather_text,
                                      bool has_new_quest) {
    const bool season_changed = (top_right_season_text_ != season_text);
    top_right_time_text_ = time_text;
    top_right_season_text_ = season_text;
    top_right_weather_text_ = weather_text;
    top_right_has_new_quest_ = has_new_quest;
    if (season_changed) {
        ApplySeasonTheme_(top_right_season_text_);
    }
    top_right_geometry_dirty_ = true;
}

// ============================================================================
// 【PixelGameHud::UpdateStaminaBar】
// ============================================================================
void PixelGameHud::UpdateStaminaBar(float stamina_ratio, float /*current*/, float /*max_stamina*/) {
    stamina_ratio_ = std::clamp(stamina_ratio, 0.0f, 1.0f);
    stamina_bar_.SetProgress(stamina_ratio);
    stamina_bar_.SetLabel(std::to_string(static_cast<int>(stamina_ratio_ * 100.0f)) + "%");
    low_stamina_active_ = stamina_ratio_ < StaminaBarConfig::LowThreshold;
    critical_stamina_active_ = stamina_ratio_ < StaminaBarConfig::CriticalThreshold;
    stamina_bar_.SetLowState(low_stamina_active_);
}

void PixelGameHud::UpdateHungerBar(float hunger_ratio, float /*current*/, float /*max_hunger*/) {
    hunger_ratio_ = std::clamp(hunger_ratio, 0.0f, 1.0f);
    hunger_bar_.SetProgress(hunger_ratio_);
    hunger_bar_.SetLabel(std::to_string(static_cast<int>(hunger_ratio_ * 100.0f)) + "%");
    hunger_bar_.SetLowState(hunger_ratio_ < 0.25f);
}

// ============================================================================
// 【PixelGameHud::UpdateCoins】
// ============================================================================
void PixelGameHud::UpdateCoins(int coin_amount) {
    if (coin_amount_ != coin_amount) {
        coin_amount_ = coin_amount;
        coin_geometry_dirty_ = true;
    }
}

// ============================================================================
// 【PixelGameHud::UpdateToolbar】
// ============================================================================
void PixelGameHud::UpdateToolbar(const std::array<ToolbarSlot, 12>& slots, int selected_slot) {
    toolbar_.UpdateSlots(slots);
    toolbar_.SetSelectedSlot(selected_slot);
}

// ============================================================================
// 【PixelGameHud::UpdateDialogue】
// ============================================================================
void PixelGameHud::UpdateDialogue(const std::string& speaker_name,
                                  const std::string& full_text,
                                  const std::vector<DialogueChoice>& choices,
                                  const sf::Texture* avatar) {
    if (!dialogue_box_.IsActive()) {
        dialogue_box_.Show();
        dialogue_box_.SetSpeakerName(speaker_name);
        dialogue_box_.SetAvatar(avatar);
        dialogue_box_.SetText(full_text);
        dialogue_box_.SetChoices(choices);
        panel_state_.dialogue_open = true;
    }
}

// ============================================================================
// 【PixelGameHud::UpdateQuests】
// ============================================================================
void PixelGameHud::UpdateQuests(const std::vector<Quest>& quests) {
    quest_menu_.UpdateQuests(quests);
}

// ============================================================================
// 【PixelGameHud::UpdateMapMarkers】
// ============================================================================
void PixelGameHud::UpdateMapMarkers(const std::vector<MapMarker>& markers) {
    minimap_.UpdateMarkers(markers);
}

void PixelGameHud::UpdateCloudForecast(const CloudForecastViewData& data) {
    cloud_forecast_panel_.UpdateData(data);
}

void PixelGameHud::UpdatePlayerStatus(const PlayerStatusViewData& data) {
    player_status_panel_.UpdateData(data);
}

void PixelGameHud::UpdateTeaGardenPanel(const TeaGardenPanelViewData& data) {
    tea_garden_panel_.UpdateData(data);
}

void PixelGameHud::UpdateFestivalPanel(const FestivalPanelViewData& data) {
    festival_panel_.UpdateData(data);
}

void PixelGameHud::UpdateShopPanel(const ShopPanelViewData& data) {
    shop_panel_.UpdateData(data);
}

void PixelGameHud::UpdateMailPanel(const MailPanelViewData& data) {
    mail_panel_.UpdateData(data);
}

void PixelGameHud::UpdateAchievementPanel(const AchievementPanelViewData& data) {
    achievement_panel_.UpdateData(data);
}

void PixelGameHud::UpdateSpiritBeastPanel(const SpiritBeastPanelViewData& data) {
    spirit_beast_panel_.UpdateData(data);
}

void PixelGameHud::UpdateBuildingPanel(const BuildingPanelViewData& data) {
    building_panel_.UpdateData(data);
}

void PixelGameHud::UpdateContractPanel(const ContractPanelViewData& data) {
    contract_panel_.UpdateData(data);
}

void PixelGameHud::UpdateNpcDetailPanel(const NpcDetailPanelViewData& data) {
    npc_detail_panel_.UpdateData(data);
}

void PixelGameHud::UpdateSpiritRealmPanel(const SpiritRealmPanelViewData& data) {
    spirit_realm_panel_.UpdateData(data);
}

void PixelGameHud::UpdateBeastiaryPanel(const BeastiaryPanelViewData& data) {
    beastiary_panel_.UpdateData(data);
}

void PixelGameHud::UpdateWorkshopPanel(const WorkshopPanelViewData& data) {
    workshop_panel_.UpdateData(data);
}

void PixelGameHud::PushNotification(const std::string& message) {
    notification_banner_.Push(message);
    if (message.find("成就") != std::string::npos) {
        EmitUiEvent_(UiEventType::Achievement);
    }
}

void PixelGameHud::UpdateSkillBranchOverlay(bool visible,
                                            const std::string& skill_name,
                                            const std::string& option_a,
                                            const std::string& option_b) {
    skill_branch_overlay_.visible = visible;
    skill_branch_overlay_.skill_name = skill_name;
    skill_branch_overlay_.option_a = option_a;
    skill_branch_overlay_.option_b = option_b;
    if (!visible) {
        skill_branch_overlay_.submitted_choice.reset();
    }
}

void PixelGameHud::UpdateFishingQteOverlay(bool visible,
                                           float progress,
                                           float target_center,
                                           float target_width,
                                           const std::string& title) {
    fishing_qte_overlay_.visible = visible;
    fishing_qte_overlay_.progress = std::clamp(progress, 0.0f, 1.0f);
    fishing_qte_overlay_.target_center = std::clamp(target_center, 0.0f, 1.0f);
    fishing_qte_overlay_.target_width = std::clamp(target_width, 0.05f, 0.6f);
    fishing_qte_overlay_.title = title;
    if (!visible) {
        fishing_qte_overlay_.confirm_requested = false;
    }
}

void PixelGameHud::UpdateDiyPlacementOverlay(bool visible,
                                             const std::string& object_name,
                                             int tile_x,
                                             int tile_y,
                                             int rotation) {
    diy_overlay_.visible = visible;
    diy_overlay_.object_name = object_name;
    diy_overlay_.tile_x = tile_x;
    diy_overlay_.tile_y = tile_y;
    diy_overlay_.rotation = rotation;
    if (!visible) {
        diy_overlay_.move_x = 0;
        diy_overlay_.move_y = 0;
        diy_overlay_.rotate_requested = false;
        diy_overlay_.confirm_requested = false;
        diy_overlay_.pickup_requested = false;
    }
}

void PixelGameHud::UpdateDailyRecommendations(const std::vector<std::string>& items) {
    daily_recommendations_ = items;
    if (selected_recommendation_ >= static_cast<int>(daily_recommendations_.size())) {
        selected_recommendation_ = -1;
    }
}

void PixelGameHud::ConfigureNotificationTimings(float fade_in_seconds,
                                                float hold_seconds,
                                                float fade_out_seconds,
                                                float cloud_report_total_seconds) {
    notification_banner_.SetTimings(fade_in_seconds, hold_seconds, fade_out_seconds);
    notification_banner_.SetCloudReportDuration(cloud_report_total_seconds);
}

void PixelGameHud::SetBottomRightHotkeyHints(std::string interact_key, std::string tool_key) {
    if (!interact_key.empty()) {
        interact_key_hint_ = std::move(interact_key);
    }
    if (!tool_key.empty()) {
        tool_key_hint_ = std::move(tool_key);
    }
}

std::optional<bool> PixelGameHud::ConsumeSkillBranchChoice() {
    const auto out = skill_branch_overlay_.submitted_choice;
    skill_branch_overlay_.submitted_choice.reset();
    return out;
}

bool PixelGameHud::ConsumeFishingQteConfirm() {
    const bool out = fishing_qte_overlay_.confirm_requested;
    fishing_qte_overlay_.confirm_requested = false;
    return out;
}

sf::Vector2i PixelGameHud::ConsumeDiyMoveDelta() {
    const sf::Vector2i out{diy_overlay_.move_x, diy_overlay_.move_y};
    diy_overlay_.move_x = 0;
    diy_overlay_.move_y = 0;
    return out;
}

bool PixelGameHud::ConsumeDiyRotate() {
    const bool out = diy_overlay_.rotate_requested;
    diy_overlay_.rotate_requested = false;
    return out;
}

bool PixelGameHud::ConsumeDiyConfirm() {
    const bool out = diy_overlay_.confirm_requested;
    diy_overlay_.confirm_requested = false;
    return out;
}

bool PixelGameHud::ConsumeDiyPickup() {
    const bool out = diy_overlay_.pickup_requested;
    diy_overlay_.pickup_requested = false;
    return out;
}

// ============================================================================
// 【PixelGameHud::SetDialogueBoxCallbacks】
// ============================================================================
void PixelGameHud::SetDialogueBoxCallbacks(PixelDialogueBox::OnCompleteCallback on_complete,
                                             PixelDialogueBox::OnChoiceCallback on_choice) {
    dialogue_box_.SetOnComplete(std::move(on_complete));
    dialogue_box_.SetOnChoice(std::move(on_choice));
}

// ============================================================================
// 【PixelGameHud::HandleKeyPressed】
// ============================================================================
bool PixelGameHud::HandleKeyPressed(const sf::Event::KeyPressed& key) {
    if (context_menu_.IsVisible()) {
        if (key.scancode == sf::Keyboard::Scancode::Escape) {
            context_menu_.CloseMenu();
            return true;
        }
    }
    if (skill_branch_overlay_.visible) {
        if (key.scancode == sf::Keyboard::Scancode::Left || key.scancode == sf::Keyboard::Scancode::A) {
            skill_branch_overlay_.choose_a = true;
            return true;
        }
        if (key.scancode == sf::Keyboard::Scancode::Right || key.scancode == sf::Keyboard::Scancode::D) {
            skill_branch_overlay_.choose_a = false;
            return true;
        }
        if (key.scancode == sf::Keyboard::Scancode::Enter || key.scancode == sf::Keyboard::Scancode::Space) {
            skill_branch_overlay_.submitted_choice = skill_branch_overlay_.choose_a;
            return true;
        }
    }
    if (fishing_qte_overlay_.visible) {
        if (key.scancode == sf::Keyboard::Scancode::Enter || key.scancode == sf::Keyboard::Scancode::Space) {
            fishing_qte_overlay_.confirm_requested = true;
            return true;
        }
    }
    if (diy_overlay_.visible) {
        if (key.scancode == sf::Keyboard::Scancode::Left || key.scancode == sf::Keyboard::Scancode::A) {
            diy_overlay_.move_x -= 1;
            return true;
        }
        if (key.scancode == sf::Keyboard::Scancode::Right || key.scancode == sf::Keyboard::Scancode::D) {
            diy_overlay_.move_x += 1;
            return true;
        }
        if (key.scancode == sf::Keyboard::Scancode::Up || key.scancode == sf::Keyboard::Scancode::W) {
            diy_overlay_.move_y -= 1;
            return true;
        }
        if (key.scancode == sf::Keyboard::Scancode::Down || key.scancode == sf::Keyboard::Scancode::S) {
            diy_overlay_.move_y += 1;
            return true;
        }
        if (key.scancode == sf::Keyboard::Scancode::R) {
            diy_overlay_.rotate_requested = true;
            return true;
        }
        if (key.scancode == sf::Keyboard::Scancode::Enter) {
            diy_overlay_.confirm_requested = true;
            return true;
        }
        if (key.scancode == sf::Keyboard::Scancode::Backspace || key.scancode == sf::Keyboard::Scancode::Delete) {
            diy_overlay_.pickup_requested = true;
            return true;
        }
    }
    if (tutorial_visible_) {
        if (key.scancode == sf::Keyboard::Scancode::Left) {
            tutorial_step_delta_ = -1;
            return true;
        }
        if (key.scancode == sf::Keyboard::Scancode::Right
            || key.scancode == sf::Keyboard::Scancode::Enter
            || key.scancode == sf::Keyboard::Scancode::Space) {
            tutorial_step_delta_ = 1;
            return true;
        }
    }
    if (HandleFocusNavigation_(key)) {
        return true;
    }
    if (key.scancode == sf::Keyboard::Scancode::I) {
        ToggleInventory();
        return true;
    }
    if (key.scancode == sf::Keyboard::Scancode::F) {
        ToggleQuestMenu();
        return true;
    }
    if (key.scancode == sf::Keyboard::Scancode::M) {
        ToggleMap();
        return true;
    }
    if (key.scancode == sf::Keyboard::Scancode::F5) {
        ToggleCloudForecast();
        return true;
    }
    if (key.scancode == sf::Keyboard::Scancode::K) { ToggleTeaGarden(); return true; }
    if ((key.scancode == sf::Keyboard::Scancode::W && key.control)
        || key.scancode == sf::Keyboard::Scancode::F7) {
        ToggleWorkshop();
        return true;
    }
    if (key.scancode == sf::Keyboard::Scancode::H) { ToggleContract(); return true; }
    if (key.scancode == sf::Keyboard::Scancode::N) { ToggleNpcDetail(); return true; }
    if (key.scancode == sf::Keyboard::Scancode::B) { ToggleSpiritBeast(); return true; }
    if (key.scancode == sf::Keyboard::Scancode::F8) { ToggleFestival(); return true; }
    if (key.scancode == sf::Keyboard::Scancode::R) { ToggleSpiritRealm(); return true; }
    if (key.scancode == sf::Keyboard::Scancode::U) { ToggleBuilding(); return true; }
    if (key.scancode == sf::Keyboard::Scancode::O) { ToggleShop(); return true; }
    if (key.scancode == sf::Keyboard::Scancode::P) { ToggleMail(); return true; }
    if (key.scancode == sf::Keyboard::Scancode::L) { ToggleAchievement(); return true; }
    if (key.scancode == sf::Keyboard::Scancode::V) { ToggleBeastiary(); return true; }
    if (mail_panel_.IsVisible()
        && key.scancode == sf::Keyboard::Scancode::Enter
        && panel_action_callbacks_.mail_collect_arrived) {
        panel_action_callbacks_.mail_collect_arrived();
        EmitUiEvent_(UiEventType::Select);
        return true;
    }
    if (contract_panel_.IsVisible() && panel_action_callbacks_.contract_cycle_tracking_volume) {
        if (key.scancode == sf::Keyboard::Scancode::Left || key.scancode == sf::Keyboard::Scancode::Q) {
            panel_action_callbacks_.contract_cycle_tracking_volume(-1);
            EmitUiEvent_(UiEventType::Select);
            return true;
        }
        if (key.scancode == sf::Keyboard::Scancode::Right || key.scancode == sf::Keyboard::Scancode::E) {
            panel_action_callbacks_.contract_cycle_tracking_volume(1);
            EmitUiEvent_(UiEventType::Select);
            return true;
        }
    }
    if (spirit_beast_panel_.IsVisible()
        && key.scancode == sf::Keyboard::Scancode::T
        && panel_action_callbacks_.spirit_beast_toggle_dispatch) {
        panel_action_callbacks_.spirit_beast_toggle_dispatch();
        EmitUiEvent_(UiEventType::Select);
        return true;
    }
    if (key.scancode == sf::Keyboard::Scancode::Escape) {
        CloseAllPanels();
        return true;
    }
    if (key.scancode == sf::Keyboard::Scancode::Tab) {
        const int next_tab = (static_cast<int>(inventory_grid_.GetActiveTab()) + 1)
            % static_cast<int>(InventoryTab::Count);
        inventory_grid_.SetActiveTab(static_cast<InventoryTab>(next_tab));
        if (inventory_grid_.GetActiveTab() != InventoryTab::Social) {
            npc_detail_panel_.SetVisible(false);
        } else if (!inventory_grid_.GetSelectedSocialNpc().has_value()) {
            npc_detail_panel_.SetVisible(false);
        }
        return true;
    }
    if (key.scancode == sf::Keyboard::Scancode::C) {
        TogglePlayerStatus();
        return true;
    }
    if (key.scancode >= sf::Keyboard::Scancode::Num1
        && key.scancode <= sf::Keyboard::Scancode::Num9) {
        const int idx = static_cast<int>(key.scancode) - static_cast<int>(sf::Keyboard::Scancode::Num1);
        toolbar_.SetSelectedSlot(idx);
        inventory_grid_.SetSelectedSlot(idx);
        return true;
    }
    if (key.scancode == sf::Keyboard::Scancode::Left || key.scancode == sf::Keyboard::Scancode::A) {
        toolbar_.MoveSelection(-1);
        return true;
    }
    if (key.scancode == sf::Keyboard::Scancode::Right || key.scancode == sf::Keyboard::Scancode::D) {
        toolbar_.MoveSelection(1);
        return true;
    }
    return false;
}

// ============================================================================
// 【PixelGameHud::HandleMouseMove】
// ============================================================================
void PixelGameHud::HandleMouseMove(float mx, float my) {
    last_mouse_pos_ = {mx, my};
    // UI-129: title-bar dragging for PixelUiPanel based panels.
    settings_panel_.OnMouseMoved(mx, my);
    cloud_forecast_panel_.OnMouseMoved(mx, my);
    player_status_panel_.OnMouseMoved(mx, my);
    tea_garden_panel_.OnMouseMoved(mx, my);
    workshop_panel_.OnMouseMoved(mx, my);
    contract_panel_.OnMouseMoved(mx, my);
    npc_detail_panel_.OnMouseMoved(mx, my);
    spirit_beast_panel_.OnMouseMoved(mx, my);
    festival_panel_.OnMouseMoved(mx, my);
    spirit_realm_panel_.OnMouseMoved(mx, my);
    building_panel_.OnMouseMoved(mx, my);
    shop_panel_.OnMouseMoved(mx, my);
    mail_panel_.OnMouseMoved(mx, my);
    achievement_panel_.OnMouseMoved(mx, my);
    beastiary_panel_.OnMouseMoved(mx, my);

    if (panel_state_.inventory_open) {
        inventory_grid_.MouseHover(mx, my);
    }
    if (panel_state_.quest_menu_open) {
        quest_menu_.MouseHover(mx, my);
    }
    if (context_menu_.IsVisible()) {
        context_menu_.HandleMouseMove(mx, my);
    }
    if (panel_state_.dialogue_open) {
        // 检查选项悬停
        const auto r = dialogue_box_.GetRect();
        const float y0 = r.position.y + r.size.y - 40.0f;
        const int choice_h = 28;
        const int hovered = static_cast<int>((my - y0) / choice_h);
        dialogue_box_.HoverChoice(hovered);
    }

    // Tooltip: delay handled inside PixelTooltip (300ms).
    if (panel_state_.inventory_open && HasFont()) {
        const auto hovered = inventory_grid_.GetHoveredItem();
        if (hovered && !hovered->empty) {
            const std::string item_id = hovered->item_id;
            if (item_id != last_tooltip_item_id_) {
                EmitUiEvent_(UiEventType::Hover);
                tooltip_.SetContent(
                    hovered->item_name,
                    "普通",
                    hovered->description,
                    std::to_string(hovered->price) + " 金币");
                last_tooltip_item_id_ = item_id;
                tooltip_.Show({mx, my});
            }
        } else {
            last_tooltip_item_id_.clear();
            tooltip_.Hide();
        }
    } else {
        last_tooltip_item_id_.clear();
        tooltip_.Hide();
    }
}

// ============================================================================
// 【PixelGameHud::HandleMouseClick】
// ============================================================================
bool PixelGameHud::HandleMouseClick(float mx, float my, sf::Mouse::Button button) {
    if (button == sf::Mouse::Button::Left) {
        // UI-129: begin drag if click is on a panel title bar.
        // Note: order matters (top-most first) - keep roughly "modal" panels earlier.
        if (settings_panel_.OnMousePressed(mx, my)
            || cloud_forecast_panel_.OnMousePressed(mx, my)
            || player_status_panel_.OnMousePressed(mx, my)
            || tea_garden_panel_.OnMousePressed(mx, my)
            || workshop_panel_.OnMousePressed(mx, my)
            || contract_panel_.OnMousePressed(mx, my)
            || npc_detail_panel_.OnMousePressed(mx, my)
            || spirit_beast_panel_.OnMousePressed(mx, my)
            || festival_panel_.OnMousePressed(mx, my)
            || spirit_realm_panel_.OnMousePressed(mx, my)
            || building_panel_.OnMousePressed(mx, my)
            || shop_panel_.OnMousePressed(mx, my)
            || mail_panel_.OnMousePressed(mx, my)
            || achievement_panel_.OnMousePressed(mx, my)
            || beastiary_panel_.OnMousePressed(mx, my)) {
            return true;
        }
    }
    if (button == sf::Mouse::Button::Left) {
        if (notification_banner_.IsCloudReportBannerHit(mx, my, last_window_width_)) {
            if (!panel_state_.cloud_forecast_open) {
                ToggleCloudForecast();
            }
            return true;
        }
    }
    if (tutorial_visible_ && tutorial_bubble_.IsVisible()) {
        const auto r = tutorial_bubble_.GetRect();
        if (r.contains({mx, my})) {
            const float btn_y = r.position.y + r.size.y - 36.0f;
            if (my >= btn_y) {
                if (mx < r.position.x + r.size.x * 0.5f) {
                    tutorial_step_delta_ = -1;
                } else {
                    tutorial_step_delta_ = 1;
                }
                return true;
            }
            return true;
        }
    }
    if (context_menu_.IsVisible() && !panel_state_.inventory_open) {
        (void)context_menu_.HandleMouseClick(mx, my);
        return true;
    }
    if (panel_state_.inventory_open) {
        inventory_grid_.MouseHover(mx, my);
        if (button == sf::Mouse::Button::Right) {
            const auto hovered = inventory_grid_.GetHoveredItem();
            if (hovered && !hovered->empty) {
                std::vector<PixelContextMenu::MenuItem> items;
                const std::string& id = hovered->item_id;
                if (id.find("Tool") != std::string::npos) {
                    items = {{"use", "使用"}, {"drop", "丢弃"}, {"equip", "装备"}};
                } else if (id.find("Seed") != std::string::npos || id.find("seed") != std::string::npos) {
                    items = {{"plant", "种植"}, {"sell", "出售"}, {"drop", "丢弃"}};
                } else {
                    items = {{"use", "使用"}, {"sell", "出售"}, {"drop", "丢弃"}, {"gift", "送礼"}};
                }
                context_menu_.OpenAt({mx, my}, items);
                context_menu_.SetOnSelect([this, hovered](const std::string& action) {
                    EmitUiEvent_(UiEventType::Select);
                    const auto apply = [&](const auto& fn) -> bool {
                        if (!fn) return false;
                        return fn(hovered->item_id, hovered->item_name, hovered->price);
                    };
                    if (action == "drop") {
                        if (!apply(inventory_action_callbacks_.drop_item)) {
                            PushNotification("已丢弃：" + hovered->item_name);
                        }
                    } else if (action == "gift") {
                        if (!apply(inventory_action_callbacks_.gift_item)) {
                            PushNotification("已准备送礼：" + hovered->item_name);
                        }
                    } else if (action == "sell") {
                        if (!apply(inventory_action_callbacks_.sell_item)) {
                            PushNotification("已出售：" + hovered->item_name);
                        }
                    } else if (action == "plant") {
                        PushNotification("已选择种植：" + hovered->item_name);
                    } else if (action == "equip") {
                        PushNotification("已装备：" + hovered->item_name);
                    } else {
                        if (!apply(inventory_action_callbacks_.use_item)) {
                            PushNotification("已使用：" + hovered->item_name);
                        }
                    }
                });
                return true;
            }
            EmitUiEvent_(UiEventType::Error);
            PushNotification("未选中可操作物品。");
            return true;
        }
        if (context_menu_.IsVisible() && context_menu_.HandleMouseClick(mx, my)) {
            EmitUiEvent_(UiEventType::Select);
            return true;
        }
        inventory_grid_.MouseClick(mx, my);
        if (inventory_grid_.GetActiveTab() == InventoryTab::Social) {
            if (const auto sel = inventory_grid_.GetSelectedSocialNpc()) {
                const auto inv_r = inventory_grid_.GetRect();
                const sf::FloatRect detail_r{
                    {inv_r.position.x + inv_r.size.x + kNpcDetailPanelGap, inv_r.position.y},
                    {kNpcDetailPanelWidth, inv_r.size.y}
                };
                npc_detail_panel_.SetRect(detail_r);
                npc_detail_panel_.SetVisible(true);

                NpcDetailPanelViewData d;
                d.name = sel->npc_name;
                d.favor = sel->favor;
                d.heart_level = sel->heart_level;
                npc_detail_panel_.UpdateData(d);
            } else {
                npc_detail_panel_.SetVisible(false);
            }
        } else {
            npc_detail_panel_.SetVisible(false);
        }
        return true;
    }
    if (panel_state_.quest_menu_open) {
        quest_menu_.MouseClick(mx, my);
        return true;
    }
    if (panel_state_.dialogue_open) {
        dialogue_box_.OnMouseClick(mx, my);
        return true;
    }

    // Click top-right info area to open cloud forecast panel.
    const auto it = layout_data_.panels.find("pixel_top_right_info");
    const auto cfg = (it != layout_data_.panels.end())
        ? it->second
        : infrastructure::UiLayoutConfig::GetDefaults().panels["pixel_top_right_info"];
    const sf::FloatRect top_right_rect{ToVec2(cfg.position), ToVec2(cfg.size)};
    if (top_right_rect.contains({mx, my})) {
        ToggleCloudForecast();
        return true;
    }

    // UI-021: recommendations panel click
    if (HasFont()) {
        const sf::FloatRect rec_rect{{16.0f, 260.0f}, {220.0f, 120.0f}};
        if (rec_rect.contains({mx, my})) {
            const float row_h = 28.0f;
            const float y0 = rec_rect.position.y + 28.0f;
            const int idx = static_cast<int>((my - y0) / row_h);
            if (idx >= 0 && idx < static_cast<int>(daily_recommendations_.size())) {
                selected_recommendation_ = idx;
                PushNotification("已标记推荐：" + daily_recommendations_[static_cast<std::size_t>(idx)]);
            }
            return true;
        }
    }
    return false;
}

void PixelGameHud::HandleMouseRelease(float /*mx*/, float /*my*/, sf::Mouse::Button button) {
    if (button != sf::Mouse::Button::Left) return;
    settings_panel_.OnMouseReleased();
    cloud_forecast_panel_.OnMouseReleased();
    player_status_panel_.OnMouseReleased();
    tea_garden_panel_.OnMouseReleased();
    workshop_panel_.OnMouseReleased();
    contract_panel_.OnMouseReleased();
    npc_detail_panel_.OnMouseReleased();
    spirit_beast_panel_.OnMouseReleased();
    festival_panel_.OnMouseReleased();
    spirit_realm_panel_.OnMouseReleased();
    building_panel_.OnMouseReleased();
    shop_panel_.OnMouseReleased();
    mail_panel_.OnMouseReleased();
    achievement_panel_.OnMouseReleased();
    beastiary_panel_.OnMouseReleased();
}

// ============================================================================
// 【PixelGameHud::HandleMouseWheel】
// ============================================================================
bool PixelGameHud::HandleMouseWheel(float mx, float my, float delta) {
    if (panel_state_.inventory_open) {
        inventory_grid_.MouseWheel(mx, my, delta);
        return true;
    }
    if (panel_state_.quest_menu_open) {
        quest_menu_.MouseWheel(mx, my, delta);
        return true;
    }
    if (panel_state_.map_open) {
        minimap_.MouseWheel(mx, my, delta);
        return true;
    }
    return false;
}

void PixelGameHud::RenderSkillBranchOverlay_(sf::RenderWindow& window) {
    if (!skill_branch_overlay_.visible || !HasFont()) {
        return;
    }
    const sf::FloatRect box{{260.0f, 120.0f}, {500.0f, 180.0f}};
    sf::RectangleShape panel(box.size);
    panel.setPosition(box.position);
    panel.setFillColor(sf::Color(249, 242, 225, 236));
    panel.setOutlineThickness(4.0f);
    panel.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(panel);
    font_renderer_->DrawText(window, "技能分支选择", box.position + sf::Vector2f(20.0f, 18.0f), TextStyle::PanelTitle());
    font_renderer_->DrawText(window, "技能：" + skill_branch_overlay_.skill_name,
                             box.position + sf::Vector2f(20.0f, 52.0f), TextStyle::Default());
    font_renderer_->DrawText(window, "左右切换，Enter 确认",
                             box.position + sf::Vector2f(20.0f, 80.0f), TextStyle::HotkeyHint());
    const sf::Vector2f card_size{220.0f, 48.0f};
    auto draw_card = [&](const sf::Vector2f& pos, const std::string& label, bool active) {
        sf::RectangleShape card(card_size);
        card.setPosition(pos);
        card.setFillColor(active ? sf::Color(255, 230, 154) : sf::Color(236, 226, 206));
        card.setOutlineThickness(3.0f);
        card.setOutlineColor(active ? ColorPalette::ActiveGreen : ColorPalette::BrownOutline);
        window.draw(card);
        font_renderer_->DrawWrappedText(window, label, pos + sf::Vector2f(10.0f, 12.0f),
                                        card_size.x - 20.0f, TextStyle::Default(), 1.2f);
    };
    draw_card(box.position + sf::Vector2f(20.0f, 112.0f), "A: " + skill_branch_overlay_.option_a,
              skill_branch_overlay_.choose_a);
    draw_card(box.position + sf::Vector2f(255.0f, 112.0f), "B: " + skill_branch_overlay_.option_b,
              !skill_branch_overlay_.choose_a);
}

void PixelGameHud::RenderFishingQteOverlay_(sf::RenderWindow& window) {
    if (!fishing_qte_overlay_.visible || !HasFont()) {
        return;
    }
    const sf::FloatRect box{{300.0f, 540.0f}, {420.0f, 96.0f}};
    sf::RectangleShape panel(box.size);
    panel.setPosition(box.position);
    panel.setFillColor(sf::Color(30, 43, 64, 228));
    panel.setOutlineThickness(3.0f);
    panel.setOutlineColor(sf::Color(179, 221, 255));
    window.draw(panel);
    font_renderer_->DrawText(window,
                             fishing_qte_overlay_.title.empty() ? "垂钓收线" : fishing_qte_overlay_.title,
                             box.position + sf::Vector2f(16.0f, 10.0f), TextStyle::Default());
    font_renderer_->DrawText(window, "让浮标进入亮区后按 Space / Enter",
                             box.position + sf::Vector2f(16.0f, 30.0f), TextStyle::HotkeyHint());
    const sf::FloatRect bar{{box.position.x + 18.0f, box.position.y + 58.0f}, {box.size.x - 36.0f, 16.0f}};
    sf::RectangleShape bar_bg(bar.size);
    bar_bg.setPosition(bar.position);
    bar_bg.setFillColor(sf::Color(80, 97, 122));
    window.draw(bar_bg);
    const float target_x = bar.position.x + bar.size.x
        * (fishing_qte_overlay_.target_center - fishing_qte_overlay_.target_width * 0.5f);
    sf::RectangleShape target({bar.size.x * fishing_qte_overlay_.target_width, bar.size.y});
    target.setPosition({target_x, bar.position.y});
    target.setFillColor(sf::Color(160, 255, 170));
    window.draw(target);
    sf::RectangleShape cursor({8.0f, 28.0f});
    cursor.setPosition({bar.position.x + bar.size.x * fishing_qte_overlay_.progress - 4.0f, bar.position.y - 6.0f});
    cursor.setFillColor(sf::Color(255, 245, 196));
    cursor.setOutlineThickness(2.0f);
    cursor.setOutlineColor(sf::Color::Black);
    window.draw(cursor);
}

void PixelGameHud::RenderDiyPlacementOverlay_(sf::RenderWindow& window) {
    if (!diy_overlay_.visible || !HasFont()) {
        return;
    }
    const sf::FloatRect box{{20.0f, 420.0f}, {300.0f, 136.0f}};
    sf::RectangleShape panel(box.size);
    panel.setPosition(box.position);
    panel.setFillColor(sf::Color(31, 27, 22, 228));
    panel.setOutlineThickness(3.0f);
    panel.setOutlineColor(sf::Color(214, 183, 133));
    window.draw(panel);
    font_renderer_->DrawText(window, "DIY 摆放模式", box.position + sf::Vector2f(16.0f, 12.0f), TextStyle::Default());
    font_renderer_->DrawText(window, "物件：" + diy_overlay_.object_name,
                             box.position + sf::Vector2f(16.0f, 36.0f), TextStyle::HotkeyHint());
    font_renderer_->DrawText(window,
                             "格位: (" + std::to_string(diy_overlay_.tile_x) + ", "
                                 + std::to_string(diy_overlay_.tile_y) + ")  角度: "
                                 + std::to_string(diy_overlay_.rotation),
                             box.position + sf::Vector2f(16.0f, 58.0f), TextStyle::HotkeyHint());
    font_renderer_->DrawText(window, "方向键移动  R旋转  Enter确认",
                             box.position + sf::Vector2f(16.0f, 84.0f), TextStyle::HotkeyHint());
    font_renderer_->DrawText(window, "Backspace 收回最后摆件",
                             box.position + sf::Vector2f(16.0f, 106.0f), TextStyle::HotkeyHint());
}

// ============================================================================
// 【PixelGameHud::Render】
// ============================================================================
void PixelGameHud::Render(sf::RenderWindow& window) {
    last_window_width_ = static_cast<float>(window.getSize().x);
    if (tutorial_visible_ && HasFont()) {
        RenderTutorialOverlay_(window);
        tutorial_bubble_.Render(window);
    }
    if (HasFont()) {
        RenderDailyRecommendations_(window);
    }
    RenderSkillBranchOverlay_(window);
    RenderFishingQteOverlay_(window);
    RenderDiyPlacementOverlay_(window);
    // 渲染层次 8：右下角状态（体力条/金币）
    RenderBottomRightStatus_(window);

    // 渲染层次 7：右上角信息
    RenderTopRightInfo_(window);

    // 渲染层次 9：工具栏
    toolbar_.Render(window);

    // 各组件自行管理可见性，HUD 统一调用 Render。
    minimap_.Render(window);
    inventory_grid_.Render(window);
    quest_menu_.Render(window);
    settings_panel_.Render(window);
    cloud_forecast_panel_.Render(window);
    player_status_panel_.Render(window);
    tea_garden_panel_.Render(window);
    workshop_panel_.Render(window);
    contract_panel_.Render(window);
    npc_detail_panel_.Render(window);
    spirit_beast_panel_.Render(window);
    festival_panel_.Render(window);
    spirit_realm_panel_.Render(window);
    building_panel_.Render(window);
    shop_panel_.Render(window);
    mail_panel_.Render(window);
    achievement_panel_.Render(window);
    beastiary_panel_.Render(window);
    dialogue_box_.Render(window);
    if (HasFont()) {
        notification_banner_.Render(window, *font_renderer_);
        tooltip_.Render(window, *font_renderer_);
    }
    if (context_menu_.IsVisible()) {
        context_menu_.Render(window);
    }
    RenderFocusRing_(window);

    // 渲染对话框文本（使用 font renderer）
    if (panel_state_.dialogue_open && HasFont()) {
        const auto& dbox = dialogue_box_;
        const auto r = dbox.GetRect();
        const float p = DialogueBoxConfig::TextPadding;
        const float av_size = DialogueBoxConfig::AvatarSize;
        const float tx = r.position.x + p + av_size;
        const float ty = r.position.y + p;

        // NPC 名字
        if (!dbox.GetSpeakerName().empty()) {
            font_renderer_->DrawText(window, dbox.GetSpeakerName(),
                                     tx, ty - 2.0f, TextStyle::NpcName());
        }

        // 对话文字
        font_renderer_->DrawWrappedText(window, dbox.GetCurrentDisplayedText(),
                                        {tx, ty + 18.0f},
                                        r.size.x - p * 2 - av_size,
                                        TextStyle::Dialogue(), 1.5f);

        if (dbox.HasChoices()) {
            const auto& choices = dbox.GetChoices();
            const float choice_y0 = r.position.y + r.size.y - DialogueBoxConfig::ChoiceAreaBottomOffset;
            const float choice_h = DialogueBoxConfig::ChoiceRowHeight;
            for (std::size_t i = 0; i < choices.size(); ++i) {
                const float row_y = choice_y0 + static_cast<float>(i) * choice_h;
                const bool hovered = dbox.GetHoveredChoice() == i;
                const sf::Color row_color = hovered ? ColorPalette::HighlightYellow : ColorPalette::BackgroundWhite;
                sf::VertexArray row_bg(sf::PrimitiveType::Triangles);
                AddQuad(row_bg,
                        {tx, row_y},
                        {r.position.x + r.size.x - p, row_y},
                        {r.position.x + r.size.x - p, row_y + choice_h - 2.0f},
                        {tx, row_y + choice_h - 2.0f},
                        row_color);
                window.draw(row_bg, sf::RenderStates::Default);
                font_renderer_->DrawText(window, choices[i].text, {tx + 6.0f, row_y + 8.0f}, TextStyle::Dialogue());
            }
        }

        // 提示符：打字/确认用不同视觉
        const bool show_indicator = (static_cast<int>(top_right_blink_timer_ * 4.0f) % 2 == 0);
        if (show_indicator && (dbox.GetState() == DialogueBoxState::Typing
            || dbox.GetState() == DialogueBoxState::WaitingConfirm)) {
            TextStyle s = TextStyle::Default();
            const std::string indicator = (dbox.GetState() == DialogueBoxState::Typing) ? "▼" : "[Enter]";
            s.fill_color = (dbox.GetState() == DialogueBoxState::Typing)
                ? ColorPalette::TextBrown
                : ColorPalette::ActiveGreen;
            font_renderer_->DrawText(window, indicator,
                                     r.position.x + r.size.x - p - 64.0f,
                                     r.position.y + r.size.y - p - 14.0f,
                                     s);
        }
    }
}

bool PixelGameHud::HandleFocusNavigation_(const sf::Event::KeyPressed& key) {
    focusables_.clear();
    auto add_focus_rect = [this](sf::FloatRect r) {
        // Ensure minimum 44x44 hotspot
        if (r.size.x < MinTouchTargetSize) {
            const float d = (MinTouchTargetSize - r.size.x) * 0.5f;
            r.position.x -= d; r.size.x = MinTouchTargetSize;
        }
        if (r.size.y < MinTouchTargetSize) {
            const float d = (MinTouchTargetSize - r.size.y) * 0.5f;
            r.position.y -= d; r.size.y = MinTouchTargetSize;
        }
        focusables_.push_back({r});
    };

    if (panel_state_.inventory_open) {
        const auto r = inventory_grid_.GetRect();
        const float tab_w = r.size.x / static_cast<float>(InventoryTab::Count);
        for (int i = 0; i < static_cast<int>(InventoryTab::Count); ++i) {
            add_focus_rect({{r.position.x + tab_w * static_cast<float>(i), r.position.y}, {tab_w, 40.0f}});
        }
    } else {
        const auto tr = toolbar_.GetRect();
        const float slot_w = ToolbarConfig::SlotSize + ToolbarConfig::SlotSpacing;
        for (int i = 0; i < ToolbarConfig::SlotCount; ++i) {
            add_focus_rect({{tr.position.x + slot_w * static_cast<float>(i), tr.position.y}, {ToolbarConfig::SlotSize, ToolbarConfig::SlotSize}});
        }
    }

    if (focusables_.empty()) {
        focus_index_ = -1;
        return false;
    }
    if (focus_index_ < 0 || focus_index_ >= static_cast<int>(focusables_.size())) focus_index_ = 0;

    const bool next = key.scancode == sf::Keyboard::Scancode::Tab
        || key.scancode == sf::Keyboard::Scancode::Right
        || key.scancode == sf::Keyboard::Scancode::Down;
    const bool prev = key.scancode == sf::Keyboard::Scancode::Left
        || key.scancode == sf::Keyboard::Scancode::Up;
    if (next) {
        focus_index_ = (focus_index_ + 1) % static_cast<int>(focusables_.size());
        EmitUiEvent_(UiEventType::Select);
        return true;
    }
    if (prev) {
        focus_index_ = (focus_index_ + static_cast<int>(focusables_.size()) - 1) % static_cast<int>(focusables_.size());
        EmitUiEvent_(UiEventType::Select);
        return true;
    }
    if (key.scancode == sf::Keyboard::Scancode::Enter) {
        if (panel_state_.inventory_open) {
            inventory_grid_.SetActiveTab(static_cast<InventoryTab>(std::clamp(focus_index_, 0, static_cast<int>(InventoryTab::Count) - 1)));
        } else {
            toolbar_.SetSelectedSlot(std::clamp(focus_index_, 0, ToolbarConfig::SlotCount - 1));
        }
        EmitUiEvent_(UiEventType::Select);
        return true;
    }
    return false;
}

void PixelGameHud::RenderFocusRing_(sf::RenderWindow& window) {
    if (focus_index_ < 0 || focus_index_ >= static_cast<int>(focusables_.size())) return;
    const auto& fr = focusables_[static_cast<std::size_t>(focus_index_)].rect;
    const float breath = std::sin(focus_breath_timer_ * (6.2831853f / std::max(0.01f, ControllerConfig::FocusBreathPeriod)))
        * ControllerConfig::FocusBreathAmplitude;
    const float pad = ControllerConfig::NavigationPadding + breath;
    const sf::FloatRect r{{fr.position.x - pad, fr.position.y - pad}, {fr.size.x + pad * 2.0f, fr.size.y + pad * 2.0f}};
    const float t = ControllerConfig::FocusRingThickness;
    sf::VertexArray ring(sf::PrimitiveType::Triangles);
    AddQuad(ring, {r.position.x, r.position.y}, {r.position.x + r.size.x, r.position.y}, {r.position.x + r.size.x, r.position.y + t}, {r.position.x, r.position.y + t}, ColorPalette::ActiveGreen);
    AddQuad(ring, {r.position.x, r.position.y + r.size.y - t}, {r.position.x + r.size.x, r.position.y + r.size.y - t}, {r.position.x + r.size.x, r.position.y + r.size.y}, {r.position.x, r.position.y + r.size.y}, ColorPalette::ActiveGreen);
    AddQuad(ring, {r.position.x, r.position.y}, {r.position.x + t, r.position.y}, {r.position.x + t, r.position.y + r.size.y}, {r.position.x, r.position.y + r.size.y}, ColorPalette::ActiveGreen);
    AddQuad(ring, {r.position.x + r.size.x - t, r.position.y}, {r.position.x + r.size.x, r.position.y}, {r.position.x + r.size.x, r.position.y + r.size.y}, {r.position.x + r.size.x - t, r.position.y + r.size.y}, ColorPalette::ActiveGreen);
    window.draw(ring, sf::RenderStates::Default);
}

void PixelGameHud::RenderTutorialOverlay_(sf::RenderWindow& window) {
    const sf::Vector2f screen{ScreenConfig::Width, ScreenConfig::Height};
    const sf::FloatRect h = tutorial_highlight_rect_;

    sf::VertexArray mask(sf::PrimitiveType::Triangles);
    const sf::Color shade(0, 0, 0, 180);

    const float left = std::max(0.0f, h.position.x);
    const float top = std::max(0.0f, h.position.y);
    const float right = std::min(screen.x, h.position.x + h.size.x);
    const float bottom = std::min(screen.y, h.position.y + h.size.y);

    // top
    AddQuad(mask, {0.0f, 0.0f}, {screen.x, 0.0f}, {screen.x, top}, {0.0f, top}, shade);
    // bottom
    AddQuad(mask, {0.0f, bottom}, {screen.x, bottom}, {screen.x, screen.y}, {0.0f, screen.y}, shade);
    // left
    AddQuad(mask, {0.0f, top}, {left, top}, {left, bottom}, {0.0f, bottom}, shade);
    // right
    AddQuad(mask, {right, top}, {screen.x, top}, {screen.x, bottom}, {right, bottom}, shade);
    window.draw(mask, sf::RenderStates::Default);

    // highlight border
    sf::VertexArray border(sf::PrimitiveType::Triangles);
    AddQuad(border, {left, top}, {right, top}, {right, top + 2.0f}, {left, top + 2.0f}, ColorPalette::ActiveGreen);
    AddQuad(border, {left, bottom - 2.0f}, {right, bottom - 2.0f}, {right, bottom}, {left, bottom}, ColorPalette::ActiveGreen);
    AddQuad(border, {left, top}, {left + 2.0f, top}, {left + 2.0f, bottom}, {left, bottom}, ColorPalette::ActiveGreen);
    AddQuad(border, {right - 2.0f, top}, {right, top}, {right, bottom}, {right - 2.0f, bottom}, ColorPalette::ActiveGreen);
    window.draw(border, sf::RenderStates::Default);

    // arrow: simple triangle from bubble to highlight center
    const sf::Vector2f bubble_center = tutorial_bubble_.GetRect().position + tutorial_bubble_.GetRect().size * 0.5f;
    const sf::Vector2f target{(left + right) * 0.5f, (top + bottom) * 0.5f};
    const sf::Vector2f mid{(bubble_center.x + target.x) * 0.5f, (bubble_center.y + target.y) * 0.5f};
    sf::VertexArray tri(sf::PrimitiveType::Triangles);
    AddQuad(tri,
            {mid.x - 6.0f, mid.y - 6.0f},
            {mid.x + 6.0f, mid.y - 6.0f},
            {mid.x + 6.0f, mid.y + 6.0f},
            {mid.x - 6.0f, mid.y + 6.0f},
            ColorPalette::HighlightYellow);
    window.draw(tri, sf::RenderStates::Default);
}

void PixelGameHud::RenderDailyRecommendations_(sf::RenderWindow& window) {
    const sf::FloatRect r{{16.0f, 260.0f}, {220.0f, 120.0f}};
    sf::VertexArray bg(sf::PrimitiveType::Triangles);
    AddQuad(bg,
            {r.position.x, r.position.y},
            {r.position.x + r.size.x, r.position.y},
            {r.position.x + r.size.x, r.position.y + r.size.y},
            {r.position.x, r.position.y + r.size.y},
            ColorPalette::Cream);
    AddQuad(bg,
            {r.position.x, r.position.y},
            {r.position.x + r.size.x, r.position.y},
            {r.position.x + r.size.x, r.position.y + 1.0f},
            {r.position.x, r.position.y + 1.0f},
            ColorPalette::BrownOutline);
    AddQuad(bg,
            {r.position.x, r.position.y + r.size.y - 1.0f},
            {r.position.x + r.size.x, r.position.y + r.size.y - 1.0f},
            {r.position.x + r.size.x, r.position.y + r.size.y},
            {r.position.x, r.position.y + r.size.y},
            ColorPalette::BrownOutline);
    AddQuad(bg,
            {r.position.x, r.position.y},
            {r.position.x + 1.0f, r.position.y},
            {r.position.x + 1.0f, r.position.y + r.size.y},
            {r.position.x, r.position.y + r.size.y},
            ColorPalette::BrownOutline);
    AddQuad(bg,
            {r.position.x + r.size.x - 1.0f, r.position.y},
            {r.position.x + r.size.x, r.position.y},
            {r.position.x + r.size.x, r.position.y + r.size.y},
            {r.position.x + r.size.x - 1.0f, r.position.y + r.size.y},
            ColorPalette::BrownOutline);
    window.draw(bg, sf::RenderStates::Default);

    TextStyle title = TextStyle::PanelTitle();
    title.character_size = 14;
    font_renderer_->DrawText(window, "今日推荐", {r.position.x + 10.0f, r.position.y + 6.0f}, title);

    TextStyle item = TextStyle::Default();
    item.character_size = 12;
    item.fill_color = ColorPalette::TextBrown;
    const float y0 = r.position.y + 32.0f;
    const float row_h = 28.0f;
    const int count = std::min(3, static_cast<int>(daily_recommendations_.size()));
    for (int i = 0; i < count; ++i) {
        const float ry = y0 + static_cast<float>(i) * row_h;
        if (selected_recommendation_ == i) {
            sf::VertexArray hi(sf::PrimitiveType::Triangles);
            AddQuad(hi,
                    {r.position.x + 6.0f, ry - 2.0f},
                    {r.position.x + r.size.x - 6.0f, ry - 2.0f},
                    {r.position.x + r.size.x - 6.0f, ry + row_h - 4.0f},
                    {r.position.x + 6.0f, ry + row_h - 4.0f},
                    ColorPalette::HighlightYellow);
            window.draw(hi, sf::RenderStates::Default);
        }
        font_renderer_->DrawWrappedText(window,
                                        daily_recommendations_[static_cast<std::size_t>(i)],
                                        {r.position.x + 10.0f, ry},
                                        r.size.x - 20.0f,
                                        item,
                                        1.2f);
    }
}

// ============================================================================
// 【PixelGameHud::RenderTopRightInfo_】
// ============================================================================
void PixelGameHud::RenderTopRightInfo_(sf::RenderWindow& window) {
    const auto it = layout_data_.panels.find("pixel_top_right_info");
    const auto cfg = (it != layout_data_.panels.end())
        ? it->second
        : infrastructure::UiLayoutConfig::GetDefaults().panels["pixel_top_right_info"];
    const sf::Vector2f cfg_pos = ToVec2(cfg.position);
    const sf::Vector2f cfg_size = ToVec2(cfg.size);
    const sf::Color fill = Uint32ToSfColor(cfg.fill_color);
    const sf::Color border = Uint32ToSfColor(cfg.outline_color);

    if (top_right_geometry_dirty_) {
        top_right_bg_.clear();

        // 背景
        AddQuad(top_right_bg_,
                {cfg_pos.x, cfg_pos.y},
                {cfg_pos.x + cfg_size.x, cfg_pos.y},
                {cfg_pos.x + cfg_size.x, cfg_pos.y + cfg_size.y},
                {cfg_pos.x, cfg_pos.y + cfg_size.y},
                fill);

        // 像素边框
        const float cs = PixelBorderConfig::CornerBlockSize;
        const float t = PixelBorderConfig::BorderThickness;
        AddQuad(top_right_bg_,
                {cfg_pos.x, cfg_pos.y},
                {cfg_pos.x + cs, cfg_pos.y},
                {cfg_pos.x + cs, cfg_pos.y + cs},
                {cfg_pos.x, cfg_pos.y + cs},
                border);
        AddQuad(top_right_bg_,
                {cfg_pos.x + cfg_size.x - cs, cfg_pos.y},
                {cfg_pos.x + cfg_size.x, cfg_pos.y},
                {cfg_pos.x + cfg_size.x, cfg_pos.y + cs},
                {cfg_pos.x + cfg_size.x - cs, cfg_pos.y + cs},
                border);
        AddQuad(top_right_bg_,
                {cfg_pos.x, cfg_pos.y + cfg_size.y - cs},
                {cfg_pos.x + cs, cfg_pos.y + cfg_size.y - cs},
                {cfg_pos.x + cs, cfg_pos.y + cfg_size.y},
                {cfg_pos.x, cfg_pos.y + cfg_size.y},
                border);
        AddQuad(top_right_bg_,
                {cfg_pos.x + cfg_size.x - cs, cfg_pos.y + cfg_size.y - cs},
                {cfg_pos.x + cfg_size.x, cfg_pos.y + cfg_size.y - cs},
                {cfg_pos.x + cfg_size.x, cfg_pos.y + cfg_size.y},
                {cfg_pos.x + cfg_size.x - cs, cfg_pos.y + cfg_size.y},
                border);

        AddQuad(top_right_bg_,
                {cfg_pos.x + cs, cfg_pos.y},
                {cfg_pos.x + cfg_size.x - cs, cfg_pos.y},
                {cfg_pos.x + cfg_size.x - cs, cfg_pos.y + t},
                {cfg_pos.x + cs, cfg_pos.y + t},
                border);
        AddQuad(top_right_bg_,
                {cfg_pos.x + cs, cfg_pos.y + cfg_size.y - t},
                {cfg_pos.x + cfg_size.x - cs, cfg_pos.y + cfg_size.y - t},
                {cfg_pos.x + cfg_size.x - cs, cfg_pos.y + cfg_size.y},
                {cfg_pos.x + cs, cfg_pos.y + cfg_size.y},
                border);
        AddQuad(top_right_bg_,
                {cfg_pos.x, cfg_pos.y + cs},
                {cfg_pos.x + t, cfg_pos.y + cs},
                {cfg_pos.x + t, cfg_pos.y + cfg_size.y - cs},
                {cfg_pos.x, cfg_pos.y + cfg_size.y - cs},
                border);
        AddQuad(top_right_bg_,
                {cfg_pos.x + cfg_size.x - t, cfg_pos.y + cs},
                {cfg_pos.x + cfg_size.x, cfg_pos.y + cs},
                {cfg_pos.x + cfg_size.x, cfg_pos.y + cfg_size.y - cs},
                {cfg_pos.x + cfg_size.x - t, cfg_pos.y + cfg_size.y - cs},
                border);

        top_right_geometry_dirty_ = false;
    }

    window.draw(top_right_bg_, sf::RenderStates::Default);

    // 渲染文本
    if (HasFont()) {
        const auto t_it = layout_data_.texts.find("pixel_top_right_time");
        const auto s_it = layout_data_.texts.find("pixel_top_right_season");
        const auto w_it = layout_data_.texts.find("pixel_top_right_weather");
        const auto defaults = infrastructure::UiLayoutConfig::GetDefaults();

        const auto tcfg = (t_it != layout_data_.texts.end()) ? t_it->second : defaults.texts.at("pixel_top_right_time");
        const auto scfg = (s_it != layout_data_.texts.end()) ? s_it->second : defaults.texts.at("pixel_top_right_season");
        const auto wcfg = (w_it != layout_data_.texts.end()) ? w_it->second : defaults.texts.at("pixel_top_right_weather");

        const TextStyle style = ToPixelTextStyle(tcfg, TextStyle::TopRightInfo());
        font_renderer_->DrawText(window, top_right_time_text_, ToVec2(tcfg.position), style);
        font_renderer_->DrawText(window, top_right_season_text_, ToVec2(scfg.position), TextStyle::TopRightInfo());
        font_renderer_->DrawText(window, top_right_weather_text_, ToVec2(wcfg.position), TextStyle::TopRightInfo());
        DrawUiFrame_(window, atlas::WeatherIconForText(top_right_weather_text_),
                     {cfg_pos.x + 8.0f, cfg_pos.y + 6.0f}, {14.0f, 14.0f});

        // 任务感叹号（新任务时闪烁）
        if (top_right_has_new_quest_) {
            const bool show = (static_cast<int>(top_right_blink_timer_ * 2.0f) % 2 == 0);
            if (show) {
                DrawUiFrame_(window, atlas::kIconQuest,
                             {cfg_pos.x + cfg_size.x - 32.0f, cfg_pos.y + 6.0f}, {14.0f, 14.0f});
                font_renderer_->DrawText(window, "!",
                                         cfg_pos.x + cfg_size.x - 16.0f,
                                         cfg_pos.y + 8.0f,
                                         TextStyle::Default());
            }
        }
    }
}

// ============================================================================
// 【PixelGameHud::RenderBottomRightStatus_】
// ============================================================================
void PixelGameHud::RenderBottomRightStatus_(sf::RenderWindow& window) {
    const auto it = layout_data_.panels.find("pixel_bottom_right_status");
    const auto cfg = (it != layout_data_.panels.end())
        ? it->second
        : infrastructure::UiLayoutConfig::GetDefaults().panels["pixel_bottom_right_status"];
    const sf::Vector2f cfg_pos = ToVec2(cfg.position);
    const sf::Vector2f cfg_size = ToVec2(cfg.size);
    const sf::Color fill = Uint32ToSfColor(cfg.fill_color);
    const sf::Color border = Uint32ToSfColor(cfg.outline_color);

    if (coin_geometry_dirty_) {
        coin_bg_.clear();

        AddQuad(coin_bg_,
                {cfg_pos.x, cfg_pos.y},
                {cfg_pos.x + cfg_size.x, cfg_pos.y},
                {cfg_pos.x + cfg_size.x, cfg_pos.y + cfg_size.y},
                {cfg_pos.x, cfg_pos.y + cfg_size.y},
                fill);

        const float cs = PixelBorderConfig::CornerBlockSize;
        const float t = PixelBorderConfig::BorderThickness;
        AddQuad(coin_bg_,
                {cfg_pos.x, cfg_pos.y},
                {cfg_pos.x + cs, cfg_pos.y},
                {cfg_pos.x + cs, cfg_pos.y + cs},
                {cfg_pos.x, cfg_pos.y + cs},
                border);
        AddQuad(coin_bg_,
                {cfg_pos.x + cfg_size.x - cs, cfg_pos.y},
                {cfg_pos.x + cfg_size.x, cfg_pos.y},
                {cfg_pos.x + cfg_size.x, cfg_pos.y + cs},
                {cfg_pos.x + cfg_size.x - cs, cfg_pos.y + cs},
                border);
        AddQuad(coin_bg_,
                {cfg_pos.x, cfg_pos.y + cfg_size.y - cs},
                {cfg_pos.x + cs, cfg_pos.y + cfg_size.y - cs},
                {cfg_pos.x + cs, cfg_pos.y + cfg_size.y},
                {cfg_pos.x, cfg_pos.y + cfg_size.y},
                border);
        AddQuad(coin_bg_,
                {cfg_pos.x + cfg_size.x - cs, cfg_pos.y + cfg_size.y - cs},
                {cfg_pos.x + cfg_size.x, cfg_pos.y + cfg_size.y - cs},
                {cfg_pos.x + cfg_size.x, cfg_pos.y + cfg_size.y},
                {cfg_pos.x + cfg_size.x - cs, cfg_pos.y + cfg_size.y},
                border);

        AddQuad(coin_bg_,
                {cfg_pos.x + cs, cfg_pos.y},
                {cfg_pos.x + cfg_size.x - cs, cfg_pos.y},
                {cfg_pos.x + cfg_size.x - cs, cfg_pos.y + t},
                {cfg_pos.x + cs, cfg_pos.y + t},
                border);
        AddQuad(coin_bg_,
                {cfg_pos.x + cs, cfg_pos.y + cfg_size.y - t},
                {cfg_pos.x + cfg_size.x - cs, cfg_pos.y + cfg_size.y - t},
                {cfg_pos.x + cfg_size.x - cs, cfg_pos.y + cfg_size.y},
                {cfg_pos.x + cs, cfg_pos.y + cfg_size.y},
                border);
        AddQuad(coin_bg_,
                {cfg_pos.x, cfg_pos.y + cs},
                {cfg_pos.x + t, cfg_pos.y + cs},
                {cfg_pos.x + t, cfg_pos.y + cfg_size.y - cs},
                {cfg_pos.x, cfg_pos.y + cfg_size.y - cs},
                border);
        AddQuad(coin_bg_,
                {cfg_pos.x + cfg_size.x - t, cfg_pos.y + cs},
                {cfg_pos.x + cfg_size.x, cfg_pos.y + cs},
                {cfg_pos.x + cfg_size.x, cfg_pos.y + cfg_size.y - cs},
                {cfg_pos.x + cfg_size.x - t, cfg_pos.y + cfg_size.y - cs},
                border);

        coin_geometry_dirty_ = false;
    }

    window.draw(coin_bg_, sf::RenderStates::Default);
    stamina_bar_.Render(window);
    hunger_bar_.Render(window);

    // 金币文本
    if (HasFont()) {
        const auto defaults = infrastructure::UiLayoutConfig::GetDefaults();

        const auto c_it = layout_data_.texts.find("pixel_coin");
        const auto& ccfg = (c_it != layout_data_.texts.end()) ? c_it->second : defaults.texts.at("pixel_coin");
        const auto FormatCoins = [](int value) {
            const bool negative = value < 0;
            std::string digits = std::to_string(negative ? -value : value);
            std::string out;
            for (std::size_t i = 0; i < digits.size(); ++i) {
                if (i > 0 && (digits.size() - i) % 3 == 0) {
                    out.push_back(',');
                }
                out.push_back(digits[i]);
            }
            if (negative) out.insert(out.begin(), '-');
            return out;
        };

        const sf::Vector2f coin_pos = ToVec2(ccfg.position);
        DrawUiFrame_(window, atlas::kIconCoin,
                     {coin_pos.x, coin_pos.y + 2.0f}, {12.0f, 12.0f});
        DrawUiFrame_(window, atlas::kIconStamina,
                     {cfg_pos.x + 188.0f, cfg_pos.y + 4.0f}, {14.0f, 14.0f});
        font_renderer_->DrawText(window,
                                 FormatCoins(coin_amount_),
                                 {coin_pos.x + 16.0f, coin_pos.y},
                                 ToPixelTextStyle(ccfg, TextStyle::CoinText()));

        font_renderer_->DrawText(window,
                                 "[" + interact_key_hint_ + "] 交互  [" + tool_key_hint_ + "] 工具",
                                 {cfg_pos.x + 10.0f, cfg_pos.y + cfg_size.y - 14.0f},
                                 TextStyle::HotkeyHint());
        font_renderer_->DrawText(window,
                                 stamina_bar_.GetProgress() > 0.0f
                                     ? std::to_string(static_cast<int>(stamina_bar_.GetProgress() * 100.0f)) + "%"
                                     : "0%",
                                 {cfg_pos.x + 220.0f, cfg_pos.y + 6.0f},
                                 TextStyle::TopRightInfo());
        font_renderer_->DrawText(window,
                                 hunger_bar_.GetProgress() > 0.0f
                                     ? std::to_string(static_cast<int>(hunger_bar_.GetProgress() * 100.0f)) + "%"
                                     : "0%",
                                 {cfg_pos.x + 220.0f, cfg_pos.y + 22.0f},
                                 TextStyle::TopRightInfo());

        if (low_stamina_active_) {
            const auto w_it = layout_data_.texts.find("pixel_stamina_warning");
            const auto& wcfg = (w_it != layout_data_.texts.end()) ? w_it->second : defaults.texts.at("pixel_stamina_warning");
            const bool blink = (static_cast<int>(top_right_blink_timer_ * (critical_stamina_active_ ? 6.0f : 3.0f)) % 2 == 0);
            if (blink) {
                const char* warning_text = critical_stamina_active_ ? "!! 体力危险 !!" : "! 体力偏低 !";
                font_renderer_->DrawText(window, warning_text,
                                         ToVec2(wcfg.position),
                                         ToPixelTextStyle(wcfg, TextStyle::Default()));
            }
        }
    }
}

}  // namespace CloudSeamanor::engine
