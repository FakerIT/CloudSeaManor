#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/GameRuntime.hpp"
#include "CloudSeamanor/CloudSeaManor.hpp"
#include "CloudSeamanor/PlayerInteractRuntime.hpp"
#include "CloudSeamanor/GameAppText.hpp"
#include "CloudSeamanor/TextRenderUtils.hpp"
#include "CloudSeamanor/GameConstants.hpp"
#include "CloudSeamanor/EventBus.hpp"
#include "CloudSeamanor/NpcDialogueManager.hpp"
#include "CloudSeamanor/FarmingLogic.hpp"
#include "CloudSeamanor/Profiling.hpp"

#include "CloudSeamanor/engine/systems/PlayerMovementSystem.hpp"
#include "CloudSeamanor/engine/systems/PickupSystemRuntime.hpp"
#include "CloudSeamanor/engine/systems/CropGrowthSystem.hpp"
#include "CloudSeamanor/engine/systems/NpcScheduleSystem.hpp"
#include "CloudSeamanor/engine/systems/SpiritBeastSystem.hpp"
#include "CloudSeamanor/engine/systems/TutorialSystem.hpp"
#include "CloudSeamanor/engine/systems/WorkshopSystemRuntime.hpp"
#include "CloudSeamanor/engine/BattleUI.hpp"

#include <cmath>
#include <array>
#include <cstdint>
#include <fstream>
#include <optional>
#include <random>
#include <sstream>
#include <unordered_map>

namespace CloudSeamanor::engine {

GameRuntime::GameRuntime() = default;

namespace {
std::string SeasonalBgmPath_(CloudSeamanor::domain::Season season) {
    using CloudSeamanor::domain::Season;
    switch (season) {
    case Season::Spring: return "assets/audio/bgm/spring_theme.ogg";
    case Season::Summer: return "assets/audio/bgm/summer_theme.ogg";
    case Season::Autumn: return "assets/audio/bgm/autumn_theme.ogg";
    case Season::Winter: return "assets/audio/bgm/winter_theme.ogg";
    }
    return "assets/audio/bgm/spring_theme.ogg";
}
} // namespace

namespace {
std::vector<PriceTableEntry> LoadPriceTable_(const std::string& path) {
    std::vector<PriceTableEntry> entries;
    std::ifstream in(path);
    if (!in.is_open()) {
        return entries;
    }
    std::string line;
    if (!std::getline(in, line)) {
        return entries;
    }
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string f;
        PriceTableEntry e;
        int col = 0;
        while (std::getline(ss, f, ',')) {
            switch (col) {
            case 0: e.item_id = f; break;
            case 1: e.buy_price = std::max(0, std::stoi(f)); break;
            case 2: e.sell_price = std::max(0, std::stoi(f)); break;
            case 3: e.buy_from = f; break;
            case 4: e.category = f; break;
            default: break;
            }
            ++col;
        }
        if (!e.item_id.empty()) entries.push_back(std::move(e));
    }
    return entries;
}

std::vector<PriceTableEntry> LoadPriceTableFromRoots_(
    const std::vector<std::filesystem::path>& data_roots) {
    std::unordered_map<std::string, PriceTableEntry> merged;
    for (const auto& root : data_roots) {
        const auto path = root / "PriceTable.csv";
        const auto entries = LoadPriceTable_(path.string());
        for (const auto& entry : entries) {
            merged[entry.item_id] = entry;
        }
    }
    std::vector<PriceTableEntry> out;
    out.reserve(merged.size());
    for (const auto& kv : merged) {
        out.push_back(kv.second);
    }
    return out;
}

std::string ResolveDataFileFromRoots_(
    const std::vector<std::filesystem::path>& data_roots,
    const std::string& relative_file,
    const std::string& fallback_path) {
    std::string selected = fallback_path;
    for (const auto& root : data_roots) {
        const auto candidate = root / relative_file;
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec) && std::filesystem::is_regular_file(candidate, ec)) {
            selected = candidate.string();
        }
    }
    return selected;
}

std::string ResolveMapFromMods_(
    const CloudSeamanor::engine::ModLoader& mod_loader,
    const std::string& requested_map_path) {
    std::string resolved_map_path = requested_map_path;
    std::error_code ec;
    for (const auto& mod : mod_loader.LoadedMods()) {
        const auto mod_map = std::filesystem::path("mods")
            / mod.id
            / "maps"
            / std::filesystem::path(requested_map_path).filename();
        if (std::filesystem::exists(mod_map, ec) && std::filesystem::is_regular_file(mod_map, ec)) {
            resolved_map_path = mod_map.string();
        }
    }
    return resolved_map_path;
}

std::string CurrentSeasonTag_(CloudSeamanor::domain::Season season) {
    using CloudSeamanor::domain::Season;
    switch (season) {
    case Season::Spring: return "spring";
    case Season::Summer: return "summer";
    case Season::Autumn: return "autumn";
    case Season::Winter: return "winter";
    }
    return "spring";
}
}

