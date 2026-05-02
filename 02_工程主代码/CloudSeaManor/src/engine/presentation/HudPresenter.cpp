#include "CloudSeamanor/engine/presentation/HudPresenter.hpp"

#include "CloudSeamanor/app/GameAppText.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace CloudSeamanor::engine {

namespace {

std::string AuraStageText_(int spirit_energy) {
    if (spirit_energy >= 180) return "丰盈";
    if (spirit_energy >= 120) return "活跃";
    if (spirit_energy >= 60) return "平稳";
    return "低潮";
}

std::string TideTypeText_(int tide_countdown_days) {
    if (tide_countdown_days <= 0) return "大潮日";
    if (tide_countdown_days <= 2) return "小潮将近";
    return "平潮期";
}

}  // namespace

void HudPresenter::UpdateCoreViews(
    PixelGameHud& hud,
    GameRuntime& runtime,
    const InputManager& input,
    float delta_seconds) {
    auto& world_state = runtime.WorldState();

    hud.Update(delta_seconds, &world_state.MutableInteraction().dialogue_engine);
    hud.UpdateTopRightInfo(
        "第" + std::to_string(world_state.GetClock().Day()) + "天 " + world_state.GetClock().TimeText(),
        world_state.GetClock().DateText(),
        runtime.Systems().GetCloud().CurrentStateText(),
        false);
    hud.SetBottomRightHotkeyHints(
        input.GetPrimaryKeyName(Action::Interact),
        input.GetPrimaryKeyName(Action::UseTool));
    hud.UpdateStaminaBar(
        world_state.GetStamina().Ratio(),
        world_state.GetStamina().Current(),
        world_state.GetStamina().Max());
    hud.UpdateHungerBar(
        world_state.GetHunger().Ratio(),
        static_cast<float>(world_state.GetHunger().Current()),
        static_cast<float>(world_state.GetHunger().Max()));
    hud.UpdateCoins(world_state.GetGold());

    std::vector<Quest> quests;
    quests.reserve(world_state.GetRuntimeQuests().size());
    for (const auto& runtime_quest : world_state.GetRuntimeQuests()) {
        Quest quest;
        quest.id = runtime_quest.id;
        quest.title = runtime_quest.title;
        quest.description = runtime_quest.description;
        quest.objective = runtime_quest.objective;
        quest.reward_text = runtime_quest.reward;
        if (runtime_quest.state == QuestState::NotTaken) {
            quest.status = QuestStatus::Available;
        } else if (runtime_quest.state == QuestState::InProgress) {
            quest.status = QuestStatus::Active;
        } else {
            quest.status = QuestStatus::Completed;
        }

        if (runtime_quest.id == "daily_commission_tea") {
            quest.progress_max = 3;
            quest.progress_current = std::min(3, world_state.GetInventory().CountOf("TeaLeaf"));
        } else if (runtime_quest.id == "tool_upgrade_intro") {
            quest.progress_max = 2;
            quest.progress_current = std::min(2, world_state.GetMainHouseRepair().level);
        }
        quests.push_back(std::move(quest));
    }
    hud.UpdateQuests(quests);

    hud.MutableMinimap().SetWorldBounds(world_state.GetConfig().world_bounds);
    hud.MutableMinimap().UpdatePlayerPosition(CloudSeamanor::adapter::ToSf(world_state.GetPlayer().GetPosition()));
    hud.MutableMinimap().SetLocationText(
        world_state.GetInSpiritRealm() ? "灵界 · 浅层入口" : "云海农场 · 主屋前");

    CloudForecastViewData forecast_view;
    const auto& cloud_system = runtime.Systems().GetCloud();
    forecast_view.today_prefix = runtime.Config().GetString("forecast_today_prefix", forecast_view.today_prefix);
    forecast_view.tomorrow_prefix = runtime.Config().GetString("forecast_tomorrow_prefix", forecast_view.tomorrow_prefix);
    forecast_view.aura_stage_prefix = runtime.Config().GetString("forecast_aura_stage_prefix", forecast_view.aura_stage_prefix);
    forecast_view.tide_type_prefix = runtime.Config().GetString("forecast_tide_type_prefix", forecast_view.tide_type_prefix);
    forecast_view.bonus_format_prefix = runtime.Config().GetString("forecast_bonus_prefix", forecast_view.bonus_format_prefix);
    forecast_view.bonus_midfix = runtime.Config().GetString("forecast_bonus_midfix", forecast_view.bonus_midfix);
    forecast_view.tide_countdown_prefix = runtime.Config().GetString("forecast_tide_countdown_prefix", forecast_view.tide_countdown_prefix);
    forecast_view.tide_countdown_suffix = runtime.Config().GetString("forecast_tide_countdown_suffix", forecast_view.tide_countdown_suffix);
    forecast_view.recommendations_title = runtime.Config().GetString("forecast_recommendations_title", forecast_view.recommendations_title);
    forecast_view.today_state_text = cloud_system.CurrentStateText();
    forecast_view.tomorrow_state_text = cloud_system.ForecastStateText();
    forecast_view.tide_countdown_days = cloud_system.TideCountdownDays(world_state.GetClock().Day());
    forecast_view.aura_stage_text = AuraStageText_(cloud_system.SpiritEnergy());
    forecast_view.tide_type_text = TideTypeText_(forecast_view.tide_countdown_days);
    forecast_view.crop_bonus_percent = static_cast<int>((runtime.CloudMultiplier() - 1.0f) * 100.0f);
    forecast_view.spirit_bonus = cloud_system.SpiritEnergyGain();
    forecast_view.recommendations = BuildDailyRecommendations(
        world_state.GetClock(),
        cloud_system.CurrentState(),
        cloud_system,
        world_state.GetMainHouseRepair(),
        world_state.GetInventory(),
        world_state.GetTeaMachine(),
        world_state.GetSpiritBeast(),
        world_state.GetSpiritBeastWateredToday(),
        world_state.GetTeaPlots(),
        world_state.GetNpcs());
    hud.UpdateCloudForecast(forecast_view);

    PlayerStatusViewData status_view;
    status_view.player_name = runtime.Config().GetString("player_name", "云海旅人");
    status_view.header_level_separator = runtime.Config().GetString("player_status_level_separator", status_view.header_level_separator);
    status_view.manor_stage_prefix = runtime.Config().GetString("player_status_manor_stage_prefix", status_view.manor_stage_prefix);
    status_view.total_gold_prefix = runtime.Config().GetString("player_status_total_gold_prefix", status_view.total_gold_prefix);
    status_view.stamina_label = runtime.Config().GetString("player_status_stamina_label", status_view.stamina_label);
    status_view.spirit_label = runtime.Config().GetString("player_status_spirit_label", status_view.spirit_label);
    status_view.fatigue_label = runtime.Config().GetString("player_status_fatigue_label", status_view.fatigue_label);
    status_view.contract_progress_prefix = runtime.Config().GetString("player_status_contract_progress_prefix", status_view.contract_progress_prefix);
    status_view.contract_total = std::max(1, static_cast<int>(runtime.Config().GetFloat("player_status_contract_total", static_cast<float>(status_view.contract_total))));
    status_view.player_level = runtime.Systems().GetSkills().GetLevel(CloudSeamanor::domain::SkillType::SpiritFarm);
    status_view.manor_stage = world_state.GetMainHouseRepair().level;
    status_view.total_gold = world_state.GetGold();
    status_view.stamina_ratio = world_state.GetStamina().Ratio();
    status_view.spirit_ratio = std::min(1.0f, std::max(0.0f, static_cast<float>(runtime.Systems().GetCloud().SpiritEnergy()) / 200.0f));
    status_view.fatigue_ratio = std::min(1.0f, std::max(0.0f, 1.0f - world_state.GetStamina().Ratio()));
    status_view.contract_progress = runtime.Systems().GetContracts().CompletedVolumeCount();
    hud.UpdatePlayerStatus(status_view);
}

}  // namespace CloudSeamanor::engine
