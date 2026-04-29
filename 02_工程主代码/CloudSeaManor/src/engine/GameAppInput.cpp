#include "CloudSeamanor/app/GameApp.hpp"
#include "CloudSeamanor/domain/BuffSystem.hpp"
#include "CloudSeamanor/engine/DialogueEngine.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"
#include "CloudSeamanor/domain/HungerTable.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace CloudSeamanor::engine {

// RE-206: UI state packed into ui_state_ (compat aliases for this TU).
#define pending_save_overwrite_slot_ ui_state_.pending_save_overwrite_slot
#define pending_save_overwrite_seconds_ ui_state_.pending_save_overwrite_seconds

namespace {

using FoodBuffTable = std::unordered_map<std::string, CloudSeamanor::domain::RuntimeBuff>;

FoodBuffTable LoadFoodBuffTable_() {
    FoodBuffTable table;
    std::ifstream in("assets/data/food/food_buff_profiles.csv");
    if (!in.is_open()) {
        return table;
    }
    std::string line;
    if (!std::getline(in, line)) {
        return table;
    }
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::stringstream ss(line);
        std::string item_id;
        std::string remain_text;
        std::string recover_text;
        std::string cost_text;
        if (!std::getline(ss, item_id, ',')) continue;
        if (!std::getline(ss, remain_text, ',')) continue;
        if (!std::getline(ss, recover_text, ',')) continue;
        if (!std::getline(ss, cost_text, ',')) continue;
        CloudSeamanor::domain::RuntimeBuff buff;
        buff.id = "food_buff_" + item_id;
        buff.remaining_seconds = static_cast<float>(std::atof(remain_text.c_str()));
        buff.stamina_recovery_multiplier = static_cast<float>(std::atof(recover_text.c_str()));
        buff.stamina_cost_multiplier = static_cast<float>(std::atof(cost_text.c_str()));
        if (!item_id.empty() && buff.remaining_seconds > 0.0f) {
            table[item_id] = buff;
        }
    }
    return table;
}

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

CloudSeamanor::domain::RuntimeBuff BuildFoodBuff_(const std::string& item_id, const int hunger_restore) {
    static const FoodBuffTable food_buffs = LoadFoodBuffTable_();
    if (const auto it = food_buffs.find(item_id); it != food_buffs.end()) {
        return it->second;
    }
    CloudSeamanor::domain::RuntimeBuff buff;
    buff.id = "food_buff_" + item_id;
    buff.remaining_seconds = 120.0f;
    buff.stamina_recovery_multiplier = 1.1f;
    buff.stamina_cost_multiplier = 0.95f;
    if (item_id.find("tea") != std::string::npos || item_id.find("Tea") != std::string::npos) {
        buff.remaining_seconds = 180.0f;
        buff.stamina_recovery_multiplier = 1.15f;
        buff.stamina_cost_multiplier = 0.9f;
    }
    if (hunger_restore >= 30) {
        buff.remaining_seconds = std::max(buff.remaining_seconds, 240.0f);
        buff.stamina_recovery_multiplier = std::max(buff.stamina_recovery_multiplier, 1.2f);
        buff.stamina_cost_multiplier = std::min(buff.stamina_cost_multiplier, 0.85f);
    }
    return buff;
}

}  // namespace