// ============================================================================
// 【GameRuntime::Initialize】初始化
// ============================================================================
void GameRuntime::Initialize(
    const std::string& config_path,
    const std::string& schedule_path,
    const std::string& gift_path,
    const std::string& npc_text_path,
    const std::string& map_path,
    const GameRuntimeCallbacks& callbacks
) {
    callbacks_ = callbacks;
    (void)mod_loader_.LoadAll("mods");
    world_state_.GetModHooks() = mod_loader_.Hooks();
    const auto data_roots = mod_loader_.BuildDataRoots("assets/data");
    const std::string resolved_schedule_path = ResolveDataFileFromRoots_(
        data_roots, "Schedule_Data.csv", schedule_path);
    const std::string resolved_gift_path = ResolveDataFileFromRoots_(
        data_roots, "Gift_Preference.json", gift_path);
    const std::string resolved_npc_text_path = ResolveDataFileFromRoots_(
        data_roots, "NPC_Texts.json", npc_text_path);
    const std::string resolved_quest_path = ResolveDataFileFromRoots_(
        data_roots, "QuestTable.csv", "assets/data/QuestTable.csv");
    const std::string resolved_delivery_path = ResolveDataFileFromRoots_(
        data_roots, "NpcDeliveryTable.csv", "assets/data/NpcDeliveryTable.csv");
    const std::string resolved_crop_table_path = ResolveDataFileFromRoots_(
        data_roots, "CropTable.csv", "assets/data/CropTable.csv");
    std::string resolved_map_path = ResolveMapFromMods_(mod_loader_, map_path);
    dialogue_data_root_ = std::filesystem::path(resolved_schedule_path).parent_path().string();
    if (dialogue_data_root_.empty()) {
        dialogue_data_root_ = "assets/data";
    }

    // 日志初始化
    const bool config_loaded = config_.LoadFromFile(config_path);
    const std::uintmax_t log_max_kb =
        static_cast<std::uintmax_t>(config_.GetFloat("log_max_kb", 512.0f));
    CloudSeamanor::infrastructure::Logger::Initialize("logs", "CloudSeamanor.log", log_max_kb * 1024);
    CloudSeamanor::infrastructure::Logger::Info(
        config_loaded ? "已加载 gameplay 配置。" : "未找到 gameplay 配置，使用默认值。");
    const int dialogue_typing_speed_ms = static_cast<int>(
        config_.GetFloat("dialogue_typing_speed_ms", 45.0f));

    // 验证数据资产
    ValidateDataAsset(resolved_schedule_path, "Schedule_Data.csv",
        &CloudSeamanor::infrastructure::Logger::Error,
        &CloudSeamanor::infrastructure::Logger::Info);
    ValidateDataAsset(resolved_gift_path, "Gift_Preference.json",
        &CloudSeamanor::infrastructure::Logger::Error,
        &CloudSeamanor::infrastructure::Logger::Info);
    ValidateDataAsset(resolved_npc_text_path, "NPC_Texts.json",
        &CloudSeamanor::infrastructure::Logger::Error,
        &CloudSeamanor::infrastructure::Logger::Info);

    // 初始化世界配置
    WorldConfig world_config;
    world_config.player_speed = config_.GetFloat("player_speed", GameConstants::Player::Speed);
    world_config.stamina_move_per_second = config_.GetFloat("stamina_move_per_second", GameConstants::Player::StaminaMovePerSecond);
    world_config.stamina_interact_cost = config_.GetFloat("stamina_interact_cost", GameConstants::Player::StaminaInteractCost);
    world_config.stamina_recover_per_second = config_.GetFloat("stamina_recover_per_second", GameConstants::Player::StaminaRecoverPerSecond);
    time_scale_ = config_.GetFloat("time_scale", GameConstants::Runtime::DefaultTimeScale);

    world_state_.GetConfig() = world_config;
    world_state_.GetStamina().SetMax(config_.GetFloat("player_stamina_max", GameConstants::Player::StaminaMax));
    world_state_.GetStamina().SetCurrent(config_.GetFloat("player_stamina", GameConstants::Player::StaminaInitial));

    // 初始化 UI 面板
    world_state_.InitializePanels();

    // 构建场景
    BuildScene(
        tmx_map_,
        world_state_.GetGroundTiles(),
        world_state_.GetObstacleShapes(),
        world_state_.GetObstacleBounds(),
        world_state_.GetInteractables(),
        world_state_.GetPickups(),
        world_config.world_bounds,
        resolved_map_path,
        &CloudSeamanor::infrastructure::Logger::Info,
        CloudSeamanor::infrastructure::Logger::Warning);
    map_root_override_ = std::filesystem::path(resolved_map_path).parent_path().string();
    SetCropTableDataPath(resolved_crop_table_path);

    // 初始化农业系统
    auto farming_callbacks = CreateDefaultPlotCallbacks(
        [this](const std::string& msg, float dur) { callbacks_.push_hint(msg, dur); },
        &CloudSeamanor::infrastructure::Logger::Info,
        RefreshTeaPlotVisual);
    farming_.Initialize(farming_callbacks);
    farming_.BuildDefaultPlots();
    // B-24 茶园：独立于普通农田的长期茶树地块（显式深拷贝，后续独立更新）。
    world_state_.GetTeaGardenPlots().clear();
    world_state_.GetTeaGardenPlots().reserve(farming_.Plots().size());
    for (const auto& base_plot : farming_.Plots()) {
        TeaPlot p = base_plot;
        p.crop_name = "茶园-" + p.crop_name;
        p.growth_time = std::max(160.0f, p.growth_time * 2.0f);
        world_state_.GetTeaGardenPlots().push_back(std::move(p));
    }

    // 初始化作物生长系统
    crop_growth_.SetHintCallback(
        [this](const std::string& msg, float dur) { callbacks_.push_hint(msg, dur); });
    crop_growth_.SetFertilizerMultipliers(
        config_.GetFloat("fertilizer_basic_multiplier", 1.2f),
        config_.GetFloat("fertilizer_premium_multiplier", 1.5f));

    // 初始化灵兽
    BuildSpiritBeast(world_state_.GetSpiritBeast(),
                    world_state_.GetClock(),
                    world_state_.GetSpiritBeastWateredToday(),
                    world_state_.GetInteraction().spirit_beast_highlighted);

    // 初始化 NPC
    LoadNpcTextMappings(resolved_npc_text_path, npc_text_mappings_);
    BuildNpcs(resolved_schedule_path, resolved_gift_path, npc_text_mappings_, world_state_.GetNpcs());

    // 初始化 NPC 对话管理器（支持 mod 覆盖的数据根）
    dialogue_manager_ = NpcDialogueManager(dialogue_data_root_);
    dialogue_manager_.InitializeHeartEvents();
    for (const auto& npc : world_state_.GetNpcs()) {
        dialogue_manager_.LoadDailyDialogue(npc.id);
    }
    dialogue_manager_.SetNpcList(&world_state_.GetNpcs());

    // 初始化拾取系统运行时
    pickup_runtime_ = std::make_unique<PickupSystemRuntime>(
        pickups_,
        world_state_,
        [this](const std::string& msg, float dur) { callbacks_.push_hint(msg, dur); });
    pickup_runtime_->Initialize();
    tutorial_system_ = std::make_unique<TutorialSystem>(
        world_state_,
        [this](const std::string& msg, float dur) { callbacks_.push_hint(msg, dur); });

    // 初始化系统
    systems_.InitializeContracts();
    systems_.Initialize();
    systems_.GetCloud().AdvanceToNextDay(0);
    systems_.GetCloud().UpdateForecastVisibility(0, 0);
    workshop_runtime_ = std::make_unique<WorkshopSystemRuntime>(
        systems_,
        world_state_,
        [this](const std::string& msg, float dur) { callbacks_.push_hint(msg, dur); });

    SyncTeaMachineFromWorkshop_();
    battle_manager_.Initialize(&systems_.GetCloud());
    battle_manager_.SetRewardCallbacks(BattleManager::RewardCallbacks{
        .on_item_reward = [this](const std::string& item_id, int count) {
            world_state_.GetInventory().AddItem(item_id, count);
        },
        .on_exp_reward = [this](float exp) {
            (void)systems_.GetSkills().AddExp(
                CloudSeamanor::domain::SkillType::SpiritForage, exp, systems_.GetCloud().CurrentSpiritDensity());
        },
        .on_partner_favor_reward = [this](const std::string&, int favor_delta) {
            world_state_.GetSpiritBeast().favor += favor_delta;
        },
        .on_notice = [this](const std::string& message) {
            callbacks_.push_hint(message, 2.8f);
        }
    });

    // 初始化世界
    world_state_.InitializeWorld(world_config);
    world_state_.GetInteraction().dialogue_engine.SetTypingSpeed(
        std::clamp(dialogue_typing_speed_ms, 20, 100));
    world_state_.GetInteraction().dialogue_text =
        "欢迎回家。在云海山庄里慢慢建立属于你的日常：种田、观云、结缘灵兽、与人来往。"
        "每日22:00可查看次日云海预报；云海和灵气共同影响作物与灵茶收益；"
        "完成云海守护者契约可获得永久增益。这里没有硬性期限，错过的内容来年也能再体验。";
    SetHintMessage(world_state_,
        "欢迎来到云海山庄。先用 WASD 移动，并观察附近的高亮提示；"
        "这里没有时间压力，可以按自己的节奏慢慢来。", GameConstants::Ui::HintDuration::Welcome);

    // 初始物品
    world_state_.GetInventory().AddItem("TeaSeed", GameConstants::Initial::TeaSeedCount);
    world_state_.GetInventory().AddItem("TurnipSeed", GameConstants::Initial::TurnipSeedCount);
    world_state_.GetInventory().AddItem("Wood", GameConstants::Initial::WoodCount);
    // 工具耐久（MVP：用背包数量表示剩余使用次数）
    world_state_.GetInventory().AddItem("ToolHoe", 60);
    world_state_.GetInventory().AddItem("ToolAxe", 60);
    world_state_.GetInventory().AddItem("ToolPickaxe", 60);
    world_state_.GetInventory().AddItem("ToolSickle", 60);
    world_state_.GetInventory().AddItem("SprinklerItem", 2);
    world_state_.GetInventory().AddItem("FertilizerItem", 4);
    world_state_.GetPriceTable() = LoadPriceTableFromRoots_(data_roots);
    const auto grant_achievement_reward = [this](const std::string& achievement_id) {
        if (achievement_id == "first_crop") {
            world_state_.GetGold() += 100;
            callbacks_.push_hint("成就奖励：金币 +100", 1.8f);
        } else if (achievement_id == "ten_crops") {
            world_state_.GetGold() += 500;
            callbacks_.push_hint("成就奖励：金币 +500", 1.8f);
        } else if (achievement_id == "gift_expert") {
            world_state_.GetInventory().AddItem("TeaPack", 1);
            callbacks_.push_hint("成就奖励：茶包 x1", 1.8f);
        } else if (achievement_id == "master_builder") {
            world_state_.GetInventory().AddItem("Wood", 5);
            callbacks_.push_hint("成就奖励：木材 x5", 1.8f);
        } else if (achievement_id == "home_designer") {
            world_state_.GetGold() += 200;
            callbacks_.push_hint("成就奖励：金币 +200", 1.8f);
        } else if (achievement_id == "beast_bond") {
            world_state_.GetInventory().AddItem("Feed", 1);
            callbacks_.push_hint("成就奖励：饲料 x1", 1.8f);
        } else if (achievement_id == "beast_bond_max") {
            world_state_.GetGold() += 300;
            callbacks_.push_hint("成就奖励：金币 +300", 1.8f);
        } else if (achievement_id == "beast_all_types") {
            world_state_.GetInventory().AddItem("star_fragment", 1);
            callbacks_.push_hint("成就奖励：星辰碎片 x1", 1.8f);
        }
    };
    GlobalEventBus().Subscribe("harvest", [this, grant_achievement_reward](const Event& event) {
        achievement_system_.HandleEvent(
            world_state_.GetAchievements(),
            event,
            [this](const std::string& msg) { callbacks_.push_hint(msg, 2.2f); },
            grant_achievement_reward);
    });
    GlobalEventBus().Subscribe("gift", [this, grant_achievement_reward](const Event& event) {
        achievement_system_.HandleEvent(
            world_state_.GetAchievements(),
            event,
            [this](const std::string& msg) { callbacks_.push_hint(msg, 2.2f); },
            grant_achievement_reward);
    });
    GlobalEventBus().Subscribe("build", [this, grant_achievement_reward](const Event& event) {
        achievement_system_.HandleEvent(
            world_state_.GetAchievements(),
            event,
            [this](const std::string& msg) { callbacks_.push_hint(msg, 2.2f); },
            grant_achievement_reward);
    });
    GlobalEventBus().Subscribe("beast_bond", [this, grant_achievement_reward](const Event& event) {
        achievement_system_.HandleEvent(
            world_state_.GetAchievements(),
            event,
            [this](const std::string& msg) { callbacks_.push_hint(msg, 2.2f); },
            grant_achievement_reward);
    });
    GlobalEventBus().Subscribe("beast_type_collected", [this, grant_achievement_reward](const Event& event) {
        achievement_system_.HandleEvent(
            world_state_.GetAchievements(),
            event,
            [this](const std::string& msg) { callbacks_.push_hint(msg, 2.2f); },
            grant_achievement_reward);
    });
    (void)quest_manager_.LoadFromCsv(resolved_quest_path, world_state_.GetRuntimeQuests());
    (void)npc_delivery_.LoadFromCsv(resolved_delivery_path);
    npc_delivery_.SetHintCallback([this](const std::string& msg, float dur) { callbacks_.push_hint(msg, dur); });

    // 节日系统
    systems_.GetFestivals().Initialize();
    systems_.GetFestivals().Update(world_state_.GetClock().Season(),
                                      world_state_.GetClock().DayInSeason());
    {
        const std::string fest_notice = systems_.GetFestivals().GetNoticeText();
        world_state_.SetFestivalNoticeText(fest_notice);
    }

    // 主线剧情系统
    plot_system_.Initialize(dialogue_data_root_);
    plot_system_.SetGameClock(&world_state_.GetClock());
    plot_system_.SetCloudSystem(&systems_.GetCloud());
    plot_system_.SetNpcHeartGetter([this](const std::string& npc_id) -> int {
        for (const auto& npc : world_state_.GetNpcs()) {
            if (npc.id == npc_id) {
                return npc.heart_level;
            }
        }
        return 0;
    });
    plot_system_.SetCallbacks(PlotCallbacks{
        .on_chapter_start = [this](const std::string& ch) {
            callbacks_.push_hint("章节开始：" + ch, 3.0f);
        },
        .on_chapter_complete = [this](const std::string& ch) {
            callbacks_.push_hint("章节完成：" + ch, 3.0f);
        },
        .on_plot_start = [this](const std::string& plot_id) {
            callbacks_.push_hint("剧情触发：" + plot_id, 2.0f);
        },
        .on_plot_complete = [this](const std::string& plot_id) {
            callbacks_.push_hint("剧情完成：" + plot_id, 2.0f);
        },
        .on_unlock_region = [this](const std::string& region) {
            callbacks_.push_hint("新区域解锁：" + region, 3.0f);
        },
        .on_unlock_npc = [this](const std::string& npc) {
            callbacks_.push_hint("新角色加入山庄：" + npc, 3.0f);
        },
        .on_flag_set = [](const std::string&) {},
        .on_cloud_delta = [this](int delta) {
            systems_.GetCloud().SetSpiritEnergy(
                systems_.GetCloud().SpiritEnergy() + delta);
            callbacks_.push_hint(std::string("灵气 ") + (delta >= 0 ? "+" : "") +
                                    std::to_string(delta), 2.0f);
        },
        .on_favor_delta = [](const std::string&, int) {},
        .on_route_lock = [this](const std::string& route) {
            callbacks_.push_hint("路线已锁定：" + route, 4.0f);
        },
        .on_notice = [this](const std::string& notice) {
            callbacks_.push_hint(notice, 4.0f);
        },
        .log_info = &CloudSeamanor::infrastructure::Logger::Info,
    });
    plot_system_.CheckPlotTriggers();  // 第1天自动触发序章

    CloudSeamanor::infrastructure::Logger::Info("GameRuntime 初始化完成。");
}

