#include "CloudSeamanor/PixelPlayerStatusPanel.hpp"

#include "CloudSeamanor/PixelFontRenderer.hpp"

namespace CloudSeamanor::engine {

PixelPlayerStatusPanel::PixelPlayerStatusPanel()
    : PixelUiPanel({{400.0f, 100.0f}, {480.0f, 520.0f}}, "玩家状态", true) {
    m_stamina.SetStaminaStyle();
    m_spirit.SetContractStyle();
    m_fatigue.SetCropStyle();
}

void PixelPlayerStatusPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;

    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    const float line_h = 24.0f;

    m_font_renderer->DrawText(window,
                              m_data.player_name + " Lv." + std::to_string(m_data.player_level) + "  山庄阶段 " + std::to_string(m_data.manor_stage),
                              {x, y},
                              TextStyle::PanelTitle());
    m_font_renderer->DrawText(window, "总资产: " + std::to_string(m_data.total_gold), {x, y + line_h}, TextStyle::CoinText());

    m_stamina.SetPosition({x, y + line_h * 2.2f});
    m_stamina.SetProgress(m_data.stamina_ratio);
    m_stamina.Render(window);
    m_font_renderer->DrawText(window, "体力", {x + 170.0f, y + line_h * 2.0f}, TextStyle::HotkeyHint());

    m_spirit.SetPosition({x, y + line_h * 3.3f});
    m_spirit.SetProgress(m_data.spirit_ratio);
    m_spirit.Render(window);
    m_font_renderer->DrawText(window, "灵气", {x + 170.0f, y + line_h * 3.1f}, TextStyle::HotkeyHint());

    m_fatigue.SetPosition({x, y + line_h * 4.4f});
    m_fatigue.SetProgress(1.0f - m_data.fatigue_ratio);
    m_fatigue.Render(window);
    m_font_renderer->DrawText(window, "疲劳", {x + 170.0f, y + line_h * 4.2f}, TextStyle::HotkeyHint());

    m_font_renderer->DrawText(window, "契约进度: " + std::to_string(m_data.contract_progress) + "/6", {x, y + line_h * 5.6f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
