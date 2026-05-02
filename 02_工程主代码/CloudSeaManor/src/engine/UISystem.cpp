#include "CloudSeamanor/engine/UISystem.hpp"
#include "CloudSeamanor/infrastructure/GameConstants.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"
#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/Profiling.hpp"
#include "CloudSeamanor/engine/rendering/UiPanelInitializer.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace CloudSeamanor::engine {

namespace {

const infrastructure::PanelLayout& GetPanelOrFallback(
    const infrastructure::UiLayoutData& layout,
    const char* key) {
    const auto it = layout.panels.find(key);
    if (it != layout.panels.end()) {
        return it->second;
    }
    infrastructure::Logger::Warning(std::string("UISystem: 缺少 panels 配置项: ") + key);
    static const infrastructure::PanelLayout kEmpty{};
    return kEmpty;
}

float GetSemanticNumberOrFallback(
    const infrastructure::UiLayoutData& layout,
    const char* key,
    float fallback) {
    const auto it = layout.semantic_numbers.find(key);
    return it != layout.semantic_numbers.end() ? it->second : fallback;
}

}  // namespace

// ============================================================================
// 【UISystem::InitializePanels】初始化所有面板
// ============================================================================
void UISystem::InitializePanels(const infrastructure::UiLayoutConfig* config) {
    layout_data_ = config ? config->Data()
                          : infrastructure::UiLayoutConfig::GetDefaults();
    UiPanelInitializer::InitializePanels(panels_, config);
}

// ============================================================================
// 【UISystem::InitializeTexts】初始化所有文本
// ============================================================================
void UISystem::InitializeTexts(const sf::Font& font, const infrastructure::UiLayoutConfig* config) {
    layout_data_ = config ? config->Data()
                          : infrastructure::UiLayoutConfig::GetDefaults();

    font_loaded_ = font.getInfo().family != "";
    font_ptr_ = &font;

    if (!font_loaded_) {
        return;
    }

    UiPanelInitializer::InitializeTexts(font, texts_, config);

    MarkAllDirty();
}

// ============================================================================
// 【UISystem::ReloadFont】重新加载字体
// ============================================================================
void UISystem::ReloadFont(const sf::Font& font) {
    font_ptr_ = &font;
    font_loaded_ = font.getInfo().family != "";
    if (font_loaded_) {
        if (texts_.hud_text) texts_.hud_text->setFont(font);
        if (texts_.inventory_text) texts_.inventory_text->setFont(font);
        if (texts_.hint_text) texts_.hint_text->setFont(font);
        if (texts_.dialogue_text) texts_.dialogue_text->setFont(font);
        if (texts_.debug_text) texts_.debug_text->setFont(font);
        if (texts_.world_tip_text) texts_.world_tip_text->setFont(font);
        if (texts_.festival_notice_text) texts_.festival_notice_text->setFont(font);
        if (texts_.level_up_text) texts_.level_up_text->setFont(font);
    }
    MarkAllDirty();
}

// ============================================================================
// 【MarkDirty / MarkAllDirty】脏标记
// ============================================================================
void UISystem::MarkDirty(UiTextType type) {
    dirty_flags_[static_cast<std::size_t>(type)] = true;
}

void UISystem::MarkAllDirty() {
    for (auto& flag : dirty_flags_) {
        flag = true;
    }
}

// ============================================================================
// 【UpdateHudText】更新 HUD 文本
// ============================================================================
void UISystem::UpdateHudText(const std::string& text) {
    if (!font_loaded_ || !texts_.hud_text) return;
    texts_.hud_text->setString(sf::String::fromUtf8(text.begin(), text.end()));
    MarkDirty(UiTextType::Hud);
}

// ============================================================================
// 【UpdateInventoryText】更新背包文本
// ============================================================================
void UISystem::UpdateInventoryText(const std::string& text) {
    if (!font_loaded_ || !texts_.inventory_text) return;
    texts_.inventory_text->setString(sf::String::fromUtf8(text.begin(), text.end()));
    MarkDirty(UiTextType::Inventory);
}

// ============================================================================
// 【UpdateHintText】更新提示文本
// ============================================================================
void UISystem::UpdateHintText(const std::string& text) {
    UpdateHintText(text, HintType::Normal);
}

// ============================================================================
// 【UpdateHintText】更新提示文本（带类型）
// ============================================================================
void UISystem::UpdateHintText(const std::string& text, CloudSeamanor::engine::HintType type) {
    if (!font_loaded_ || !texts_.hint_text) return;
    texts_.hint_text->setString(sf::String::fromUtf8(text.begin(), text.end()));

    // QA-015: 根据提示类型设置颜色
    switch (type) {
        case HintType::Warning:
            texts_.hint_text->setFillColor(sf::Color(255, 80, 80));  // 红色警告
            break;
        case HintType::Success:
            texts_.hint_text->setFillColor(sf::Color(80, 255, 120));  // 绿色成功
            break;
        case HintType::Info:
            texts_.hint_text->setFillColor(sf::Color(100, 180, 255));  // 蓝色信息
            break;
        case HintType::Normal:
        default:
            texts_.hint_text->setFillColor(sf::Color(220, 220, 220));  // 默认灰色
            break;
    }

    MarkDirty(UiTextType::Hint);
}