// ============================================================================
// 【GameRuntime::SetWindow】设置窗口
// ============================================================================
void GameRuntime::SetWindow(sf::RenderWindow* window) {
    window_ = window;
}

// ============================================================================
// 【GameRuntime::CloudMultiplier】云海倍率
// ============================================================================
float GameRuntime::CloudMultiplier() const {
    return CloudGrowthMultiplier(systems_.GetCloud().CurrentState());
}

// ============================================================================
// 【GameRuntime::OnDayChanged】日期变化
// ============================================================================
void GameRuntime::OnDayChanged() {
    const int spirit_realm_daily_max = std::max(1, static_cast<int>(config_.GetFloat("spirit_realm_daily_max", 5.0f)));
    world_state_.SetSpiritRealmDailyMax(spirit_realm_daily_max);
    world_state_.SetSpiritRealmDailyRemaining(spirit_realm_daily_max);

    const float sprinkler_radius = config_.GetFloat("sprinkler_radius", 80.0f);
    const auto current_season = world_state_.GetClock().Season();
    const auto season_changed_event = world_state_.GetClock().ConsumeSeasonChangedEvent();
    const float cloud_density = systems_.GetCloud().CurrentSpiritDensity();

    systems_.UpdateDaily(systems_.GetCloud().CurrentState() == CloudSeamanor::domain::CloudState::Mist
                             ? CloudSeamanor::domain::Season::Spring
                             : CloudSeamanor::domain::Season::Spring,
                         static_cast<int>(world_state_.GetSessionTime() / 100),
                         cloud_density);

    const int current_day = world_state_.GetClock().Day();
    if (last_reset_day_ != current_day) {
        farming_.ResetDailyState(RefreshTeaPlotVisual);
        ResetDailyInteractionState(world_state_,
                                   systems_.GetCloud().CurrentState() == CloudSeamanor::domain::CloudState::Clear
                                       ? 0 : 1);
        last_reset_day_ = current_day;
    }
    crop_growth_.HandleDailyDiseaseAndPest(world_state_);

    for (auto& plot : world_state_.GetTeaPlots()) {
        if (plot.sprinkler_installed) {
            if (plot.sprinkler_days_left > 0) {
                --plot.sprinkler_days_left;
            }
            if (plot.sprinkler_days_left <= 0) {
                plot.sprinkler_installed = false;
            } else if (plot.seeded && !plot.ready) {
                plot.watered = true;
                RefreshTeaPlotVisual(plot, false);
            }
        }
    }

    if (season_changed_event.changed) {
        crop_growth_.HandleSeasonChanged(world_state_, current_season);
        callbacks_.push_hint(
            "季节更替：" + CloudSeamanor::domain::GameClock::SeasonName(season_changed_event.from)
            + " -> " + CloudSeamanor::domain::GameClock::SeasonName(season_changed_event.to),
            2.4f);
    }
    // B4-033: 独立灵界管理器负责稀有节点刷新。
    spirit_realm_manager_.RefreshDailyNodes(world_state_, systems_.GetCloud());
    // A-11 洒水器：对附近地块提供晨间自动浇水
    ApplySprinklerAutoWater(world_state_.GetTeaPlots(), sprinkler_radius);
    for (auto& p : world_state_.GetTeaPlots()) {
        if (p.seeded && p.watered && !p.ready) {
            RefreshTeaPlotVisual(p, false);
        }
    }

    // A-30/B-2 委托面板：每日 6:00 后自动接取委托类任务。
    quest_manager_.RefreshByTimeslot(world_state_.GetRuntimeQuests(), world_state_.GetClock().Hour());

    // C-9/C-10：交由独立系统负责每日结算。
    coop_system_.DailyUpdate(world_state_);
    barn_system_.DailyUpdate(world_state_);
    inn_system_.DailySettlement(world_state_, world_state_.GetMainHouseRepair().level);
    {
        // BE-064: 舒适度影响体力恢复速度（以每日额外恢复量实现）。
        const float comfort_bonus = std::clamp(
            static_cast<float>(world_state_.GetDecorationScore()) * 0.15f,
            0.0f,
            12.0f);
        if (comfort_bonus > 0.0f) {
            world_state_.GetStamina().Recover(comfort_bonus);
            callbacks_.push_hint(
                "家居舒适度恢复体力 +" + std::to_string(static_cast<int>(comfort_bonus)),
                1.8f);
        }
    }
    const auto grant_achievement_reward = [this](const std::string& achievement_id) {
        if (achievement_id == "home_designer") {
            world_state_.GetGold() += 200;
            callbacks_.push_hint("成就奖励：金币 +200", 1.8f);
        } else if (achievement_id == "small_tycoon") {
            world_state_.GetInventory().AddItem("TeaPack", 2);
            callbacks_.push_hint("成就奖励：茶包 x2", 1.8f);
        } else if (achievement_id == "beast_bond_max") {
            world_state_.GetGold() += 300;
            callbacks_.push_hint("成就奖励：金币 +300", 1.8f);
        }
    };
    achievement_system_.EvaluateDaily(
        world_state_.GetAchievements(),
        world_state_.GetDecorationScore(),
        world_state_.GetGold(),
        world_state_.GetSpiritBeast(),
        [this](const std::string& text) { callbacks_.push_hint(text, 2.4f); },
        grant_achievement_reward);
    if (!world_state_.GetModHooks().empty()) {
        callbacks_.push_hint("Mod Hook 触发: OnDayChanged x"
            + std::to_string(world_state_.GetModHooks().size()), 1.4f);
    }

    // B-16 价格波动：每周重置统计，按供需调整价格。
    if (world_state_.GetClock().Day() % 7 == 1) {
        world_state_.GetWeeklyBuyCount().clear();
        world_state_.GetWeeklySellCount().clear();
    }
    for (auto& price : world_state_.GetPriceTable()) {
        const int buy_count = world_state_.GetWeeklyBuyCount()[price.item_id];
        const int sell_count = world_state_.GetWeeklySellCount()[price.item_id];
        if (buy_count > 10 && price.buy_price > 0) {
            price.buy_price = std::min(price.buy_price * 11 / 10, price.buy_price * 2);
        }
        if (sell_count > 20 && price.sell_price > 0) {
            price.sell_price = std::max(1, price.sell_price * 95 / 100);
        }
    }
    // B-17 每日杂货店随机进货。
    world_state_.GetDailyGeneralStoreStock().clear();
    for (const auto& e : world_state_.GetPriceTable()) {
        if (e.buy_from == "general_store") {
            world_state_.GetDailyGeneralStoreStock().push_back(e.item_id);
            if (world_state_.GetDailyGeneralStoreStock().size() >= 3) break;
        }
    }
    quest_manager_.EvaluateProgress(
        world_state_.GetRuntimeQuests(),
        world_state_.GetInventory(),
        systems_.GetSkills().GetLevel(CloudSeamanor::domain::SkillType::SpiritFarm),
        world_state_.GetGold(),
        &world_state_.GetStamina(),
        [this](const std::string& item_id, int count) {
            world_state_.GetInventory().AddItem(item_id, count);
        },
        [this](int favor_delta) {
            auto& npcs = world_state_.GetNpcs();
            if (!npcs.empty()) {
                npcs.front().favor += favor_delta;
            }
        },
        [this](const std::string& text) {
            callbacks_.push_hint(text, 2.2f);
        });

    // BE-027 NPC 心情系统：按天气/季节偏好/送礼行为刷新，并影响当日互动倍率与语气。
    const auto weather = systems_.GetCloud().CurrentState();
    const auto season = world_state_.GetClock().Season();
    auto PreferredSeasonForNpc_ = [](const std::string& npc_id)
        -> std::optional<CloudSeamanor::domain::Season> {
        using CloudSeamanor::domain::Season;
        if (npc_id == "acha") return Season::Spring;
        if (npc_id == "xiaoman") return Season::Summer;
        if (npc_id == "lin") return Season::Autumn;
        if (npc_id == "wanxing") return Season::Winter;
        return std::nullopt;
    };
    for (auto& npc : world_state_.GetNpcs()) {
        // 基础：天气驱动
        switch (weather) {
        case CloudSeamanor::domain::CloudState::Clear:
        case CloudSeamanor::domain::CloudState::Tide:
            npc.mood = NpcMood::Happy;
            break;
        case CloudSeamanor::domain::CloudState::DenseCloud:
        case CloudSeamanor::domain::CloudState::Mist:
        default:
            npc.mood = NpcMood::Normal;
            break;
        }

        // 季节偏好：喜欢当前季节则更愉快
        if (const auto pref = PreferredSeasonForNpc_(npc.id); pref && *pref == season) {
            npc.mood = NpcMood::Happy;
        }

        // 玩家行为：最近送礼更开心，长时间未送礼更低落
        const int days_since_gift = std::max(0, world_state_.GetClock().Day() - npc.last_gift_day);
        if (npc.last_gift_day == world_state_.GetClock().Day()
            || npc.last_gift_day == (world_state_.GetClock().Day() - 1)) {
            npc.mood = NpcMood::Happy;
        } else if (npc.last_gift_day > 0 && days_since_gift >= 3) {
            npc.mood = NpcMood::Sad;
        }
    }

    // BE-075: 四季 BGM 路由：换日时按季节切换；大潮日覆盖为 tide 主题。
    std::string bgm_path = SeasonalBgmPath_(world_state_.GetClock().Season());
    if (systems_.GetCloud().CurrentState() == CloudSeamanor::domain::CloudState::Tide) {
        bgm_path = "assets/audio/bgm/tide_theme.ogg";
    }
    if (bgm_path != current_bgm_path_) {
        if (callbacks_.play_bgm) {
            callbacks_.play_bgm(bgm_path, true, 0.8f, 0.5f);
        }
        current_bgm_path_ = bgm_path;
    }

    auto& mail_orders = world_state_.GetMailOrders();
    for (std::size_t i = 0; i < mail_orders.size();) {
        if (mail_orders[i].deliver_day <= world_state_.GetClock().Day()) {
            world_state_.GetInventory().AddItem(mail_orders[i].item_id, mail_orders[i].count);
            callbacks_.push_hint("邮件已送达：" + mail_orders[i].item_id + " x"
                + std::to_string(mail_orders[i].count), 2.8f);
            mail_orders[i] = mail_orders.back();
            mail_orders.pop_back();
            continue;
        }
        ++i;
    }

    systems_.CheckContractUnlocks();
    const int contract_count = systems_.GetContracts().CompletedVolumeCount();
    if (contract_count > last_contract_completed_count_) {
        Event ev;
        ev.type = "OnContractCompleted";
        ev.data["completed_count"] = std::to_string(contract_count);
        GlobalEventBus().Emit(ev);
        last_contract_completed_count_ = contract_count;
    }

    TrySpiritBeastWateringAid(
        world_state_.GetSpiritBeast(),
        world_state_.GetSpiritBeastWateredToday(),
        world_state_.GetTeaPlots(),
        world_state_.GetStamina(),
        RefreshTeaPlotVisual,
        [this](const std::string& msg, float dur) { callbacks_.push_hint(msg, dur); },
        CloudSeamanor::infrastructure::Logger::Info);
}

