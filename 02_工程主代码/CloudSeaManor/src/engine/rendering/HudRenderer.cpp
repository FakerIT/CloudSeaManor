#include "CloudSeamanor/engine/rendering/HudRenderer.hpp"

#include "CloudSeamanor/engine/GameWorldState.hpp"

namespace CloudSeamanor::engine {

HudRenderer::HudRenderer(UISystem& ui_system)
    : ui_system_(ui_system)
{
}

void HudRenderer::Render(
    sf::RenderWindow& window,
    const GameWorldState& world_state
) {
    RenderBackgroundLayer_(window, world_state);
    RenderPanels_(window, world_state);
    RenderTexts_(window, world_state);
}

void HudRenderer::RenderBackgroundLayer_(
    sf::RenderWindow& window,
    const GameWorldState& /*ws*/
) {
    window.draw(ui_system_.GetPanels().aura_overlay);
}

void HudRenderer::RenderPanels_(
    sf::RenderWindow& window,
    const GameWorldState& ws
) {
    if (!ws.GetFestivalNoticeText().empty()) {
        window.draw(ui_system_.GetPanels().festival_notice_panel);
    }
    window.draw(ui_system_.GetPanels().stamina_bar_bg);
    window.draw(ui_system_.GetPanels().stamina_bar_fill);
    window.draw(ui_system_.GetPanels().workshop_progress_bg);
    window.draw(ui_system_.GetPanels().workshop_progress_fill);
    window.draw(ui_system_.GetPanels().main_panel);
    window.draw(ui_system_.GetPanels().inventory_panel);
    window.draw(ui_system_.GetPanels().dialogue_panel);
    window.draw(ui_system_.GetPanels().hint_panel);
}

void HudRenderer::RenderTexts_(
    sf::RenderWindow& window,
    const GameWorldState& ws
) {
    const auto& t = ui_system_.GetTexts();
    if (t.festival_notice_text) window.draw(*t.festival_notice_text);
    if (t.hud_text) window.draw(*t.hud_text);
    if (t.inventory_text) window.draw(*t.inventory_text);
    if (t.hint_text) window.draw(*t.hint_text);
    if (t.dialogue_text) window.draw(*t.dialogue_text);
    if (t.world_tip_text) window.draw(*t.world_tip_text);
    if (t.level_up_text) window.draw(*t.level_up_text);
    if (ws.GetTutorial().show_debug_overlay && t.debug_text) window.draw(*t.debug_text);
}

}  // namespace CloudSeamanor::engine
