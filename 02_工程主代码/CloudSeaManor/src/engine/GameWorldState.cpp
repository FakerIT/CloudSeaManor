#include "CloudSeamanor/engine/GameWorldState.hpp"

#include "CloudSeamanor/infrastructure/GameConstants.hpp"
#include "CloudSeamanor/app/GameAppText.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"
#include "CloudSeamanor/infrastructure/UiLayoutConfig.hpp"
#include "CloudSeamanor/engine/rendering/UiPanelInitializer.hpp"
#include "CloudSeamanor/engine/TextRenderUtils.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace CloudSeamanor::engine {

namespace {
using CloudSeamanor::rendering::toSfString;
}

// ============================================================================
// 【GameWorldState】构造函数
// ============================================================================
GameWorldState::GameWorldState()
    : session_time_(0.0f)
    , spirit_beast_watered_today_(false)
    , level_up_overlay_active_(false)
    , level_up_overlay_timer_(0.0f)
    , level_up_skill_type_(CloudSeamanor::domain::SkillType::SpiritFarm)
    , low_stamina_warning_active_(false)
    , world_tip_pulse_(0.0f)
    , font_loaded_(false)
    , in_battle_mode_(false)
    , battle_available_(true) {
}

// ============================================================================
// 【InitializePanels】初始化 UI 面板
// ============================================================================
void GameWorldState::InitializePanels() {
    UiPanelInitializer::InitializePanels(panels_);
}

// ============================================================================
// 【InitializeTexts】初始化 UI 文本
// ============================================================================
void GameWorldState::InitializeTexts(const sf::Font& font) {
    if (!font_loaded_) {
        font_loaded_ = font.getInfo().family != "";
    }
    UiPanelInitializer::InitializeTexts(font, texts_);
}

// ============================================================================
// 【InitializeWorld】初始化世界
// ============================================================================
void GameWorldState::InitializeWorld(const WorldConfig& config) {
    config_ = config;
    session_time_ = 0.0f;
    spirit_beast_watered_today_ = false;
    level_up_overlay_active_ = false;
    level_up_overlay_timer_ = 0.0f;
    low_stamina_warning_active_ = false;
    world_tip_pulse_ = 0.0f;
    festival_notice_text_.clear();
    active_festival_id_.clear();
    festival_runtime_.Reset();
    gold_ = 500;
    inn_orders_.clear();
    inn_gold_reserve_ = 0;
    inn_visitors_today_ = 0;
    inn_income_today_ = 0;
    inn_reputation_ = 0;
    coop_fed_today_ = 0;
    livestock_eggs_today_ = 0;
    livestock_milk_today_ = 0;
    weekly_reports_.clear();
    diary_entries_.clear();
    recipe_unlocks_.clear();
    skill_branches_.clear();
    pending_skill_branches_.clear();
    placed_objects_.clear();
    purify_return_days_ = 0;
    purify_return_spirits_ = 0;
    fishing_attempts_ = 0;
    last_fish_catch_.clear();
    greenhouse_unlocked_ = false;
    greenhouse_tag_next_planting_ = false;
    spirit_realm_daily_max_ = 5;
    spirit_realm_daily_remaining_ = spirit_realm_daily_max_;
    in_battle_mode_ = false;
    battle_available_ = true;
    battle_active_partners_.clear();

    interaction_.highlighted_index = -1;
    interaction_.highlighted_plot_index = -1;
    interaction_.highlighted_npc_index = -1;
    interaction_.spirit_beast_highlighted = false;
    interaction_.dialogue_text = "暂时还没有对话。";
    interaction_.hint_message = "欢迎回到云海山庄。";
    interaction_.hint_timer = 0.0f;

    tutorial_.intro_move_hint_shown = false;
    tutorial_.intro_interact_hint_shown = false;
    tutorial_.intro_crop_hint_shown = false;
    tutorial_.intro_save_hint_shown = false;
    tutorial_.show_debug_overlay = true;
    tutorial_.daily_cloud_report_day_shown = -1;
    tutorial_.tutorial_bubble_completed_mask = 0;
    tutorial_.tutorial_bubble_step = 1;

    // 运行期容器预分配，降低动态扩容频率。
    if (ground_tiles_.capacity() < 96) {
        ground_tiles_.reserve(96);  // fallback 场景 8x12
    }
    if (npcs_.capacity() < 13) {
        npcs_.reserve(13);
    }
    SyncSceneVisuals();
}

void GameWorldState::SyncSceneVisuals() {
    scene_visuals_.SyncTeaPlots(tea_plots_);
    scene_visuals_.SyncNpcs(npcs_);
    scene_visuals_.SyncSpiritBeast(spirit_beast_);
}

// ============================================================================
// 【SetHintMessage】设置提示消息（带类型）
// ============================================================================
void SetHintMessage(GameWorldState& state, const std::string& message, float duration, HintType type) {
    state.MutableInteraction().hint_message = message;
    state.MutableInteraction().hint_timer = duration;
    state.MutableInteraction().hint_type = type;
}

// ============================================================================
// 【SetWarningMessage】快捷函数：设置警告提示（红色）
// ============================================================================
void SetWarningMessage(GameWorldState& state, const std::string& message, float duration) {
    SetHintMessage(state, message, duration, HintType::Warning);
}