// ============================================================================
// 【GameRuntime::OnPlayerMoved】玩家移动
// ============================================================================
void GameRuntime::OnPlayerMoved(float delta_seconds, const sf::Vector2f& direction) {
    player_movement_.Update(world_state_, delta_seconds, direction);
}

// ============================================================================
// 【GameRuntime::OnPlayerInteracted】玩家交互回调
// ============================================================================
void GameRuntime::OnPlayerInteracted(const CloudSeamanor::domain::Interactable& target) {
    world_state_.GetStamina().Consume(world_state_.GetConfig().stamina_interact_cost);

    if (target.Type() == CloudSeamanor::domain::InteractableType::GatheringNode) {
        pickups_.SpawnPickup(
            target.Shape().getPosition() + sf::Vector2f(14.0f, -10.0f),
            target.RewardItem(),
            target.RewardAmount());
        const float cloud_density = systems_.GetCloud().CurrentSpiritDensity();
        const float beast_share = world_state_.GetSpiritBeast().daily_interacted
            ? GameConstants::SpiritBeast::AssistHarvestMultiplier
            : GameConstants::Runtime::DefaultTimeScale;
        if (systems_.GetSkills().AddExp(
                CloudSeamanor::domain::SkillType::SpiritForage,
                GameConstants::Runtime::SpiritForageExpGain,
                cloud_density,
                GameConstants::Runtime::DefaultTimeScale,
                beast_share)) {
            world_state_.SetLevelUpOverlayActive(true);
            world_state_.SetLevelUpOverlayTimer(GameConstants::Ui::LevelUp::OverlayDuration);
            world_state_.SetLevelUpSkillType(CloudSeamanor::domain::SkillType::SpiritForage);
            callbacks_.push_hint(
                "灵觅技能提升至 Lv." +
                std::to_string(systems_.GetSkills().GetLevel(
                    CloudSeamanor::domain::SkillType::SpiritForage)) + "！",
                GameConstants::Ui::HintDuration::SkillLevelUp);
        }
    }
}

