#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/GameApp.hpp"
#include "CloudSeamanor/DialogueEngine.hpp"

namespace CloudSeamanor::engine {

namespace {

sf::Color AuraColorFromLayout(const infrastructure::UiLayoutConfig& layout, domain::CloudState s) {
    std::string key;
    using CS = domain::CloudState;
    switch (s) {
    case CS::Clear:      key = "cloud_clear"; break;
    case CS::Mist:       key = "cloud_mist"; break;
    case CS::DenseCloud: key = "cloud_dense"; break;
    case CS::Tide:       key = "cloud_tide"; break;
    }
    const auto cs = layout.GetCloudColor(key);
    const auto r = static_cast<std::uint8_t>((cs.aura >> 24) & 0xFF);
    const auto g = static_cast<std::uint8_t>((cs.aura >> 16) & 0xFF);
    const auto b = static_cast<std::uint8_t>((cs.aura >> 8) & 0xFF);
    const auto a = static_cast<std::uint8_t>(cs.aura & 0xFF);
    return sf::Color(r, g, b, a);
}

}  // namespace

void GameApp::SetupInputCallbacks_() {
    input_handler_.SetPanelCallbacks(BuildPanelCallbacks_());
    input_handler_.SetGameCallbacks(BuildGameCallbacks_());
}

InputHandler::PanelCallbacks GameApp::BuildPanelCallbacks_() {
    InputHandler::PanelCallbacks panel_cbs;
    panel_cbs.is_any_panel_open = [this]() { return pixel_hud_->IsAnyPanelOpen(); };
    panel_cbs.is_dialogue_open  = [this]() { return pixel_hud_->IsDialogueOpen(); };
    panel_cbs.close_all_panels  = [this]() { pixel_hud_->CloseAllPanels(); };
    panel_cbs.toggle_inventory  = [this]() { pixel_hud_->ToggleInventory(); };
    panel_cbs.toggle_quest_menu = [this]() { pixel_hud_->ToggleQuestMenu(); };
    panel_cbs.toggle_map        = [this]() { pixel_hud_->ToggleMap(); };
    panel_cbs.toggle_settings   = [this]() {
        if (!pixel_hud_->IsSettingsOpen()) {
            pixel_hud_->SetSettingsSlots(runtime_.ReadSaveSlots());
        }
        pixel_hud_->ToggleSettings();
    };
    panel_cbs.is_settings_open = [this]() { return pixel_hud_->IsSettingsOpen(); };
    panel_cbs.settings_move_selection = [this](int delta) { pixel_hud_->SettingsMoveSelection(delta); };
    panel_cbs.settings_adjust_value = [this](int delta) { pixel_hud_->SettingsAdjustValue(delta); };
    panel_cbs.settings_selected_row = [this]() { return pixel_hud_->SettingsSelectedRow(); };
    panel_cbs.settings_selected_slot = [this]() { return pixel_hud_->SettingsSelectedSlot(); };

    // 对话框确认键：优先通过 DialogueEngine 处理分支对话
    panel_cbs.confirm_dialogue = [this]() {
        auto& engine = runtime_.WorldState().GetInteraction().dialogue_engine;
        if (engine.IsActive()) {
            return engine.OnConfirm();
        }
        return pixel_hud_->GetDialogueBox().OnConfirm();
    };
    panel_cbs.dialogue_move_choice = [this](int delta) { pixel_hud_->DialogueMoveChoice(delta); };
    panel_cbs.dialogue_confirm_choice = [this]() { pixel_hud_->DialogueConfirmChoice(); };

    panel_cbs.select_hotbar_slot = [this](int i) { pixel_hud_->GetToolbar().SetSelectedSlot(i); };
    return panel_cbs;
}

InputHandler::GameCallbacks GameApp::BuildGameCallbacks_() {
    InputHandler::GameCallbacks game_cbs;
    game_cbs.push_hint     = [this](const std::string& msg, float dur) { runtime_.Callbacks().push_hint(msg, dur); };
    game_cbs.refresh_title = [this]() { runtime_.RefreshWindowTitle(); };
    game_cbs.save_game     = [this]() {
        RunWithLoading_("正在保存存档", [this]() { runtime_.SaveGame(); });
    };
    game_cbs.load_game     = [this]() {
        RunWithLoading_("正在读取存档", [this]() { runtime_.LoadGame(); });
    };
    game_cbs.save_game_to_slot = [this](int slot) {
        RunWithLoading_("正在保存到槽位", [this, slot]() { runtime_.SaveGameToSlot(slot); });
    };
    game_cbs.load_game_from_slot = [this](int slot) {
        RunWithLoading_("正在读取槽位", [this, slot]() { runtime_.LoadGameFromSlot(slot); });
    };
    game_cbs.can_sleep = [this]() { return runtime_.CanSleep(); };
    game_cbs.sleep = [this]() { runtime_.SleepToNextMorning(); };
    game_cbs.gift_npc = [this]() { runtime_.HandleGiftInteraction(); };
    game_cbs.interact = [this]() { runtime_.HandlePrimaryInteraction(); };
    game_cbs.update_aura = [this](const CloudSeamanor::domain::CloudState& cs) {
        ui_system_->UpdateAuraOverlay(AuraColorFromLayout(ui_layout_config_, cs));
    };
    return game_cbs;
}

}  // namespace CloudSeamanor::engine