void GameApp::SetupInputCallbacks_() {
    input_handler_.SetPanelCallbacks(BuildPanelCallbacks_());
    input_handler_.SetGameCallbacks(BuildGameCallbacks_());
    if (pixel_hud_) {
        // BE-130: 选项 ID 失配时回退到首个选项，并记录警告日志。
        pixel_hud_->SetDialogueBoxCallbacks(
            []() {},
            [this](const std::string& choice_id) {
                auto& engine = runtime_.WorldState().MutableInteraction().dialogue_engine;
                if (!engine.IsActive()) {
                    runtime_.Callbacks().push_hint("当前没有可选择的对话。", 2.0f);
                    return;
                }
                const auto& choices = engine.CurrentChoices();
                if (choices.empty()) {
                    runtime_.Callbacks().push_hint("当前节点没有可用选项。", 2.0f);
                    return;
                }

                std::size_t idx = choices.size();
                for (std::size_t i = 0; i < choices.size(); ++i) {
                    if (choices[i].id == choice_id) {
                        idx = i;
                        break;
                    }
                }
                if (idx >= choices.size()) {
                    idx = 0;
                    CloudSeamanor::infrastructure::Logger::Warning(
                        "Dialogue choice id not found: '" + choice_id + "', fallback to first choice.");
                    runtime_.Callbacks().push_hint("选项已过期，已自动回退到第一个选项。", 2.4f);
                }
                if (!engine.SelectChoice(idx)) {
                    runtime_.Callbacks().push_hint("对话推进失败，请重试。", 2.4f);
                }
            });

        PixelGameHud::InventoryActionCallbacks action_cbs;
        action_cbs.use_item = [this](const std::string& item_id, const std::string& item_name, int) {
            auto& inv = runtime_.WorldState().MutableInventory();
            // 通用“食用”入口：如果物品在 HungerTable 中，则按饱食数据恢复。
            const auto* hunger_def = CloudSeamanor::domain::GetGlobalHungerTable().Get(item_id);
            if (hunger_def != nullptr) {
                if (!inv.RemoveItem(item_id, 1)) return false;
                runtime_.WorldState().MutableHunger().RestoreFromFood(
                    hunger_def->hunger_restore,
                    hunger_def->quality_multiplier);
                runtime_.WorldState().MutableBuffs().ApplyBuff(
                    BuildFoodBuff_(item_id, hunger_def->hunger_restore));
                runtime_.Callbacks().push_hint(
                    "已食用：" + item_name + "（饱食 +" + std::to_string(hunger_def->hunger_restore) + "）",
                    1.8f);
                return true;
            }

            if (!inv.RemoveItem(item_id, 1)) return false;
            runtime_.Callbacks().push_hint("已使用：" + item_name, 1.6f);
            return true;
        };
        action_cbs.drop_item = [this](const std::string& item_id, const std::string& item_name, int) {
            auto& inv = runtime_.WorldState().MutableInventory();
            if (!inv.RemoveItem(item_id, 1)) return false;
            runtime_.Callbacks().push_hint("已丢弃：" + item_name, 1.6f);
            return true;
        };
        action_cbs.sell_item = [this](const std::string& item_id, const std::string& item_name, int unit_price) {
            auto& ws = runtime_.WorldState();
            auto& inv = ws.MutableInventory();
            if (!inv.RemoveItem(item_id, 1)) return false;
            ws.SetGold(ws.GetGold() + std::max(1, unit_price));
            runtime_.Callbacks().push_hint("已出售：" + item_name, 1.6f);
            return true;
        };
        action_cbs.gift_item = [this](const std::string&, const std::string&, int) {
            runtime_.HandleGiftInteraction();
            return true;
        };
        pixel_hud_->SetInventoryActionCallbacks(std::move(action_cbs));
    }
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
    panel_cbs.settings_selected_row = [this]() { return static_cast<int>(pixel_hud_->SettingsSelectedRow()); };
    panel_cbs.settings_selected_slot = [this]() { return static_cast<int>(pixel_hud_->SettingsSelectedSlot()); };
    panel_cbs.settings_apply = [this]() {
        if (!pixel_hud_ || !audio_) return;
        auto& settings = pixel_hud_->MutableSettingsPanel();
        audio_->SetBGMVolume(settings.BgmVolume());
        audio_->SetSFXVolume(settings.SfxVolume());
        std::ofstream out("configs/audio.json", std::ios::trunc);
        if (out.is_open()) {
            out << "{\n"
                << "  \"music_volume\": " << audio_->MusicVolume() << ",\n"
                << "  \"bgm_volume\": " << settings.BgmVolume() << ",\n"
                << "  \"sfx_volume\": " << settings.SfxVolume() << ",\n"
                << "  \"fullscreen\": " << (settings.Fullscreen() ? "true" : "false") << "\n"
                << "}\n";
            runtime_.Callbacks().push_hint("设置已应用并保存。", 2.0f);
        } else {
            runtime_.Callbacks().push_hint("设置应用成功，但保存失败。", 2.0f);
        }
    };

    // 对话框确认键：优先通过 DialogueEngine 处理分支对话
    panel_cbs.confirm_dialogue = [this]() {
        auto& engine = runtime_.WorldState().MutableInteraction().dialogue_engine;
        if (engine.IsActive()) {
            return engine.OnConfirm();
        }
        return pixel_hud_->MutableDialogueBox().OnConfirm();
    };
    panel_cbs.dialogue_move_choice = [this](int delta) { pixel_hud_->DialogueMoveChoice(delta); };
    panel_cbs.dialogue_confirm_choice = [this]() { pixel_hud_->DialogueConfirmChoice(); };

    panel_cbs.select_hotbar_slot = [this](int i) { pixel_hud_->MutableToolbar().SetSelectedSlot(i); };
    return panel_cbs;
}