// ============================================================================
// 【GameRuntime::Update】每帧更新
// ============================================================================
void GameRuntime::Update(float delta_seconds) {
    if (in_battle_mode_) {
        const auto player_pos = world_state_.GetPlayer().GetPosition();
        battle_manager_.Update(delta_seconds, player_pos.x, player_pos.y);
        if (!battle_manager_.IsInBattle()) {
            in_battle_mode_ = false;
        }
        return;
    }

    CSM_ZONE_SCOPED;
    const int previous_day = world_state_.GetClock().Day();
    world_state_.GetClock().Tick(delta_seconds);
    systems_.GetCloud().UpdateForecastVisibility(
        world_state_.GetClock().Day(), world_state_.GetClock().Hour());

    if (world_state_.GetClock().Day() != previous_day) {
        OnDayChanged();
    }

    // BE-028 NPC 委托：每日 6:00 刷新 + 完成判定（奖励由对话领取）。
    npc_delivery_.Update(world_state_, world_state_.GetClock().Day(), world_state_.GetClock().Hour());

    // BE-029 场景过场动画更新（淡入淡出）
    scene_transition_.Update(delta_seconds);

    // UI-020 云海日报：每日 6:00 自动推送一次（使用通知横幅，点开详面板由前端处理）。
    {
        auto& tutorial = world_state_.GetTutorial();
        const int day = world_state_.GetClock().Day();
        const int hour = world_state_.GetClock().Hour();
        if (hour >= 6 && tutorial.daily_cloud_report_day_shown != day) {
            tutorial.daily_cloud_report_day_shown = day;
            if (callbacks_.push_notification) {
                const auto& cloud = systems_.GetCloud();
                const std::string msg =
                    std::string("☁ 今日云海: ") + cloud.CurrentStateText()
                    + "  作物加成: x" + std::to_string(CloudGrowthMultiplier(cloud.CurrentState()))
                    + "  明日预报: " + (cloud.IsForecastVisible() ? cloud.ForecastStateText() : "22:00后公布");
                callbacks_.push_notification(msg);
            }
        }
    }
    // B-14 灵界自动返回
    if (world_state_.GetInSpiritRealm() && world_state_.GetClock().Hour() >= 22) {
        world_state_.GetInSpiritRealm() = false;
        callbacks_.push_hint("夜深了，你被传送门自动送回主世界。", 2.8f);
    }

    world_state_.SetSessionTime(world_state_.GetSessionTime() + delta_seconds);

    // 提示计时
    if (world_state_.GetInteraction().hint_timer > 0.0f) {
        world_state_.GetInteraction().hint_timer =
            std::max(0.0f, world_state_.GetInteraction().hint_timer - delta_seconds);
    }

    // 升级动画
    if (world_state_.GetLevelUpOverlayActive()) {
        world_state_.SetLevelUpOverlayTimer(
            world_state_.GetLevelUpOverlayTimer() - delta_seconds);
        if (world_state_.GetLevelUpOverlayTimer() <= 0.0f) {
            world_state_.SetLevelUpOverlayActive(false);
        }
    }

    if (tutorial_system_) {
        tutorial_system_->Update(delta_seconds);
    } else {
        CheckTutorialHints();
    }

    // 主线剧情系统更新
    if (plot_system_.IsPlaying()) {
        plot_system_.Update(delta_seconds);
    }

    if (last_cloud_state_ != systems_.GetCloud().CurrentState()) {
        last_cloud_state_ = systems_.GetCloud().CurrentState();
        callbacks_.push_hint(
            "云海气场发生变化：" + systems_.GetCloud().CurrentStateText() + "。" +
            BuildWeatherAdviceText(systems_.GetCloud().CurrentState(),
                                   systems_.GetCloud().IsForecastVisible()),
            GameConstants::Ui::HintDuration::CloudStateChanged);
    }

    // 体力警告
    if (!world_state_.GetLowStaminaWarningActive() && world_state_.GetStamina().Ratio() <= GameConstants::Player::LowStaminaWarningRatio) {
        world_state_.SetLowStaminaWarningActive(true);
        callbacks_.push_hint("体力偏低，先休息一下或放慢节奏。", GameConstants::Ui::HintDuration::LowStaminaWarning);
    } else if (world_state_.GetLowStaminaWarningActive() && world_state_.GetStamina().Ratio() > GameConstants::Player::LowStaminaRecoverRatio) {
        world_state_.SetLowStaminaWarningActive(false);
        callbacks_.push_hint("体力已经恢复一些了。", GameConstants::Ui::HintDuration::LowStaminaRecovered);
    }

    UpdateCropGrowth(delta_seconds);
    UpdateWorkshop(delta_seconds);
    if (pickup_runtime_) {
        pickup_runtime_->Update(delta_seconds);
        pickup_runtime_->CollectNearby();
    }
    UpdateNpcs(delta_seconds);
    UpdateSpiritBeast(delta_seconds);
    pet_system_.Update(world_state_, delta_seconds);
    UpdateParticles(delta_seconds);
    UpdateHighlightedInteractable();
    UpdateUi(delta_seconds);
}

void GameRuntime::RenderBattle(sf::RenderWindow& window) {
    if (!in_battle_mode_) {
        return;
    }
    sf::RectangleShape bg;
    bg.setPosition({0.0f, 0.0f});
    bg.setSize({1280.0f, 720.0f});
    bg.setFillColor(sf::Color(18, 20, 34));
    window.draw(bg);
    for (const auto& spirit : battle_manager_.GetField().GetSpirits()) {
        if (spirit.is_defeated) {
            continue;
        }
        sf::CircleShape spirit_shape(16.0f);
        spirit_shape.setFillColor(sf::Color(180, 70, 90));
        spirit_shape.setPosition({spirit.pos_x - 16.0f, spirit.pos_y - 16.0f});
        window.draw(spirit_shape);
    }
    battle_manager_.GetUI().Draw(window);
}

bool GameRuntime::HandleBattleKey(int skill_slot) {
    if (!in_battle_mode_) {
        return false;
    }
    return battle_manager_.OnSkillKeyPressed(skill_slot, 0.0f, 0.0f);
}

void GameRuntime::ToggleBattlePause() {
    if (!in_battle_mode_) {
        return;
    }
    if (battle_manager_.IsPaused()) {
        battle_manager_.Resume();
    } else {
        battle_manager_.Pause();
    }
}

void GameRuntime::RetreatBattle() {
    if (!in_battle_mode_) {
        return;
    }
    battle_manager_.Retreat();
}

bool GameRuntime::TryEnterBattleByPlayerPosition() {
    if (in_battle_mode_ || !world_state_.GetInSpiritRealm()) {
        return false;
    }
    const auto player_pos = world_state_.GetPlayer().GetPosition();
    for (const auto& object : world_state_.GetInteractables()) {
        if (object.Label() != "Spirit Beast") {
            continue;
        }
        const auto pos = object.Shape().getPosition();
        if (battle_manager_.ShouldTriggerBattle("spirit_beast", pos.x, pos.y, player_pos.x, player_pos.y)) {
            BattleZone zone;
            zone.id = "spirit_realm_layer2";
            zone.name = "灵界中层";
            zone.is_spirit_realm = true;
            if (battle_manager_.EnterBattle(zone, {"spirit_wisp"})) {
                in_battle_mode_ = true;
                callbacks_.push_hint("遭遇灵界污染体，进入净化战斗。", 2.6f);
                return true;
            }
        }
    }
    return false;
}

void GameRuntime::CycleSpiritBeastName() {
    static const std::array<const char*, 6> kCandidateNames{
        "灵团", "云丸", "小岚", "团团", "阿雾", "辰团"
    };
    auto& beast = world_state_.GetSpiritBeast();
    std::size_t idx = 0;
    for (std::size_t i = 0; i < kCandidateNames.size(); ++i) {
        if (beast.custom_name == kCandidateNames[i]) {
            idx = (i + 1) % kCandidateNames.size();
            break;
        }
    }
    beast.custom_name = kCandidateNames[idx];
    callbacks_.push_hint("已为灵兽命名：" + beast.custom_name, 2.2f);
}

// ============================================================================
// 【GameRuntime::SleepToNextMorning】睡眠到第二天
// ============================================================================
SleepResult GameRuntime::SleepToNextMorning() {
    SleepResult result;

    systems_.GetCloud().UpdateForecastVisibility(
        static_cast<int>(world_state_.GetSessionTime() / 100), 22);

    int daily_influence = 0;
    for (const auto& plot : farming_.Plots()) {
        if (plot.seeded && plot.watered) daily_influence += GameConstants::Cloud::WateredPlotInfluence;
    }
    if (!world_state_.GetMainHouseRepair().completed) daily_influence += GameConstants::Cloud::MainHouseRepairInfluence;
    if (!world_state_.GetSpiritBeast().daily_interacted) daily_influence += GameConstants::Cloud::NoBeastInteractionPenalty;
    systems_.AddPlayerInfluence(daily_influence);
    systems_.CheckContractUnlocks();

    world_state_.GetStamina().Refill();

    TrySpiritBeastWateringAid(
        world_state_.GetSpiritBeast(),
        world_state_.GetSpiritBeastWateredToday(),
        world_state_.GetTeaPlots(),
        world_state_.GetStamina(),
        RefreshTeaPlotVisual,
        [this](const std::string& msg, float dur) { callbacks_.push_hint(msg, dur); },
        CloudSeamanor::infrastructure::Logger::Info);

    result.spirit_gain = systems_.GetCloud().SpiritEnergyGain();
    result.contract_progress = systems_.GetContracts().CompletedVolumeCount();
    result.message = "新的一天开始了。每日行动已刷新，昨天没做完的事情今天也能继续。"
                     "灵气 +" + std::to_string(result.spirit_gain) + "。";
    if (result.contract_progress > 0) {
        result.message += " 契约进度：" + std::to_string(result.contract_progress) + "卷已完成。";
    }

    callbacks_.push_hint(result.message, GameConstants::Ui::HintDuration::Welcome);
    CloudSeamanor::infrastructure::Logger::Info("玩家睡眠到第二天。");

    return result;
}