// ============================================================================
// 【UpdateDialogueText】更新对话文本
// ============================================================================
void UISystem::UpdateDialogueText(const std::string& text) {
    if (!font_loaded_ || !texts_.dialogue_text) return;
    texts_.dialogue_text->setString(sf::String::fromUtf8(text.begin(), text.end()));
    MarkDirty(UiTextType::Dialogue);
}

// ============================================================================
// 【UpdateDebugText】更新调试文本
// ============================================================================
void UISystem::UpdateDebugText(const std::string& text) {
    if (!font_loaded_ || !texts_.debug_text) return;
    texts_.debug_text->setString(sf::String::fromUtf8(text.begin(), text.end()));
    MarkDirty(UiTextType::Debug);
}

// ============================================================================
// 【UpdateWorldTipText】更新世界提示文本
// ============================================================================
void UISystem::UpdateWorldTipText(const std::string& text) {
    if (!font_loaded_ || !texts_.world_tip_text) return;
    texts_.world_tip_text->setString(sf::String::fromUtf8(text.begin(), text.end()));
    MarkDirty(UiTextType::WorldTip);
}

// ============================================================================
// 【UpdateFestivalNoticeText】更新节日通知文本
// ============================================================================
void UISystem::UpdateFestivalNoticeText(const std::string& text) {
    if (!font_loaded_ || !texts_.festival_notice_text) return;
    texts_.festival_notice_text->setString(sf::String::fromUtf8(text.begin(), text.end()));
    MarkDirty(UiTextType::FestivalNotice);
}

// ============================================================================
// 【UpdateLevelUpText】更新升级提示文本
// ============================================================================
void UISystem::UpdateLevelUpText(const std::string& text) {
    if (!font_loaded_ || !texts_.level_up_text) return;
    texts_.level_up_text->setString(sf::String::fromUtf8(text.begin(), text.end()));
    MarkDirty(UiTextType::LevelUp);
}

// ============================================================================
// 【UpdateStaminaBar】更新体力条
// ============================================================================
void UISystem::UpdateStaminaBar(float ratio) {
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    const auto& stamina = GetPanelOrFallback(layout_data_, "stamina_bar");
    auto sz = stamina.size;
    panels_.stamina_bar_fill.setSize({sz[0] * ratio, sz[1]});
}

// ============================================================================
// 【UpdateWorkshopProgressBar】更新工坊进度条
// ============================================================================
void UISystem::UpdateWorkshopProgressBar(float ratio) {
    const auto& workshop = GetPanelOrFallback(layout_data_, "workshop_progress");
    auto sz = workshop.size;
    if (ratio < 0.0f) {
        panels_.workshop_progress_fill.setSize({0.0f, sz[1]});
        return;
    }
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    panels_.workshop_progress_fill.setSize({sz[0] * ratio, sz[1]});
}

// ============================================================================
// 【UpdateAuraOverlay】更新灵气覆盖层颜色
// ============================================================================
void UISystem::UpdateAuraOverlay(const sf::Color& color) {
    panels_.aura_overlay.setFillColor(color);
}

// ============================================================================
// 【UpdateWorldTipPulse】更新世界提示脉冲
// ============================================================================
void UISystem::UpdateWorldTipPulse(float pulse) {
    const float p = GameConstants::Ui::Pulse::Bias
        + GameConstants::Ui::Pulse::Amplitude
            * std::sin(pulse * GameConstants::Ui::Pulse::Frequency);
    if (texts_.world_tip_text) {
        const auto it = layout_data_.texts.find("world_tip");
        const auto world_tip_style = (it != layout_data_.texts.end())
            ? it->second
            : infrastructure::UiLayoutConfig::GetDefaults().texts["world_tip"];
        const sf::Color base = sf::Color(
            static_cast<std::uint8_t>((world_tip_style.color >> 24) & 0xFF),
            static_cast<std::uint8_t>((world_tip_style.color >> 16) & 0xFF),
            static_cast<std::uint8_t>((world_tip_style.color >> 8) & 0xFF),
            static_cast<std::uint8_t>(world_tip_style.color & 0xFF));
        texts_.world_tip_text->setFillColor(
            sf::Color(
                base.r, base.g, base.b,
                static_cast<std::uint8_t>(
                    static_cast<float>(GameConstants::Ui::Pulse::AlphaBase)
                    + p * GameConstants::Ui::Pulse::AlphaRange)));

        const float wave_amplitude = GetSemanticNumberOrFallback(
            layout_data_,
            "world_tip_wave_amplitude",
            2.0f);
        const float wave_period = std::max(
            0.01f,
            GetSemanticNumberOrFallback(
                layout_data_,
                "world_tip_wave_period",
                0.5f));
        const float wave = std::sin(
                               pulse
                               * (2.0f * std::numbers::pi_v<float> / wave_period))
            * wave_amplitude;
        texts_.world_tip_text->setPosition({world_tip_style.position[0], world_tip_style.position[1] + wave});
    }
}

