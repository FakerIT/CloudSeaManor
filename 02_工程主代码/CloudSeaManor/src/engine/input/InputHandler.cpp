#include "CloudSeamanor/engine/input/InputHandler.hpp"

#include "CloudSeamanor/engine/GameWorldState.hpp"
#include "CloudSeamanor/engine/GameWorldSystems.hpp"

#include <functional>
#include <string>

namespace CloudSeamanor::engine {

// ============================================================================
// 【InputHandler::InputHandler】构造函数
// ============================================================================
InputHandler::InputHandler(InputManager& input_manager)
    : input_manager_(input_manager)
{
}

// ============================================================================
// 【InputHandler::SetPanelCallbacks】设置面板操作回调
// ============================================================================
void InputHandler::SetPanelCallbacks(PanelCallbacks callbacks) {
    panel_callbacks_ = std::move(callbacks);
}

// ============================================================================
// 【InputHandler::SetGameCallbacks】设置游戏操作回调
// ============================================================================
void InputHandler::SetGameCallbacks(GameCallbacks callbacks) {
    game_callbacks_ = std::move(callbacks);
}

// ============================================================================
// 【InputHandler::HandlePanelAction】处理面板相关按键
// ============================================================================
void InputHandler::HandlePanelAction(const GameWorldState& ws) const {
    (void)ws;
    if (!panel_callbacks_) return;

    if (panel_callbacks_->is_settings_open && panel_callbacks_->is_settings_open()) {
        if (IsActionJustPressed(Action::Cancel)) {
            panel_callbacks_->toggle_settings();
            return;
        }
        if (IsActionJustPressed(Action::MoveUp)) {
            panel_callbacks_->settings_move_selection(-1);
            return;
        }
        if (IsActionJustPressed(Action::MoveDown)) {
            panel_callbacks_->settings_move_selection(1);
            return;
        }
        if (IsActionJustPressed(Action::MoveLeft)) {
            panel_callbacks_->settings_adjust_value(-1);
            return;
        }
        if (IsActionJustPressed(Action::MoveRight)) {
            panel_callbacks_->settings_adjust_value(1);
            return;
        }
        if (IsActionJustPressed(Action::Confirm) || IsActionJustPressed(Action::Interact)) {
            const int row = panel_callbacks_->settings_selected_row ? panel_callbacks_->settings_selected_row() : -1;
            const int slot = panel_callbacks_->settings_selected_slot ? panel_callbacks_->settings_selected_slot() : 1;
            if (row == 0 && game_callbacks_ && game_callbacks_->save_game_to_slot) {
                game_callbacks_->save_game_to_slot(slot);
            } else if (row == 1 && game_callbacks_ && game_callbacks_->load_game_from_slot) {
                game_callbacks_->load_game_from_slot(slot);
            } else if (panel_callbacks_->settings_apply) {
                panel_callbacks_->settings_apply();
            }
            return;
        }
        return;
    }

    // Esc: 关闭面板
    if (IsActionJustPressed(Action::Cancel)) {
        if (panel_callbacks_->is_any_panel_open()) {
            panel_callbacks_->close_all_panels();
        } else if (panel_callbacks_->toggle_settings) {
            panel_callbacks_->toggle_settings();
        }
        return;
    }

    // I: 背包
    if (IsActionJustPressed(Action::OpenInventory)) {
        panel_callbacks_->toggle_inventory();
        return;
    }

    // F: 任务面板
    if (IsActionJustPressed(Action::OpenQuestMenu)) {
        panel_callbacks_->toggle_quest_menu();
        return;
    }

    // M: 地图
    if (IsActionJustPressed(Action::OpenMap)) {
        panel_callbacks_->toggle_map();
        return;
    }

    // 对话框确认
    if (panel_callbacks_->is_dialogue_open()) {
        if (panel_callbacks_->dialogue_move_choice) {
            if (IsActionJustPressed(Action::MoveUp)) {
                panel_callbacks_->dialogue_move_choice(-1);
                return;
            }
            if (IsActionJustPressed(Action::MoveDown)) {
                panel_callbacks_->dialogue_move_choice(1);
                return;
            }
        }
        if (panel_callbacks_->dialogue_confirm_choice && IsActionJustPressed(Action::Interact)) {
            panel_callbacks_->dialogue_confirm_choice();
            return;
        }
        if (IsActionJustPressed(Action::Confirm) ||
            IsActionJustPressed(Action::AdvanceDialog)) {
            panel_callbacks_->confirm_dialogue();
            return;
        }
    }
}

// ============================================================================
// 【InputHandler::HandleGameAction】处理游戏内按键
// ============================================================================
void InputHandler::HandleGameAction(
    GameWorldState& ws,
    GameWorldSystems& systems
) const {
    if (!game_callbacks_) return;

    // 调试开关
    if (IsActionJustPressed(Action::DebugToggle)) {
        ws.MutableTutorial().show_debug_overlay = !ws.MutableTutorial().show_debug_overlay;
        game_callbacks_->push_hint(
            ws.MutableTutorial().show_debug_overlay ? "调试面板已开启。" : "调试面板已隐藏。",
            2.0f);
        // 同时切换循环调试面板
        if (game_callbacks_->toggle_loop_debug_panel) {
            game_callbacks_->toggle_loop_debug_panel();
        }
        return;
    }

    // 云海状态切换
    if (IsActionJustPressed(Action::CloudToggle)) {
        systems.GetCloud().CycleDebugState();
        systems.GetCloud().UpdateForecastVisibility(ws.GetClock().Day(), ws.GetClock().Hour());
        if (game_callbacks_->update_aura) {
            game_callbacks_->update_aura(systems.GetCloud().CurrentState());
        }
        game_callbacks_->push_hint(
            "云海状态切换为 " + systems.GetCloud().CurrentStateText() + "。" +
            systems.GetCloud().CurrentStateHint(),
            3.8f);
        game_callbacks_->refresh_title();
        return;
    }

    // 快速保存
    if (IsActionJustPressed(Action::QuickSave)) {
        game_callbacks_->save_game();
        game_callbacks_->push_hint("手动保存完成。", 2.8f);
        game_callbacks_->refresh_title();
        return;
    }

    // 快速读取
    if (IsActionJustPressed(Action::QuickLoad)) {
        game_callbacks_->load_game();
        game_callbacks_->push_hint("已读取存档。", 2.8f);
        game_callbacks_->refresh_title();
        return;
    }

    // 睡眠
    if (IsActionJustPressed(Action::Sleep)) {
        if (game_callbacks_->can_sleep()) {
            game_callbacks_->sleep();
            game_callbacks_->refresh_title();
        } else {
            game_callbacks_->push_hint(
                "现在还太早，22:00 之后才能休息。"
                "今天没做完也不用着急，明天或来年都还能继续安排。",
                3.4f);
        }
        return;
    }

    // NPC 送礼
    if (IsActionJustPressed(Action::GiftNpc)) {
        game_callbacks_->gift_npc();
        return;
    }

    // 快捷食用
    if (IsActionJustPressed(Action::EatFood)) {
        if (game_callbacks_->eat_food) {
            game_callbacks_->eat_food();
        }
        return;
    }

    // 主交互
    if (IsActionJustPressed(Action::Interact)) {
        game_callbacks_->interact();
        return;
    }

    // 热键栏选择（数字键 1-0, -, =）
    HandleHotbarSelection_();
}

// ============================================================================
// 【InputHandler::HandleHotbarSelection_】处理热键栏选择
// ============================================================================
void InputHandler::HandleHotbarSelection_() const {
    if (!panel_callbacks_) return;
    if (IsActionJustPressed(Action::HotbarSlot1))  panel_callbacks_->select_hotbar_slot(0);
    if (IsActionJustPressed(Action::HotbarSlot2))  panel_callbacks_->select_hotbar_slot(1);
    if (IsActionJustPressed(Action::HotbarSlot3))  panel_callbacks_->select_hotbar_slot(2);
    if (IsActionJustPressed(Action::HotbarSlot4))  panel_callbacks_->select_hotbar_slot(3);
    if (IsActionJustPressed(Action::HotbarSlot5))  panel_callbacks_->select_hotbar_slot(4);
    if (IsActionJustPressed(Action::HotbarSlot6))  panel_callbacks_->select_hotbar_slot(5);
    if (IsActionJustPressed(Action::HotbarSlot7))  panel_callbacks_->select_hotbar_slot(6);
    if (IsActionJustPressed(Action::HotbarSlot8))  panel_callbacks_->select_hotbar_slot(7);
    if (IsActionJustPressed(Action::HotbarSlot9))  panel_callbacks_->select_hotbar_slot(8);
    if (IsActionJustPressed(Action::HotbarSlot10)) panel_callbacks_->select_hotbar_slot(9);
    if (IsActionJustPressed(Action::HotbarSlot11)) panel_callbacks_->select_hotbar_slot(10);
    if (IsActionJustPressed(Action::HotbarSlot12)) panel_callbacks_->select_hotbar_slot(11);
}

// ============================================================================
// 【InputHandler::IsActionJustPressed】动作是否刚按下
// ============================================================================
bool InputHandler::IsActionJustPressed(Action action) const {
    return input_manager_.IsActionJustPressed(action);
}

// ============================================================================
// 【InputHandler::IsActionPressed】动作是否按住
// ============================================================================
bool InputHandler::IsActionPressed(Action action) const {
    return input_manager_.IsActionPressed(action);
}

// ============================================================================
// 【InputHandler::GetMovementVector】获取移动向量
// ============================================================================
sf::Vector2f InputHandler::GetMovementVector() const {
    return input_manager_.GetMovementVector();
}

}  // namespace CloudSeamanor::engine