// ============================================================================
// 【GameRuntime::CheckTutorialHints】教程提示检查
// ============================================================================
void GameRuntime::CheckTutorialHints() {
    if (tutorial_system_) {
        tutorial_system_->Update(0.0f);
        return;
    }

    if (!world_state_.GetTutorial().intro_move_hint_shown
        && world_state_.GetSessionTime() > 2.5f) {
        world_state_.GetTutorial().intro_move_hint_shown = true;
        callbacks_.push_hint(
            "使用 WASD 移动。明亮的描边表示这些对象可以交互；先熟悉山庄，不需要赶时间。",
            GameConstants::Ui::HintDuration::TutorialMove);
    }
    if (!world_state_.GetTutorial().intro_interact_hint_shown
        && world_state_.GetInteraction().highlighted_index >= 0) {
        world_state_.GetTutorial().intro_interact_hint_shown = true;
        callbacks_.push_hint("按 E 与高亮对象交互。采集会消耗体力。", GameConstants::Ui::HintDuration::TutorialInteract);
    }
    if (!world_state_.GetTutorial().intro_crop_hint_shown
        && world_state_.GetInteraction().highlighted_plot_index >= 0) {
        world_state_.GetTutorial().intro_crop_hint_shown = true;
        callbacks_.push_hint(
            "种植流程：翻土 -> 播种 -> 浇水 -> 等待 -> 收获。"
            "今天做不完也没关系，明天还能继续。", GameConstants::Ui::HintDuration::TutorialCrop);
    }
    if (!world_state_.GetTutorial().intro_save_hint_shown
        && world_state_.GetSessionTime() > 18.0f) {
        world_state_.GetTutorial().intro_save_hint_shown = true;
        callbacks_.push_hint(
            "提示：F6 保存，F9 读取；22:00 之后按 T 可以睡觉。"
            "山庄没有硬性期限，错过的内容之后还能再体验。", GameConstants::Ui::HintDuration::TutorialSave);
    }
}

// ============================================================================
// 【GameRuntime::SaveGame】保存游戏
// ============================================================================
bool GameRuntime::SaveGame() {
    return SaveGameToSlot(active_save_slot_);
}

bool GameRuntime::SaveGameToSlot(int slot_index) {
    SetActiveSaveSlot(slot_index);
    auto push_hint = [this](const std::string& msg, float dur) {
        callbacks_.push_hint(msg, dur);
    };
    if (in_battle_mode_) {
        // BE-053: 战斗中存档强制结束战斗，防止读档刷战利品。
        battle_manager_.Retreat();
        in_battle_mode_ = false;
        callbacks_.push_hint("检测到战斗中存档，已自动结算并退出战斗。", 2.6f);
    }
    const int battle_state = static_cast<int>(battle_manager_.CurrentState());
    if (!CloudSeamanor::engine::SaveGameState(
            save_path_,
            world_state_.GetClock(),
            systems_.GetCloud(),
            world_state_.GetPlayer(),
            world_state_.GetStamina(),
            world_state_.GetMainHouseRepair(),
            world_state_.GetTeaMachine(),
            world_state_.GetSpiritBeast(),
            world_state_.GetSpiritBeastWateredToday(),
            world_state_.GetTeaPlots(),
            world_state_.GetGold(),
            world_state_.GetPriceTable(),
            world_state_.GetMailOrders(),
            world_state_.GetInventory(),
            world_state_.GetNpcs(),
            push_hint,
            &systems_.GetSkills(),
            &systems_.GetFestivals(),
            &systems_.GetDynamicLife(),
            &systems_.GetWorkshop(),
            &world_state_.GetDecorationScore(),
            &world_state_.GetPetType(),
            &world_state_.GetPetAdopted(),
            &world_state_.GetAchievements(),
            &world_state_.GetWeeklyBuyCount(),
            &world_state_.GetWeeklySellCount(),
            &world_state_.GetSpiritRealmDailyMax(),
            &world_state_.GetSpiritRealmDailyRemaining(),
            &in_battle_mode_,
            &battle_state)) {
        return false;
    }

    // 追加心事件完成状态
    std::vector<std::string> heart_lines;
    dialogue_manager_.SaveState(heart_lines);
    if (!heart_lines.empty()) {
        std::ofstream out(save_path_, std::ios::app);
        for (const auto& line : heart_lines) {
            out << line << '\n';
        }
    }

    // 追加主线剧情状态
    std::vector<std::string> plot_lines;
    plot_system_.SaveState(plot_lines);
    if (!plot_lines.empty()) {
        std::ofstream out(save_path_, std::ios::app);
        for (const auto& line : plot_lines) {
            out << line << '\n';
        }
    }
    CloudSeamanor::infrastructure::SaveSlotMetadata metadata;
    metadata.slot_index = active_save_slot_;
    metadata.exists = true;
    metadata.saved_at_text = world_state_.GetClock().DateText() + " " + world_state_.GetClock().TimeText();
    metadata.day = world_state_.GetClock().Day();
    metadata.season_text = world_state_.GetClock().PhaseText();
    // 存档缩略图：优先从当前窗口截帧保存为 PNG（失败则留空，UI 使用占位符渲染）。
    metadata.thumbnail_path.clear();
    if (window_ != nullptr) {
        try {
            namespace fs = std::filesystem;
            const fs::path thumb_dir = fs::path("saves") / ("slot_" + std::to_string(active_save_slot_));
            fs::create_directories(thumb_dir);
            const fs::path thumb_path = thumb_dir / "thumbnail.png";

            const auto size = window_->getSize();
            sf::Texture capture;
            if (capture.resize({size.x, size.y})) {
                capture.update(*window_);
                sf::Image img = capture.copyToImage();
                // 生成 160x90 缩略图（最近邻）
                sf::Image small;
                constexpr unsigned int kW = 160;
                constexpr unsigned int kH = 90;
                small.resize({kW, kH}, sf::Color(200, 192, 168));
                for (unsigned int y = 0; y < kH; ++y) {
                    for (unsigned int x = 0; x < kW; ++x) {
                        const unsigned int src_x = static_cast<unsigned int>(
                            (static_cast<std::uint64_t>(x) * size.x) / kW);
                        const unsigned int src_y = static_cast<unsigned int>(
                            (static_cast<std::uint64_t>(y) * size.y) / kH);
                        small.setPixel({x, y}, img.getPixel({std::min(src_x, size.x - 1), std::min(src_y, size.y - 1)}));
                    }
                }
                if (small.saveToFile(thumb_path.string())) {
                    metadata.thumbnail_path = thumb_path.generic_string();
                }
            }
        } catch (...) {
            metadata.thumbnail_path.clear();
        }
    }
    save_slot_manager_.WriteMetadata(active_save_slot_, metadata);
    return true;
}

// ============================================================================
// 【GameRuntime::LoadGame】加载游戏
// ============================================================================
bool GameRuntime::LoadGame() {
    return LoadGameFromSlot(active_save_slot_);
}

bool GameRuntime::LoadGameFromSlot(int slot_index) {
    SetActiveSaveSlot(slot_index);
    auto push_hint = [this](const std::string& msg, float dur) {
        callbacks_.push_hint(msg, dur);
    };
    bool loaded_in_battle_mode = false;
    int loaded_battle_state = static_cast<int>(BattleState::Inactive);
    const bool ok = CloudSeamanor::engine::LoadGameState(
        save_path_,
        world_state_.GetClock(),
        systems_.GetCloud(),
        world_state_.GetPlayer(),
        world_state_.GetStamina(),
        world_state_.GetMainHouseRepair(),
        world_state_.GetTeaMachine(),
        world_state_.GetSpiritBeast(),
        world_state_.GetSpiritBeastWateredToday(),
        world_state_.GetTeaPlots(),
        world_state_.GetGold(),
        world_state_.GetPriceTable(),
        world_state_.GetMailOrders(),
        world_state_.GetInventory(),
        world_state_.GetNpcs(),
        world_state_.GetObstacleShapes(),
        last_cloud_state_,
        [this]() {
            RefreshSpiritBeastVisual(world_state_.GetSpiritBeast(),
                                    world_state_.GetInteraction().spirit_beast_highlighted);
        },
        RefreshTeaPlotVisual,
        [this]() {
            CloudSeamanor::engine::UpdateHighlightedInteractable(
                world_state_.GetPlayer(),
                world_state_.GetInteractables(),
                world_state_.GetTeaPlots(),
                world_state_.GetNpcs(),
                world_state_.GetSpiritBeast(),
                world_state_.GetInteraction().highlighted_index,
                world_state_.GetInteraction().highlighted_plot_index,
                world_state_.GetInteraction().highlighted_npc_index,
                world_state_.GetInteraction().spirit_beast_highlighted,
                RefreshTeaPlotVisual,
                RefreshSpiritBeastVisual);
        },
        [this]() { UpdateUi(0.0f); },
        push_hint,
        &systems_.GetSkills(),
        &systems_.GetFestivals(),
        &systems_.GetDynamicLife(),
        &systems_.GetWorkshop(),
        &dialogue_manager_,
        &world_state_.GetDecorationScore(),
        &world_state_.GetPetType(),
        &world_state_.GetPetAdopted(),
        &world_state_.GetAchievements(),
        &world_state_.GetWeeklyBuyCount(),
        &world_state_.GetWeeklySellCount(),
        &world_state_.GetSpiritRealmDailyMax(),
        &world_state_.GetSpiritRealmDailyRemaining(),
        &loaded_in_battle_mode,
        &loaded_battle_state);
    // 加载主线剧情状态（从存档文件末尾读取 plot 行）
    {
        std::ifstream in(save_path_);
        if (in.is_open()) {
            std::string line;
            std::vector<std::string> plot_lines;
            while (std::getline(in, line)) {
                if (line.rfind("main_plot|", 0) == 0) {
                    plot_lines.push_back(line);
                }
            }
            if (!plot_lines.empty()) {
                plot_system_.LoadState(plot_lines);
            }
        }
    }
    // 重新初始化剧情系统引用（读档后需要刷新）
    plot_system_.SetGameClock(&world_state_.GetClock());
    plot_system_.SetCloudSystem(&systems_.GetCloud());
    plot_system_.SetNpcHeartGetter([this](const std::string& npc_id) -> int {
        for (const auto& npc : world_state_.GetNpcs()) {
            if (npc.id == npc_id) {
                return npc.heart_level;
            }
        }
        return 0;
    });
    SyncTeaMachineFromWorkshop_();
    world_state_.GetGreenhouseUnlocked() = (world_state_.GetMainHouseRepair().level >= 3);
    if (!world_state_.GetGreenhouseUnlocked()) {
        world_state_.GetGreenhouseTagNextPlanting() = false;
    }
    if (loaded_in_battle_mode || loaded_battle_state != static_cast<int>(BattleState::Inactive)) {
        battle_manager_.Retreat();
        in_battle_mode_ = false;
        callbacks_.push_hint("读档时检测到战斗态，已安全重置战斗状态。", 2.6f);
    }
    return ok;
}