// ============================================================================
// 【SetSuccessMessage】快捷函数：设置成功提示（绿色）
// ============================================================================
void SetSuccessMessage(GameWorldState& state, const std::string& message, float duration) {
    SetHintMessage(state, message, duration, HintType::Success);
}

// ============================================================================
// 【UpdateStaminaBar】更新体力条显示
// ============================================================================
void UpdateStaminaBar(GameWorldState& state) {
    const float ratio = state.GetStamina().Ratio();
    const sf::Vector2f bg_size = state.MutablePanels().stamina_bar_bg.getSize();
    state.MutablePanels().stamina_bar_fill.setSize({bg_size.x * ratio, bg_size.y});
}

// ============================================================================
// 【UpdateWorkshopProgressBar】更新工坊进度条显示
// ============================================================================
void UpdateWorkshopProgressBar(GameWorldState& state, const CloudSeamanor::domain::WorkshopSystem& workshop) {
    float ratio = 0.0f;
    if (const auto* machine = workshop.GetMachine("tea_machine")) {
        ratio = machine->is_processing ? machine->progress / 100.0f : 0.0f;
    }
    const sf::Vector2f bg_size = state.MutablePanels().workshop_progress_bg.getSize();
    state.MutablePanels().workshop_progress_fill.setSize({bg_size.x * ratio, bg_size.y});
}

// ============================================================================
// 【UpdateWorldTipPulse】更新世界提示脉冲
// ============================================================================
void UpdateWorldTipPulse(GameWorldState& state, float delta_seconds) {
    if (!std::isfinite(delta_seconds) || delta_seconds < 0.0f) {
        delta_seconds = 0.0f;
    }

    float world_tip_pulse = state.GetWorldTipPulse();
    world_tip_pulse += delta_seconds * GameConstants::Ui::Pulse::TimeScale;
    state.SetWorldTipPulse(world_tip_pulse);

    auto* world_tip_text = state.MutableTexts().world_tip_text.get();
    if (world_tip_text == nullptr) {
        return;
    }

    const float pulse = GameConstants::Ui::Pulse::Bias
        + GameConstants::Ui::Pulse::Amplitude
            * std::sin(world_tip_pulse * GameConstants::Ui::Pulse::Frequency);
    const float alpha = std::clamp(
        static_cast<float>(GameConstants::Ui::Pulse::AlphaBase)
            + pulse * GameConstants::Ui::Pulse::AlphaRange,
        0.0f,
        255.0f);

    const sf::Color world_tip_base_color = adapter::PackedRgbaToColor(
        infrastructure::UiLayoutConfig::GetDefaults().texts["world_tip"].color);
    world_tip_text->setFillColor(
        sf::Color(
            world_tip_base_color.r,
            world_tip_base_color.g,
            world_tip_base_color.b,
            static_cast<std::uint8_t>(alpha)));

    // 轻微上下浮动参数改为配置驱动，避免 UI 魔法数字散落。
    const auto base_pos = infrastructure::UiLayoutConfig::GetDefaults().texts["world_tip"].position;
    const auto& ui_defaults = infrastructure::UiLayoutConfig::GetDefaults();
    const float wave_amplitude =
        ui_defaults.semantic_numbers.count("world_tip_wave_amplitude")
            ? ui_defaults.semantic_numbers.at("world_tip_wave_amplitude")
            : 2.0f;
    const float wave_period =
        std::max(0.01f,
                 ui_defaults.semantic_numbers.count("world_tip_wave_period")
                     ? ui_defaults.semantic_numbers.at("world_tip_wave_period")
                     : 0.5f);
    const float wave = std::sin(
                           world_tip_pulse
                           * (2.0f * std::numbers::pi_v<float> / wave_period))
        * wave_amplitude;
    world_tip_text->setPosition({base_pos[0], base_pos[1] + wave});
}

// ============================================================================
// 【ResetDailyInteractionState】重置每日交互状态
// ============================================================================
void ResetDailyInteractionState(GameWorldState& state, int current_day) {
    auto& spirit_beast = state.MutableSpiritBeast();
    spirit_beast.daily_interacted = false;
    spirit_beast.last_interaction_day = current_day;
    spirit_beast.interact_timer = 0.0f;
    spirit_beast.idle_timer = 0.0f;
    spirit_beast.state = SpiritBeastState::Idle;
    spirit_beast.position = spirit_beast.home_position;
    spirit_beast.patrol_index = 0;
    state.SetSpiritBeastWateredToday(false);

    for (auto& npc : state.MutableNpcs()) {
        npc.daily_gifted = false;
        npc.daily_favor_gain = 0;
        npc.daily_talked = false;
        npc.last_talk_day = current_day - 1;
        npc.heart_level = NpcHeartLevelFromFavor(npc.favor);
    }
}

// ============================================================================
// 【ResetPlotsWateredState】重置地块浇水状态
// ============================================================================
void ResetPlotsWateredState(GameWorldState& state,
                              const std::function<void(TeaPlot&, bool)>& refresh_visual) {
    for (auto& plot : state.MutableTeaPlots()) {
        plot.watered = false;
        refresh_visual(plot, false);
    }
}

} // namespace CloudSeamanor::engine