// ============================================================================
// 【UpdateLevelUpAnimation】更新升级动画
// ============================================================================
void UISystem::UpdateLevelUpAnimation(float remaining_time) {
    if (!texts_.level_up_text) return;
    const float alpha = std::min(1.0f, remaining_time / GameConstants::Ui::LevelUp::FadeDuration);
    const float scale = 1.0f + (1.0f - alpha) * GameConstants::Ui::LevelUp::MaxScaleOffset;
    texts_.level_up_text->setScale({scale, scale});
    const auto it = layout_data_.texts.find("level_up");
    const auto pos = (it != layout_data_.texts.end())
        ? it->second.position
        : infrastructure::UiLayoutConfig::GetDefaults().texts["level_up"].position;
    const float y_offset = (1.0f - alpha) * GameConstants::Ui::LevelUp::FloatDistance;
    texts_.level_up_text->setPosition({pos[0], pos[1] - GameConstants::Ui::LevelUp::FloatDistance + y_offset});
}

// ============================================================================
// 【HideLevelUpText】隐藏升级提示
// ============================================================================
void UISystem::HideLevelUpText() {
    if (texts_.level_up_text) {
        texts_.level_up_text->setString(sf::String{});
    }
}

// ============================================================================
// 【Render】渲染所有 UI
// ============================================================================
void UISystem::Render(sf::RenderWindow& window, const UiRenderOptions& opts) {
    CSM_ZONE_SCOPED;
    RenderBackgroundLayer_(window);
    RenderPanels_(window);
    if (opts.show_festival && texts_.festival_notice_text) {
        window.draw(*texts_.festival_notice_text);
    }
    if (font_loaded_) {
        if (texts_.hud_text) window.draw(*texts_.hud_text);
        if (texts_.inventory_text) window.draw(*texts_.inventory_text);
        if (texts_.hint_text) window.draw(*texts_.hint_text);
        if (texts_.dialogue_text) window.draw(*texts_.dialogue_text);
        if (texts_.world_tip_text) window.draw(*texts_.world_tip_text);
        if (opts.show_level_up && texts_.level_up_text) window.draw(*texts_.level_up_text);
        if (opts.show_debug && texts_.debug_text) window.draw(*texts_.debug_text);
    }
    RenderProgressBars_(window);
}

// ============================================================================
// 【RenderHudOnly】仅渲染 HUD（调试用）
// ============================================================================
void UISystem::RenderHudOnly(sf::RenderWindow& window) {
    window.draw(panels_.main_panel);
    window.draw(panels_.inventory_panel);
    window.draw(panels_.dialogue_panel);
    window.draw(panels_.hint_panel);
    if (font_loaded_) {
        if (texts_.hud_text) window.draw(*texts_.hud_text);
        if (texts_.inventory_text) window.draw(*texts_.inventory_text);
        if (texts_.hint_text) window.draw(*texts_.hint_text);
        if (texts_.dialogue_text) window.draw(*texts_.dialogue_text);
    }
}

// ============================================================================
// 【RenderBackgroundLayer_】渲染背景灵气层
// ============================================================================
void UISystem::RenderBackgroundLayer_(sf::RenderWindow& window) {
    window.draw(panels_.aura_overlay);
}

// ============================================================================
// 【RenderPanels_】渲染面板
// ============================================================================
void UISystem::RenderPanels_(sf::RenderWindow& window) {
    window.draw(panels_.main_panel);
    window.draw(panels_.inventory_panel);
    window.draw(panels_.dialogue_panel);
    window.draw(panels_.hint_panel);
    window.draw(panels_.stamina_bar_bg);
    window.draw(panels_.stamina_bar_fill);
    window.draw(panels_.workshop_progress_bg);
    window.draw(panels_.workshop_progress_fill);
}

// ============================================================================
// 【RenderTexts_】渲染文本
// ============================================================================
void UISystem::RenderTexts_(sf::RenderWindow& window) {
    if (!font_loaded_) return;

    if (texts_.hud_text) window.draw(*texts_.hud_text);
    if (texts_.inventory_text) window.draw(*texts_.inventory_text);
    if (texts_.hint_text) window.draw(*texts_.hint_text);
    if (texts_.dialogue_text) window.draw(*texts_.dialogue_text);
    if (texts_.world_tip_text) window.draw(*texts_.world_tip_text);
    if (texts_.level_up_text) window.draw(*texts_.level_up_text);
    if (texts_.debug_text) window.draw(*texts_.debug_text);
}

// ============================================================================
// 【RenderProgressBars_】渲染进度条
// ============================================================================
void UISystem::RenderProgressBars_(sf::RenderWindow& window) {
    (void)window;
}

// ============================================================================
// 【MakeDefaultUiRenderOptions】默认 UI 渲染选项
// ============================================================================
UiRenderOptions MakeDefaultUiRenderOptions() {
    UiRenderOptions opts;
    opts.show_debug = true;
    opts.show_level_up = true;
    opts.show_festival = true;
    return opts;
}

}  // namespace CloudSeamanor::engine