void GameRuntime::SetActiveSaveSlot(int slot_index) {
    if (!save_slot_manager_.IsValidSlot(slot_index)) {
        return;
    }
    active_save_slot_ = slot_index;
    save_path_ = save_slot_manager_.BuildSlotPath(slot_index);
}

std::vector<CloudSeamanor::infrastructure::SaveSlotMetadata> GameRuntime::ReadSaveSlots() const {
    return save_slot_manager_.ReadAllMetadata();
}

// ============================================================================
// 【GameRuntime::ConsumeLevelUpEvent】消费升级事件
// ============================================================================
LevelUpEvent GameRuntime::ConsumeLevelUpEvent() {
    LevelUpEvent event;
    if (world_state_.GetLevelUpOverlayActive()) {
        event.triggered = true;
        event.skill_type = world_state_.GetLevelUpSkillType();
        event.skill_name = systems_.GetSkills().GetSkillName(event.skill_type);
        event.new_level = systems_.GetSkills().GetLevel(event.skill_type);
    }
    return event;
}

// ============================================================================
// 【GameRuntime::CanSleep】是否可以睡觉
// ============================================================================
bool GameRuntime::CanSleep() const {
    return world_state_.GetClock().Hour() >= 22
        || world_state_.GetClock().Hour() < 6;
}

// ============================================================================
// 【GameRuntime::GetCurrentTargetText】获取当前目标文本
// ============================================================================
std::string GameRuntime::GetCurrentTargetText() const {
    TargetHintContext ctx{
        world_state_.GetInventory(),
        systems_.GetWorkshop(),
        world_state_.GetTeaPlots(),
        world_state_.GetNpcs(),
        world_state_.GetSpiritBeast(),
        world_state_.GetInteractables(),
        world_state_.GetSpiritBeastWateredToday(),
        world_state_.GetInteraction().highlighted_plot_index,
        world_state_.GetInteraction().highlighted_npc_index,
        world_state_.GetInteraction().spirit_beast_highlighted,
        world_state_.GetInteraction().highlighted_index,
        world_state_.GetMainHouseRepair(),
        world_state_.GetTeaMachine()
    };
    return BuildCurrentTargetText(ctx);
}

// ============================================================================
// 【GameRuntime::GetControlsHint】获取控制提示
// ============================================================================
std::string GameRuntime::GetControlsHint() const {
    return "WASD 移动  E 交互  I 背包  F 任务  M 地图  C 状态  F5 预报  F6 保存  F9 读取  F7/CTRL+W 工坊  F8 节日  O 商店  P 邮件  L 成就  V 图鉴";
}

std::size_t GameRuntime::GetEntityCount() const {
    return world_state_.GetTeaPlots().size()
        + world_state_.GetNpcs().size()
        + world_state_.GetInteractables().size();
}

std::size_t GameRuntime::GetPickupCount() const {
    return world_state_.GetPickups().size();
}

// ============================================================================
// 【GameRuntime::HandleGiftInteraction】处理 NPC 送礼
// ============================================================================
void GameRuntime::HandleGiftInteraction() {
    HandleInteractionCommon_(true);
}

// ============================================================================
// 【GameRuntime::HandlePrimaryInteraction】处理主交互
// ============================================================================
void GameRuntime::HandlePrimaryInteraction() {
    HandleInteractionCommon_(false);
}

void GameRuntime::RenderSceneTransition(sf::RenderWindow& window) {
    scene_transition_.Render(window);
}

PlayerInteractRuntimeContext GameRuntime::BuildInteractionContext_() {
    auto spawn_hearts = [](const sf::Vector2f& pos,
                            std::vector<HeartParticle>& particles) {
        CloudSeamanor::engine::SpawnHeartParticles(pos, particles);
    };
    auto refresh_pickup = [](CloudSeamanor::domain::PickupDrop& pickup) {
        auto& shape = pickup.Shape();
        shape.setFillColor(PickupColorFor(pickup.ItemId()));
        shape.setOutlineThickness(2.0f);
        shape.setOutlineColor(sf::Color(72, 48, 24));
    };

    PlayerInteractRuntimeContext ctx(
        world_state_.GetClock(),
        systems_.GetCloud(),
        world_state_.GetInventory(),
        world_state_.GetStamina(),
        world_state_.GetConfig().stamina_interact_cost,
        world_state_.GetTeaPlots(),
        world_state_.GetNpcs(),
        world_state_.GetSpiritBeast(),
        world_state_.GetSpiritBeastWateredToday(),
        world_state_.GetHeartParticles(),
        world_state_.GetPickups(),
        world_state_.GetInteractables(),
        world_state_.GetMainHouseRepair(),
        world_state_.GetTeaMachine(),
        world_state_.GetObstacleShapes(),
        systems_.GetSkills(),
        systems_.GetDynamicLife(),
        systems_.GetWorkshop(),
        world_state_.GetNpcTextMappings(),
        world_state_.GetInteraction().dialogue_engine,
        dialogue_data_root_,
        world_state_.GetInteraction().dialogue_text,
        world_state_.GetGold(),
        world_state_.GetPriceTable(),
        world_state_.GetMailOrders(),
        world_state_.GetLastTradeQuality(),
        world_state_.GetInSpiritRealm(),
        world_state_.GetSpiritPlantLastHarvestHour(),
        world_state_.GetWeeklyBuyCount(),
        world_state_.GetWeeklySellCount(),
        world_state_.GetDailyGeneralStoreStock(),
        world_state_.GetInnGoldReserve(),
        world_state_.GetCoopFedToday(),
        world_state_.GetDecorationScore(),
        world_state_.GetPetType(),
        world_state_.GetPetAdopted(),
        world_state_.GetAchievements(),
        world_state_.GetModHooks(),
        world_state_.GetGreenhouseUnlocked(),
        world_state_.GetGreenhouseTagNextPlanting(),
        world_state_.GetClock().Hour(),
        [this](const std::string& npc_id) -> bool {
            return npc_delivery_.TryClaimRewards(world_state_, npc_id);
        },
        [this](bool to_spirit_realm) {
            RequestSpiritRealmTravel_(to_spirit_realm);
        },
        [this](const std::string& msg, float dur) { callbacks_.push_hint(msg, dur); },
        &CloudSeamanor::infrastructure::Logger::Info,
        [this](const std::string& id) {
            if (callbacks_.play_sfx) callbacks_.play_sfx(id);
        },
        [this]() { UpdateUi(0.0f); },
        [this]() { UpdateUi(0.0f); },
        [this]() { RefreshWindowTitle(); },
        [this](CloudSeamanor::domain::SkillType type, [[maybe_unused]] int new_level) {
            world_state_.SetLevelUpOverlayActive(true);
            world_state_.SetLevelUpOverlayTimer(GameConstants::Ui::LevelUp::OverlayDuration);
            world_state_.SetLevelUpSkillType(type);
        },
        RefreshTeaPlotVisual,
        spawn_hearts,
        refresh_pickup,
        world_state_.GetInteraction(),
        world_state_.GetInteraction().highlighted_npc_index,
        world_state_.GetInteraction().highlighted_plot_index,
        world_state_.GetInteraction().highlighted_index,
        world_state_.GetClock().Day(),
        world_state_.GetClock().Hour(),
        world_state_.GetInteraction().spirit_beast_highlighted);

    ctx.dialogue_manager = &dialogue_manager_;
    ctx.dialogue_nodes = &world_state_.GetInteraction().dialogue_nodes;
    ctx.dialogue_start_id = &world_state_.GetInteraction().dialogue_start_id;
    ctx.player_name = "云海旅人";
    ctx.farm_name = "云海山庄";
    ctx.current_heart_event_id = world_state_.GetInteraction().current_heart_event_id;
    ctx.current_heart_event_reward = world_state_.GetInteraction().current_heart_event_reward;
    ctx.current_heart_event_flag = world_state_.GetInteraction().current_heart_event_flag;
    return ctx;
}

