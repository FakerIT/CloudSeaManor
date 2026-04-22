#pragma once

// ============================================================================
// 【InputHandler】输入处理器
// ============================================================================
// Responsibilities:
// - Distribute input events to appropriate handlers
// - Handle action-to-key mapping
// - Support for gamepad (future)
// ============================================================================

#include "CloudSeamanor/InputManager.hpp"

#include <SFML/System/Vector2.hpp>
#include <functional>
#include <optional>
#include <string>

namespace CloudSeamanor::engine {

class GameWorldState;
class GameWorldSystems;

class InputHandler {
public:
    explicit InputHandler(InputManager& input_manager);

    void HandleEvent(const sf::Event& /*event*/) {}

    [[nodiscard]] bool IsActionJustPressed(Action action) const;
    [[nodiscard]] bool IsActionPressed(Action action) const;
    [[nodiscard]] sf::Vector2f GetMovementVector() const;

    // ========================================================================
    // 【面板回调】
    // ========================================================================
    struct PanelCallbacks {
        std::function<bool()> is_any_panel_open;
        std::function<bool()> is_dialogue_open;
        std::function<void()> close_all_panels;
        std::function<void()> toggle_inventory;
        std::function<void()> toggle_quest_menu;
        std::function<void()> toggle_map;
        std::function<void()> toggle_settings;
        std::function<bool()> is_settings_open;
        std::function<void(int)> settings_move_selection;
        std::function<void(int)> settings_adjust_value;
        std::function<int()> settings_selected_row;
        std::function<int()> settings_selected_slot;
        std::function<void()> confirm_dialogue;
        std::function<void(int)> dialogue_move_choice;
        std::function<void()> dialogue_confirm_choice;
        std::function<void(int)> select_hotbar_slot;
    };

    void SetPanelCallbacks(PanelCallbacks callbacks);

    // ========================================================================
    // 【游戏回调】
    // ========================================================================
    struct GameCallbacks {
        std::function<void(const std::string&, float)> push_hint;
        std::function<void()> refresh_title;
        std::function<void()> save_game;
        std::function<void()> load_game;
        std::function<void(int)> save_game_to_slot;
        std::function<void(int)> load_game_from_slot;
        std::function<bool()> can_sleep;
        std::function<void()> sleep;
        std::function<void()> gift_npc;
        std::function<void()> interact;
        std::function<void(const CloudSeamanor::domain::CloudState&)> update_aura;
    };

    void SetGameCallbacks(GameCallbacks callbacks);

    // ========================================================================
    // 【动作处理】
    // ========================================================================
    void HandlePanelAction(const GameWorldState& ws) const;
    void HandleGameAction(GameWorldState& ws, GameWorldSystems& systems) const;

private:
    void HandleHotbarSelection_() const;

    InputManager& input_manager_;
    std::optional<PanelCallbacks> panel_callbacks_;
    std::optional<GameCallbacks> game_callbacks_;
};

}  // namespace CloudSeamanor::engine
