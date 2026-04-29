#include "CloudSeamanor/engine/InteractionSystem.hpp"

#include "CloudSeamanor/engine/FarmingSystem.hpp"
#include "CloudSeamanor/infrastructure/GameConstants.hpp"
#include "CloudSeamanor/domain/WorkshopSystem.hpp"
#include "CloudSeamanor/domain/DynamicLifeSystem.hpp"
#include "CloudSeamanor/app/GameAppText.hpp"
#include "CloudSeamanor/app/GameAppSpiritBeast.hpp"
#include "CloudSeamanor/app/GameAppNpc.hpp"
#include "CloudSeamanor/engine/NpcDialogueManager.hpp"
#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/domain/CropData.hpp"
#include "CloudSeamanor/domain/GameClock.hpp"

#include <cmath>
#include <vector>
#include <string>
#include <functional>

namespace CloudSeamanor {
namespace engine {

InteractionSystem::InteractionSystem() {
}

void InteractionSystem::Initialize(const InteractionCallbacks& callbacks) {
    callbacks_ = callbacks;
}

void InteractionSystem::SetFarmingSystem(FarmingSystem* farming) {
    farming_system_ = farming;
}

void InteractionSystem::SetWorkshopSystem(CloudSeamanor::domain::WorkshopSystem* workshop) {
    workshop_system_ = workshop;
}

void InteractionSystem::SetDynamicLifeSystem(CloudSeamanor::domain::DynamicLifeSystem* dynamic_life) {
    dynamic_life_system_ = dynamic_life;
}

void InteractionSystem::SetDialogueManager(CloudSeamanor::engine::NpcDialogueManager* manager) {
    dialogue_manager_ = manager;
}

InteractionResult InteractionSystem::TalkToNpc(
    int npc_index,
    std::vector<NpcActor>& npcs,
    const NpcTextMappings& text_mappings,
    const CloudSeamanor::domain::GameClock& clock,
    CloudSeamanor::domain::CloudState cloud_state,
    const std::string& player_name,
    const std::string& farm_name
) {
    InteractionResult result;

    if (npc_index < 0 || npc_index >= static_cast<int>(npcs.size())) {
        return result;
    }

    NpcActor& npc = npcs[static_cast<std::size_t>(npc_index)];
    if (npc.last_talk_day == clock.Day() && npc.daily_talked) {
        result.success = true;
        result.show_dialogue = false;
        result.dialogue_text = "今天已经聊过了，明天再来吧。";
        result.message = result.dialogue_text;
        if (callbacks_.push_hint) {
            callbacks_.push_hint(result.dialogue_text, 2.4f);
        }
        return result;
    }
    npc.state = NpcState::Talking;
    npc.daily_talked = true;
    npc.last_talk_day = clock.Day();

    result.success = true;
    result.show_dialogue = true;
    result.update_hud = true;

    if (dialogue_manager_ && dialogue_manager_->IsDailyDialogueLoaded(npc.id)) {
        NpcDialogueContext ctx = BuildNpcDialogueContext(
            npc, clock, cloud_state, player_name, farm_name);

        // 心事件优先级高于日常对话
        if (const HeartEventEntry* evt = dialogue_manager_->CheckHeartEventTrigger(npc.id, ctx)) {
            auto nodes = dialogue_manager_->LoadHeartEventDialogue(npc.id, evt->heart_threshold);
            if (!nodes.empty()) {
                result.dialogue_nodes = nodes;
                result.dialogue_start_id = nodes.front().id;
                result.dialogue_text = npc.display_name + " 的心事件触发了！";
                if (callbacks_.play_sfx) {
                    callbacks_.play_sfx("heart_event");
                }
            }
        } else {
            auto nodes = dialogue_manager_->SelectDailyDialogue(npc.id, ctx);
            if (!nodes.empty()) {
                result.dialogue_nodes = nodes;
                result.dialogue_start_id = nodes.front().id;
                result.dialogue_text = npc.display_name + ": " +
                    LocationDisplayName(text_mappings, npc.current_location);
            } else {
                result.dialogue_text = npc.display_name + ": " +
                    LocationDisplayName(text_mappings, npc.current_location) + ", " +
                    ActivityDisplayName(text_mappings, npc.current_activity) +
                    ". Favor: " + std::to_string(npc.favor);
            }
        }
    } else {
        result.dialogue_text = npc.display_name + ": " +
            LocationDisplayName(text_mappings, npc.current_location) + ", " +
            ActivityDisplayName(text_mappings, npc.current_activity) +
            ". Favor: " + std::to_string(npc.favor);
    }

    if (callbacks_.push_hint) {
        callbacks_.push_hint("Talking to " + npc.display_name + ".", 2.4f);
    }
    if (callbacks_.play_sfx) {
        callbacks_.play_sfx("dialogue_continue");
    }
    if (callbacks_.log_info) {
        callbacks_.log_info("Talked to " + npc.display_name + ".");
    }

    return result;
}

GiftResult InteractionSystem::GiveGiftToNpc(
    int npc_index,
    std::vector<NpcActor>& npcs,
    CloudSeamanor::domain::Inventory& inventory,
    CloudSeamanor::domain::DynamicLifeSystem* dynamic_life
) {
    GiftResult result;

    if (npc_index < 0 || npc_index >= static_cast<int>(npcs.size())) {
        return result;
    }

    NpcActor& npc = npcs[static_cast<std::size_t>(npc_index)];

    if (npc.daily_gifted) {
        result.message = npc.display_name + " already received a gift today.";
        return result;
    }

    if (!inventory.TryRemoveItem("TeaPack", 1)) {
        result.message = "You need TeaPack x1 to give a gift.";
        return result;
    }

    npc.daily_gifted = true;
    result.success = true;
    if (callbacks_.play_sfx) {
        callbacks_.play_sfx("gift");
    }

    if (ContainsItem(npc.prefs.loved, "TeaPack")) {
        result.favor_change = 30;
        result.message = npc.display_name + "眼睛亮了起来！ Favor +30";
    } else if (ContainsItem(npc.prefs.liked, "TeaPack")) {
        result.favor_change = 15;
        result.message = npc.display_name + "微微点头。 Favor +15";
    } else if (ContainsItem(npc.prefs.disliked, "TeaPack")) {
        result.favor_change = -5;
        result.message = npc.display_name + "不太高兴。 Favor -5";
    } else {
        result.favor_change = static_cast<int>(GameConstants::Npc::FavorNeutral);
        result.message = npc.display_name + " accepted the TeaPack. Favor +1";
    }

    npc.favor += result.favor_change;

    if (dynamic_life) {
        dynamic_life->AddPlayerPoints(npc.id, static_cast<float>(result.favor_change) * GameConstants::Npc::FavorToDynamicLifeMultiplier);
        if (auto* state = dynamic_life->GetNpcState(npc.id)) {
            if (state->stage_changed_today) {
                result.message += " " + npc.display_name + " entered a new life stage!";
                if (callbacks_.push_hint) {
                    callbacks_.push_hint(npc.display_name + " entered a new life stage!", 4.0f);
                }
            }
        }
    }

    if (callbacks_.push_hint) callbacks_.push_hint(result.message, 3.2f);
    if (callbacks_.log_info) callbacks_.log_info("Gift to " + npc.display_name + ", favor change: " + std::to_string(result.favor_change));
    if (callbacks_.update_hud) callbacks_.update_hud();
    if (callbacks_.refresh_window_title) callbacks_.refresh_window_title();

    return result;
}

InteractionResult InteractionSystem::InteractWithSpiritBeast(
    SpiritBeast& beast,
    int current_day,
    std::vector<HeartParticle>& heart_particles
) {
    InteractionResult result;

    if (beast.daily_interacted) {
        result.success = false;
        result.message = "Already interacted with Spirit Beast today.";
        if (callbacks_.push_hint) callbacks_.push_hint(result.message, 2.2f);
        if (callbacks_.log_info) callbacks_.log_info("Already interacted with Spirit Beast today.");
        return result;
    }

    beast.daily_interacted = true;
    beast.last_interaction_day = current_day;
    beast.state = SpiritBeastState::Interact;
    beast.interact_timer = 1.5f;

    SpawnHeartParticles(beast.position, heart_particles);

    result.success = true;
    result.message = "You bonded with the Spirit Beast. Its assistance is activated for today.";

    if (callbacks_.push_hint) callbacks_.push_hint(result.message, 2.8f);
    if (callbacks_.log_info) callbacks_.log_info("Pet the Spirit Beast.");

    return result;
}

InteractionResult InteractionSystem::InteractWithPlot(
    int plot_index,
    std::vector<TeaPlot>& plots,
    CloudSeamanor::domain::Inventory& inventory,
    CloudSeamanor::domain::SkillSystem& skills,
    float cloud_density,
    bool spirit_beast_interacted
) {
    InteractionResult result;

    if (plot_index < 0 || plot_index >= static_cast<int>(plots.size())) {
        result.message = "Invalid plot index.";
        return result;
    }

    TeaPlot& plot = plots[static_cast<std::size_t>(plot_index)];

    if (!plot.tilled) {
        plot.tilled = true;
        result.success = true;
        result.message = plot.crop_name + " plot tilled. Next: plant.";
        if (callbacks_.push_hint) callbacks_.push_hint(result.message, 2.4f);
        if (callbacks_.log_info) callbacks_.log_info(plot.crop_name + " plot tilled.");
        if (callbacks_.update_hud) callbacks_.update_hud();
        if (callbacks_.refresh_window_title) callbacks_.refresh_window_title();
        return result;
    }

    if (!plot.seeded) {
        if (inventory.TryRemoveItem(plot.seed_item_id, 1)) {
            plot.seeded = true;
            plot.growth = 0.0f;
            plot.stage = 1;
            result.success = true;
            result.message = plot.crop_name + " planted. Water it to start growing.";
            if (callbacks_.push_hint) callbacks_.push_hint(result.message, 2.6f);
            if (callbacks_.log_info) callbacks_.log_info(plot.crop_name + " planted.");
            if (callbacks_.play_sfx) callbacks_.play_sfx("plant");
        } else {
            result.message = "Missing seed: " + ItemDisplayName(plot.seed_item_id) + ".";
            if (callbacks_.push_hint) callbacks_.push_hint(result.message, 2.6f);
            if (callbacks_.log_info) callbacks_.log_info("Missing seed: " + plot.seed_item_id);
        }
        if (callbacks_.update_hud) callbacks_.update_hud();
        if (callbacks_.refresh_window_title) callbacks_.refresh_window_title();
        return result;
    }

    if (!plot.watered) {
        plot.watered = true;
        result.success = true;
        result.message = plot.crop_name + " watered.";
        if (callbacks_.push_hint) callbacks_.push_hint(result.message, 3.0f);
        if (callbacks_.log_info) callbacks_.log_info(plot.crop_name + " plot watered.");
        if (callbacks_.play_sfx) callbacks_.play_sfx("water");
        if (callbacks_.update_hud) callbacks_.update_hud();
        if (callbacks_.refresh_window_title) callbacks_.refresh_window_title();
        return result;
    }

    if (plot.ready) {
        const float tea_buff = 1.0f + skills.GetBonus(CloudSeamanor::domain::SkillType::SpiritFarm) * 0.1f;
        const float beast_share = spirit_beast_interacted ? 1.2f : 1.0f;

        if (skills.AddExp(CloudSeamanor::domain::SkillType::SpiritFarm, 20.0f, cloud_density, tea_buff, beast_share)) {
            const int new_level = skills.GetLevel(CloudSeamanor::domain::SkillType::SpiritFarm);
            result.message = "Spirit Farm skill Lv." + std::to_string(new_level) + "!";
            if (callbacks_.push_hint) callbacks_.push_hint(result.message, 3.2f);
            if (callbacks_.log_info) callbacks_.log_info("Spirit Farm skill Lv." + std::to_string(new_level) + "!");
            if (callbacks_.play_sfx) callbacks_.play_sfx("level_up");
        }

        std::vector<CloudSeamanor::domain::ItemCount> harvest_items{
            {plot.harvest_item_id, plot.harvest_amount}
        };
        const auto add_result = inventory.TryAddItems(harvest_items);
        if (!add_result) {
            result.success = false;
            result.message = "背包已满，无法收获更多作物";
            if (callbacks_.push_hint) callbacks_.push_hint(result.message, 2.8f);
            return result;
        }
        plot.seeded = false;
        plot.watered = false;
        plot.ready = false;
        plot.growth = 0.0f;
        plot.stage = 0;

        result.success = true;
        const std::string quality_prefix =
            CloudSeamanor::domain::CropTable::QualityToPrefixText(plot.quality);
        std::string harvest_msg =
            "收获了 " + quality_prefix + ItemDisplayName(plot.harvest_item_id)
            + " ×" + std::to_string(plot.harvest_amount);
        result.message = harvest_msg;
        if (callbacks_.push_hint) callbacks_.push_hint(harvest_msg, 2.8f);
        if (callbacks_.log_info) callbacks_.log_info(plot.crop_name + " harvested.");
        if (callbacks_.play_sfx) callbacks_.play_sfx("harvest");
        if (callbacks_.update_hud) callbacks_.update_hud();
        if (callbacks_.refresh_window_title) callbacks_.refresh_window_title();
        return result;
    }

    result.message = plot.crop_name + " growing. " + GetPlotActionText(plot) + ".";
    if (callbacks_.push_hint) callbacks_.push_hint(result.message, 2.2f);
    if (callbacks_.log_info) callbacks_.log_info(plot.crop_name + " still growing.");
    if (callbacks_.update_hud) callbacks_.update_hud();
    if (callbacks_.refresh_window_title) callbacks_.refresh_window_title();
    return result;
}

InteractionResult InteractionSystem::InteractWithObject(
    int object_index,
    const std::vector<CloudSeamanor::domain::Interactable>& objects,
    std::vector<CloudSeamanor::domain::PickupDrop>& pickups,
    CloudSeamanor::domain::Inventory& inventory,
    CloudSeamanor::domain::WorkshopSystem* workshop,
    RepairProject& repair,
    std::vector<sf::RectangleShape>& obstacle_shapes,
    CloudSeamanor::domain::SkillSystem& skills,
    float cloud_density,
    bool spirit_beast_interacted
) {
    InteractionResult result;

    if (object_index < 0 || object_index >= static_cast<int>(objects.size())) {
        result.message = "Invalid object index.";
        return result;
    }

    const CloudSeamanor::domain::Interactable& target = objects[static_cast<std::size_t>(object_index)];

    switch (target.Type()) {
    case CloudSeamanor::domain::InteractableType::GatheringNode: {
        pickups.emplace_back(target.Shape().getPosition() + sf::Vector2f(14.0f, -10.0f), target.RewardItem(), target.RewardAmount());
        RefreshPickupVisual(pickups.back());

        const float beast_share = spirit_beast_interacted ? 1.2f : 1.0f;
        if (skills.AddExp(CloudSeamanor::domain::SkillType::SpiritForage, 15.0f, cloud_density, 1.0f, beast_share)) {
            const int new_level = skills.GetLevel(CloudSeamanor::domain::SkillType::SpiritForage);
            result.message = "Spirit Forage skill Lv." + std::to_string(new_level) + "!";
            if (callbacks_.push_hint) callbacks_.push_hint(result.message, 3.2f);
        }

        result.success = true;
        result.message = "Gathered from " + target.Label() + ".";
        if (callbacks_.push_hint) callbacks_.push_hint(result.message, 2.6f);
        if (callbacks_.log_info) callbacks_.log_info("Gathered from " + target.Label() + ".");
        if (callbacks_.update_hud) callbacks_.update_hud();
        if (callbacks_.refresh_window_title) callbacks_.refresh_window_title();
        break;
    }

    case CloudSeamanor::domain::InteractableType::Workstation: {
        if (workshop) {
            const auto* machine = workshop->GetMachine("tea_machine");
            if (machine && machine->is_processing) {
                result.message = "Tea machine is running.";
                if (callbacks_.push_hint) callbacks_.push_hint(result.message, 2.2f);
                if (callbacks_.log_info) callbacks_.log_info("Tea machine running.");
            } else if (workshop->StartProcessing("tea_machine", "green_tea", inventory)) {
                result.success = true;
                result.message = "Tea machine started.";
                if (callbacks_.push_hint) callbacks_.push_hint(result.message, 2.8f);
                if (callbacks_.log_info) callbacks_.log_info("Tea machine started.");
            } else {
                result.message = "Need Leaves x2 to make TeaPack.";
                if (callbacks_.push_hint) callbacks_.push_hint(result.message, 2.4f);
                if (callbacks_.log_info) callbacks_.log_info("Not enough leaves.");
            }
        }
        if (callbacks_.update_hud) callbacks_.update_hud();
        if (callbacks_.refresh_window_title) callbacks_.refresh_window_title();
        break;
    }

    case CloudSeamanor::domain::InteractableType::Storage: {
        const std::vector<CloudSeamanor::domain::ItemCount> repair_costs{
            {"Wood", repair.wood_cost},
            {"Turnip", repair.turnip_cost}
        };
        if (!repair.completed && inventory.TryRemoveItems(repair_costs)) {
            repair.completed = true;
            if (!obstacle_shapes.empty()) {
                obstacle_shapes.front().setFillColor(sf::Color(148, 122, 92));
                obstacle_shapes.front().setOutlineColor(sf::Color(86, 62, 38));
            }
            result.success = true;
            result.message = "Main house repaired!";
            if (callbacks_.push_hint) callbacks_.push_hint(result.message, 3.0f);
            if (callbacks_.log_info) callbacks_.log_info("Main house repaired.");
        } else if (repair.completed) {
            result.message = "Main house already repaired.";
            if (callbacks_.push_hint) callbacks_.push_hint(result.message, 2.0f);
            if (callbacks_.log_info) callbacks_.log_info("Main house already repaired.");
        } else {
            result.message = "Need Wood x" + std::to_string(repair.wood_cost) +
                " and Turnip x" + std::to_string(repair.turnip_cost) + ".";
            if (callbacks_.push_hint) callbacks_.push_hint(result.message, 2.8f);
            if (callbacks_.log_info) callbacks_.log_info("Need materials to repair.");
        }
        if (callbacks_.update_hud) callbacks_.update_hud();
        if (callbacks_.refresh_window_title) callbacks_.refresh_window_title();
        break;
    }

    default:
        result.message = target.TypeText() + ": " + target.Label();
        break;
    }

    return result;
}

std::string InteractionSystem::GetPlotActionText(const TeaPlot& plot) const {
    if (!plot.tilled) return plot.crop_name + ": Till [E]";
    if (!plot.seeded) return plot.crop_name + ": Plant " + ItemDisplayName(plot.seed_item_id) + " [E]";
    if (!plot.watered) return plot.crop_name + ": Water [E]";
    if (plot.ready) return plot.crop_name + ": Harvest [E]";
    return plot.crop_name + ": Stage " + std::to_string(plot.stage) + "/4";
}

std::string InteractionSystem::GetNpcActionText(const NpcActor& npc) const {
    return npc.display_name + ": Talk [E] / Give TeaPack [G]";
}

std::string InteractionSystem::GetSpiritBeastActionText(const SpiritBeast& beast, bool watered_today) const {
    (void)beast;
    std::string text = "Spirit Beast: Bond [E] | Assist ";
    text += watered_today ? "Used" : "Available";
    return text;
}

std::string InteractionSystem::GetObjectActionText(const CloudSeamanor::domain::Interactable& object, const RepairProject& repair, const TeaMachine& machine) const {
    if (object.Type() == CloudSeamanor::domain::InteractableType::Storage && !repair.completed) {
        return "Main House: Wood x" + std::to_string(repair.wood_cost) +
            " + Turnip x" + std::to_string(repair.turnip_cost) + " [E]";
    }
    if (object.Type() == CloudSeamanor::domain::InteractableType::Workstation) {
        if (machine.running) return "Tea Machine: Processing";
        return "Tea Machine: Leaves x2 [E]";
    }
    return object.TypeText() + ": " + object.Label() + " [E]";
}

InteractionCallbacks CreateDefaultInteractionCallbacks(
    const std::function<void(const std::string&, float)>& push_hint,
    const std::function<void(const std::string&)>& log_info,
    const std::function<void()>& update_hud,
    const std::function<void()>& refresh_window_title,
    const std::function<void(const std::string&)>& play_sfx,
    const std::function<std::string()>& get_player_name,
    const std::function<std::string()>& get_farm_name
) {
    InteractionCallbacks callbacks;
    callbacks.push_hint = push_hint;
    callbacks.log_info = log_info;
    callbacks.update_hud = update_hud;
    callbacks.refresh_window_title = refresh_window_title;
    callbacks.play_sfx = play_sfx;
    callbacks.get_player_name = get_player_name;
    callbacks.get_farm_name = get_farm_name;
    return callbacks;
}

void RefreshPickupVisual(CloudSeamanor::domain::PickupDrop& pickup) {
    auto& shape = pickup.Shape();
    shape.setFillColor(PickupColorFor(pickup.ItemId()));
    shape.setOutlineThickness(2.0f);
    shape.setOutlineColor(sf::Color(72, 48, 24));
}

} // namespace engine
} // namespace CloudSeamanor