void GameRuntime::RequestSpiritRealmTravel_(bool to_spirit_realm) {
    if (scene_transition_.IsActive()) {
        return;
    }

    const bool currently_in_spirit = world_state_.GetInSpiritRealm();
    if (currently_in_spirit == to_spirit_realm) {
        return;
    }

    if (to_spirit_realm && world_state_.GetSpiritRealmDailyRemaining() <= 0) {
        if (callbacks_.push_hint) {
            callbacks_.push_hint("今日灵界探索次数已用尽。", 2.2f);
        }
        return;
    }

    const std::string requested_map = to_spirit_realm
        ? "assets/maps/spirit_realm_layer1.tmx"
        : "assets/maps/prototype_farm.tmx";
    const std::string to_map = ResolveMapFromMods_(mod_loader_, requested_map);

    if (callbacks_.push_hint) {
        callbacks_.push_hint(to_spirit_realm ? "传送门开启……" : "正在返回主世界……", 1.8f);
    }

    scene_transition_.Start(
        0.18f,
        0.18f,
        [this, to_spirit_realm, to_map]() {
            world_state_.GetInSpiritRealm() = to_spirit_realm;
            if (to_spirit_realm) {
                world_state_.SetSpiritRealmDailyRemaining(world_state_.GetSpiritRealmDailyRemaining() - 1);
            }
            BuildScene(
                tmx_map_,
                world_state_.GetGroundTiles(),
                world_state_.GetObstacleShapes(),
                world_state_.GetObstacleBounds(),
                world_state_.GetInteractables(),
                world_state_.GetPickups(),
                world_state_.GetConfig().world_bounds,
                to_map,
                &CloudSeamanor::infrastructure::Logger::Info,
                CloudSeamanor::infrastructure::Logger::Warning);
        });
}

void GameRuntime::HandleInteractionCommon_(bool is_gift_interaction) {
    auto ctx = BuildInteractionContext_();
    if (is_gift_interaction) {
        CloudSeamanor::engine::HandleGiftInteraction(ctx);
    } else {
        CloudSeamanor::engine::HandlePrimaryInteraction(ctx);
    }
    Event ev;
    ev.type = is_gift_interaction ? "OnGiftGiven" : "OnInteract";
    ev.data["day"] = std::to_string(world_state_.GetClock().Day());
    GlobalEventBus().Emit(ev);
}

// ============================================================================
// 【GameRuntime::RefreshWindowTitle】刷新窗口标题
// ============================================================================
void GameRuntime::RefreshWindowTitle() {
    if (!window_) return;

    CloudSeamanor::engine::RefreshWindowTitle(
        *window_,
        world_state_.GetClock(),
        systems_.GetCloud(),
        world_state_.GetPlayer(),
        world_state_.GetStamina(),
        world_state_.GetMainHouseRepair(),
        world_state_.GetTeaMachine(),
        world_state_.GetSpiritBeast(),
        world_state_.GetNpcs(),
        world_state_.GetPickups(),
        world_state_.GetInteraction().highlighted_index,
        GetCurrentTargetText(),
        CanSleep(),
        &systems_.GetSkills(),
        &systems_.GetFestivals());
}

// ============================================================================
// 【GameRuntime::UpdateCropGrowth】更新作物生长
// ============================================================================
void GameRuntime::UpdateCropGrowth(float delta_seconds) {
    const float cloud_multiplier = CloudMultiplier();
    farming_.UpdateGrowth(delta_seconds, cloud_multiplier);
    crop_growth_.Update(world_state_, delta_seconds);
    tea_garden_.UpdatePlots(
        world_state_.GetTeaGardenPlots(),
        delta_seconds * cloud_multiplier,
        systems_.GetCloud().CurrentState(),
        world_state_.GetClock().Day());
}

// ============================================================================
// 【GameRuntime::UpdateWorkshop】更新工坊
// ============================================================================
void GameRuntime::UpdateWorkshop(float delta_seconds) {
    if (workshop_runtime_) {
        workshop_runtime_->Update(delta_seconds);
    }
    SyncTeaMachineFromWorkshop_();
}

// ============================================================================
// 【GameRuntime::UpdateNpcs】更新 NPC
// ============================================================================
void GameRuntime::UpdateNpcs(float delta_seconds) {
    npc_schedule_.Update(world_state_, delta_seconds);
    // B-6 NPC 互相互动：距离很近时共享活动标签。
    auto& npcs = world_state_.GetNpcs();
    for (std::size_t i = 0; i < npcs.size(); ++i) {
        for (std::size_t j = i + 1; j < npcs.size(); ++j) {
            const sf::Vector2f d = npcs[i].shape.getPosition() - npcs[j].shape.getPosition();
            if ((d.x * d.x + d.y * d.y) < 900.0f) {
                npcs[i].current_activity = "聊天中";
                npcs[j].current_activity = "聊天中";
            }
        }
    }
}

// ============================================================================
// 【GameRuntime::UpdateSpiritBeast】更新灵兽
// ============================================================================
void GameRuntime::UpdateSpiritBeast(float delta_seconds) {
    auto& beast = world_state_.GetSpiritBeast();

    if (beast.state == SpiritBeastState::Interact) {
        beast.interact_timer -= delta_seconds;
        if (beast.interact_timer <= 0.0f) {
            beast.state = SpiritBeastState::Follow;
        }
    } else {
        const auto player_pos_d = world_state_.GetPlayer().GetPosition();
        const sf::Vector2f player_pos(player_pos_d.x, player_pos_d.y);
        sf::Vector2f beast_pos = beast.shape.getPosition();
        const sf::Vector2f to_player = player_pos - beast_pos;
        const float distance = std::sqrt(to_player.x * to_player.x + to_player.y * to_player.y);

        if (distance < GameConstants::SpiritBeast::FollowStartDistance) {
            beast.state = SpiritBeastState::Follow;
        } else if (distance > GameConstants::SpiritBeast::FollowStopDistance) {
            beast.state = SpiritBeastState::Wander;
        }
    }

    spirit_beast_.SetState(beast.state);
    spirit_beast_.Update(world_state_, delta_seconds);

    beast.state = spirit_beast_.CurrentState();
    RefreshSpiritBeastVisual(beast, world_state_.GetInteraction().spirit_beast_highlighted);
}

// ============================================================================
// 【GameRuntime::UpdateParticles】更新粒子
// ============================================================================
void GameRuntime::UpdateParticles(float delta_seconds) {
    auto& particles = world_state_.GetHeartParticles();
    while (particles.size() > static_cast<std::size_t>(GameConstants::SpiritBeast::MaxParticles)) {
        particles.pop_back();
    }

    for (std::size_t i = 0; i < particles.size();) {
        auto& p = particles[i];
        p.lifetime -= delta_seconds;
        if (p.lifetime <= 0.0f) {
            particles[i] = particles.back();
            particles.pop_back();
            continue;
        }
        p.velocity.y += GameConstants::SpiritBeast::ParticleGravity * delta_seconds;
        p.shape.move(p.velocity * delta_seconds);

        const float alpha_ratio = std::max(0.0f, p.lifetime / GameConstants::SpiritBeast::ParticleLifetime);
        sf::Color color = p.shape.getFillColor();
        color.a = static_cast<std::uint8_t>(220.0f * alpha_ratio);
        p.shape.setFillColor(color);
        ++i;
    }
}

// ============================================================================
// 【GameRuntime::SyncTeaMachineFromWorkshop_】同步制茶机显示状态
// ============================================================================
void GameRuntime::SyncTeaMachineFromWorkshop_() {
    auto& tea_machine = world_state_.GetTeaMachine();

    if (const auto* machine = systems_.GetWorkshop().GetMachine("tea_machine")) {
        tea_machine.running = machine->is_processing;
        tea_machine.progress = machine->is_processing ? machine->progress : 0.0f;
        tea_machine.duration = 100.0f;
        return;
    }

    tea_machine.running = false;
    tea_machine.progress = 0.0f;
    tea_machine.duration = 100.0f;
}

// ============================================================================
// 【GameRuntime::UpdateHighlightedInteractable】更新高亮交互
// ============================================================================
void GameRuntime::UpdateHighlightedInteractable() {
    CloudSeamanor::engine::UpdateHighlightedInteractable(
        world_state_.GetPlayer(),
        world_state_.GetInteractables(),
        world_state_.GetTeaPlots(),
        world_state_.GetNpcs(),
        world_state_.GetSpiritBeast(),
        world_state_.GetInteraction().highlighted_index,
        world_state_.GetInteraction().highlighted_plot_index,
        world_state_.GetInteraction().highlighted_npc_index,
        world_state_.GetInteraction().spirit_beast_highlighted,
        RefreshTeaPlotVisual,
        RefreshSpiritBeastVisual);
}

// ============================================================================
// 【GameRuntime::UpdateUi】更新 UI
// ============================================================================
void GameRuntime::UpdateUi(float delta_seconds) {
    UpdateStaminaBar(world_state_);
    if (workshop_runtime_) {
        workshop_runtime_->UpdateProgressBar();
    } else {
        UpdateWorkshopProgressBar(world_state_, systems_.GetWorkshop());
    }
    UpdateWorldTipPulse(world_state_, delta_seconds);
    if (callbacks_.update_hud_text) {
        callbacks_.update_hud_text();
    }
}

}  // namespace CloudSeamanor::engine