InputHandler::GameCallbacks GameApp::BuildGameCallbacks_() {
    InputHandler::GameCallbacks game_cbs;
    game_cbs.push_hint     = [this](const std::string& msg, float dur) { runtime_.Callbacks().push_hint(msg, dur); };
    game_cbs.refresh_title = [this]() { runtime_.RefreshWindowTitle(); };
    game_cbs.save_game     = [this]() {
        runtime_.SaveGame();
    };
    game_cbs.load_game     = [this]() {
        runtime_.LoadGame();
    };
    game_cbs.save_game_to_slot = [this](int slot) {
        const auto slots = runtime_.ReadSaveSlots();
        const bool exists = (slot >= 1 && slot <= static_cast<int>(slots.size()))
            ? slots[static_cast<std::size_t>(slot - 1)].exists
            : false;
        if (exists) {
            if (!ui_state_.pending_save_overwrite_slot.has_value()
                || ui_state_.pending_save_overwrite_slot.value() != slot
                || ui_state_.pending_save_overwrite_seconds > 3.0f) {
                ui_state_.pending_save_overwrite_slot = slot;
                ui_state_.pending_save_overwrite_seconds = 0.0f;
                runtime_.Callbacks().push_hint(
                    "存档槽位 " + std::to_string(slot) + " 已存在：3 秒内再次确认将覆盖。",
                    2.8f);
                return;
            }
        }
        ui_state_.pending_save_overwrite_slot.reset();
        ui_state_.pending_save_overwrite_seconds = 0.0f;
        runtime_.SaveGameToSlot(slot);
        if (pixel_hud_ && pixel_hud_->IsSettingsOpen()) {
            pixel_hud_->SetSettingsSlots(runtime_.ReadSaveSlots());
        }
    };
    game_cbs.load_game_from_slot = [this](int slot) {
        ui_state_.pending_save_overwrite_slot.reset();
        ui_state_.pending_save_overwrite_seconds = 0.0f;
        runtime_.LoadGameFromSlot(slot);
        if (pixel_hud_ && pixel_hud_->IsSettingsOpen()) {
            pixel_hud_->SetSettingsSlots(runtime_.ReadSaveSlots());
        }
    };
    game_cbs.can_sleep = [this]() { return runtime_.CanSleep(); };
    game_cbs.sleep = [this]() { runtime_.SleepToNextMorning(); };
    game_cbs.gift_npc = [this]() { runtime_.HandleGiftInteraction(); };
    game_cbs.interact = [this]() { runtime_.HandlePrimaryInteraction(); };
    game_cbs.eat_food = [this]() {
        auto& ws = runtime_.WorldState();
        auto& inv = ws.MutableInventory();
        const auto& slots = inv.Slots();
        for (const auto& s : slots) {
            if (s.count <= 0) continue;
            const auto* hunger_def = CloudSeamanor::domain::GetGlobalHungerTable().Get(s.item_id);
            if (hunger_def == nullptr) continue;
            if (!inv.RemoveItem(s.item_id, 1)) continue;
            ws.MutableHunger().RestoreFromFood(hunger_def->hunger_restore, hunger_def->quality_multiplier);
            ws.MutableBuffs().ApplyBuff(BuildFoodBuff_(s.item_id, hunger_def->hunger_restore));
            const std::string display_name =
                !hunger_def->item_name.empty() ? hunger_def->item_name : s.item_id;
            runtime_.Callbacks().push_hint(
                "快捷食用：" + display_name + "（饱食 +" + std::to_string(hunger_def->hunger_restore) + "）",
                1.8f);
            runtime_.RefreshWindowTitle();
            return;
        }
        runtime_.Callbacks().push_hint("背包里没有可食用的食物。", 2.0f);
    };
    game_cbs.update_aura = [this](const CloudSeamanor::domain::CloudState& cs) {
        ui_system_->UpdateAuraOverlay(AuraColorFromLayout(ui_layout_config_, cs));
    };
    return game_cbs;
}

}  // namespace CloudSeamanor::engine
