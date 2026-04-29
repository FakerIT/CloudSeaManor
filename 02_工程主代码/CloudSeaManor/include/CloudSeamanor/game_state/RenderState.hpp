#pragma once

#include "CloudSeamanor/engine/GameWorldState.hpp"

#include <SFML/Graphics.hpp>
#include <memory>

namespace CloudSeamanor::game_state {

struct UiDrawingState {
    sf::RectangleShape ui_panel;
    sf::RectangleShape inventory_panel;
    sf::RectangleShape bottom_hint_panel;
    sf::RectangleShape dialogue_panel;
    sf::RectangleShape stamina_bar_bg;
    sf::RectangleShape stamina_bar_fill;
    sf::RectangleShape roaster_progress_bg;
    sf::RectangleShape roaster_progress_fill;
    sf::RectangleShape aura_overlay;
    sf::RectangleShape festival_notice_panel;

    std::unique_ptr<sf::Font> font;
    std::unique_ptr<sf::Text> hud_text;
    std::unique_ptr<sf::Text> inventory_text;
    std::unique_ptr<sf::Text> hint_text;
    std::unique_ptr<sf::Text> dialogue_overlay_text;
    std::unique_ptr<sf::Text> debug_text;
    std::unique_ptr<sf::Text> world_tip_text;
    std::unique_ptr<sf::Text> festival_notice_text_obj;
    std::unique_ptr<sf::Text> level_up_text;

    bool font_loaded = false;
    sf::FloatRect world_bounds{{40.0f, 40.0f}, {1200.0f, 640.0f}};
};

using InteractionStateEx = CloudSeamanor::engine::InteractionState;
using TutorialStateEx = CloudSeamanor::engine::TutorialState;

struct RenderState {
    UiDrawingState ui;
    InteractionStateEx interaction;
    TutorialStateEx tutorial;
};

}  // namespace CloudSeamanor::game_state
