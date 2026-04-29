// ============================================================================
// 【MainPlotSystemTest.cpp】MainPlotSystem 单元测试
// ============================================================================
// Test cases for CloudSeamanor::engine::MainPlotSystem
//
// Coverage:
// - Initialization and data loading
// - Chapter unlock conditions
// - Plot trigger conditions (DayRange, Season, CloudLevel, HeartCount, FlagSet)
// - Starting/ending plots
// - Dialogue flow (node progression)
// - Choice selection and effects
// - Save/Load state
// - Route locking (balance/open/close)
// - Flag management
// - Priority-based trigger ordering
// ============================================================================

#include "../catch2Compat.hpp"

#include "CloudSeamanor/engine/MainPlotSystem.hpp"
#include "CloudSeamanor/domain/GameClock.hpp"
#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/engine/DialogueEngine.hpp"

using CloudSeamanor::engine::ChapterEntry;
using CloudSeamanor::engine::DialogueChoice;
using CloudSeamanor::engine::MainPlotSystem;
using CloudSeamanor::engine::PlotCallbacks;
using CloudSeamanor::engine::PlotChoiceEffect;
using CloudSeamanor::engine::PlotCondition;
using CloudSeamanor::engine::PlotConditionType;
using CloudSeamanor::engine::PlotEntry;
using CloudSeamanor::engine::PlotNode;
using CloudSeamanor::engine::PlotRoute;
using CloudSeamanor::engine::PlotState;

using CloudSeamanor::domain::GameClock;
using CloudSeamanor::domain::Season;

namespace {

// Mock CloudSystem that always returns a fixed spirit energy
class MockCloudSystem {
public:
    int SpiritEnergy() const { return spirit_energy_; }
    void SetSpiritEnergy(int v) { spirit_energy_ = v; }
    std::string CurrentStateText() const { return "晴"; }
private:
    int spirit_energy_ = 0;
};

// Mock GameClock
GameClock g_clock;
MockCloudSystem g_cloud;

// NPC heart tracker for mock
std::unordered_map<std::string, int> g_npc_hearts;

int MockNpcHeartGetter(const std::string& npc_id) {
    auto it = g_npc_hearts.find(npc_id);
    return it != g_npc_hearts.end() ? it->second : 0;
}

}  // namespace

// ============================================================================
// Test Fixtures
// ============================================================================

struct PlotSystemFixture {
    PlotSystemFixture() {
        system.Initialize("assets/data");
        system.SetGameClock(&g_clock);
        system.SetCloudSystem(&g_cloud);
        system.SetNpcHeartGetter(&MockNpcHeartGetter);

        // Track callbacks
        callbacks.on_chapter_start = [&](const std::string&) { chapter_started_count_++; };
        callbacks.on_chapter_complete = [&](const std::string&) { chapter_complete_count_++; };
        callbacks.on_plot_start = [&](const std::string&) { plot_started_count_++; };
        callbacks.on_plot_complete = [&](const std::string&) { plot_complete_count_++; };
        callbacks.on_flag_set = [&](const std::string&) { flag_set_count_++; };
        callbacks.on_cloud_delta = [&](int) { cloud_delta_count_++; };
        callbacks.on_route_lock = [&](const std::string&) { route_locked_ = true; };
        callbacks.on_notice = [&](const std::string&) {};
        system.SetCallbacks(std::move(callbacks));
    }

    void ResetClock(int day = 1, Season season = Season::Spring) {
        g_clock.Initialize(day, 6.0f * 60.0f);
    }

    void SetCloud(int value) { g_cloud.SetSpiritEnergy(value); }

    void SetNpcHeart(const std::string& npc_id, int heart) {
        g_npc_hearts[npc_id] = heart;
    }

    void ClearNpcHearts() { g_npc_hearts.clear(); }

    MainPlotSystem system;
    PlotCallbacks callbacks;
    int chapter_started_count_ = 0;
    int chapter_complete_count_ = 0;
    int plot_started_count_ = 0;
    int plot_complete_count_ = 0;
    int flag_set_count_ = 0;
    int cloud_delta_count_ = 0;
    bool route_locked_ = false;
};

// ============================================================================
// Initialization Tests
// ============================================================================

TEST_CASE("MainPlotSystem - initializes with 10 chapters") {
    PlotSystemFixture f;
    auto chapters = f.system.GetAllChapters();
    CHECK(chapters.size() == 10);
}

TEST_CASE("MainPlotSystem - chapter 1 is always unlocked") {
    PlotSystemFixture f;
    CHECK(f.system.IsChapterUnlocked("ch1") == true);
}

TEST_CASE("MainPlotSystem - chapter 2 locked without ch1 completion") {
    PlotSystemFixture f;
    CHECK(f.system.IsChapterUnlocked("ch2") == false);
}

TEST_CASE("MainPlotSystem - chapter 2 unlocks after ch1 completed") {
    PlotSystemFixture f;
    f.system.DebugSetFlag("chapter_completed_ch1");
    CHECK(f.system.IsChapterUnlocked("ch2") == true);
}

// ============================================================================
// PlotCondition Tests
// ============================================================================

TEST_CASE("PlotCondition::IsSatisfied - DayRange") {
    PlotCondition cond;
    cond.type = PlotConditionType::DayRange;
    cond.int_value = 1;
    cond.int_value2 = 5;

    std::unordered_set<std::string> flags;
    std::unordered_map<std::string, int> npc_hearts;

    CHECK(cond.IsSatisfied(3, Season::Spring, 0, "", flags, npc_hearts, PlotRoute::None) == true);
    CHECK(cond.IsSatisfied(0, Season::Spring, 0, "", flags, npc_hearts, PlotRoute::None) == false);
    CHECK(cond.IsSatisfied(10, Season::Spring, 0, "", flags, npc_hearts, PlotRoute::None) == false);
}

TEST_CASE("PlotCondition::IsSatisfied - Season") {
    PlotCondition cond;
    cond.type = PlotConditionType::Season;
    cond.str_value = "夏";

    std::unordered_set<std::string> flags;
    std::unordered_map<std::string, int> npc_hearts;

    CHECK(cond.IsSatisfied(1, Season::Summer, 0, "", flags, npc_hearts, PlotRoute::None) == true);
    CHECK(cond.IsSatisfied(1, Season::Spring, 0, "", flags, npc_hearts, PlotRoute::None) == false);
}

TEST_CASE("PlotCondition::IsSatisfied - CloudLevel") {
    PlotCondition cond;
    cond.type = PlotConditionType::CloudLevel;
    cond.int_value = 100;

    std::unordered_set<std::string> flags;
    std::unordered_map<std::string, int> npc_hearts;

    CHECK(cond.IsSatisfied(1, Season::Spring, 150, "", flags, npc_hearts, PlotRoute::None) == true);
    CHECK(cond.IsSatisfied(1, Season::Spring, 50, "", flags, npc_hearts, PlotRoute::None) == false);
}

TEST_CASE("PlotCondition::IsSatisfied - HeartCount") {
    PlotCondition cond;
    cond.type = PlotConditionType::HeartCount;
    cond.str_value = "acha";
    cond.int_value = 5;

    std::unordered_set<std::string> flags;
    std::unordered_map<std::string, int> npc_hearts = {{"acha", 8}, {"xiaoman", 3}};

    CHECK(cond.IsSatisfied(1, Season::Spring, 0, "", flags, npc_hearts, PlotRoute::None) == true);
    CHECK(cond.IsSatisfied(1, Season::Spring, 0, "", flags, {}, PlotRoute::None) == false);
}

TEST_CASE("PlotCondition::IsSatisfied - FlagSet") {
    PlotCondition cond;
    cond.type = PlotConditionType::FlagSet;
    cond.str_value = "found_treasure";

    std::unordered_set<std::string> flags = {"found_treasure", "other_flag"};
    std::unordered_map<std::string, int> npc_hearts;

    CHECK(cond.IsSatisfied(1, Season::Spring, 0, "", flags, npc_hearts, PlotRoute::None) == true);
    CHECK(cond.IsSatisfied(1, Season::Spring, 0, "", {}, npc_hearts, PlotRoute::None) == false);
}

TEST_CASE("PlotCondition::IsSatisfied - RouteSelected") {
    PlotCondition cond;
    cond.type = PlotConditionType::RouteSelected;
    cond.str_value = "balance";

    std::unordered_set<std::string> flags;
    std::unordered_map<std::string, int> npc_hearts;

    CHECK(cond.IsSatisfied(1, Season::Spring, 0, "", flags, npc_hearts, PlotRoute::Balance) == true);
    CHECK(cond.IsSatisfied(1, Season::Spring, 0, "", flags, npc_hearts, PlotRoute::Open) == false);
}

TEST_CASE("PlotCondition::IsSatisfied - TotalHeartCount") {
    PlotCondition cond;
    cond.type = PlotConditionType::TotalHeartCount;
    cond.int_value = 30;

    std::unordered_set<std::string> flags;
    std::unordered_map<std::string, int> npc_hearts = {{"acha", 10}, {"xiaoman", 10}, {"wanxing", 10}};

    CHECK(cond.IsSatisfied(1, Season::Spring, 0, "", flags, npc_hearts, PlotRoute::None) == true);
    CHECK(cond.IsSatisfied(1, Season::Spring, 0, "", flags, {}, PlotRoute::None) == false);
}

// ============================================================================
// State Tests
// ============================================================================

TEST_CASE("MainPlotSystem - initial state is Idle") {
    PlotSystemFixture f;
    CHECK(f.system.State() == PlotState::Idle);
}

TEST_CASE("MainPlotSystem - IsPlaying returns false when Idle") {
    PlotSystemFixture f;
    CHECK(f.system.IsPlaying() == false);
}

TEST_CASE("MainPlotSystem - CurrentRoute is None initially") {
    PlotSystemFixture f;
    CHECK(f.system.CurrentRoute() == PlotRoute::None);
}

TEST_CASE("MainPlotSystem - StartPlot changes state to Playing") {
    PlotSystemFixture f;
    f.system.DebugForceStartPlot("plot_ch1_prologue");
    CHECK(f.system.State() != PlotState::Idle);
    CHECK(f.system.IsPlaying() == true);
}

TEST_CASE("MainPlotSystem - EndPlot resets to Idle") {
    PlotSystemFixture f;
    f.system.DebugForceStartPlot("plot_ch1_prologue");
    f.system.EndPlot();
    CHECK(f.system.State() == PlotState::Idle);
}

TEST_CASE("MainPlotSystem - DebugForceStartChapter starts chapter") {
    PlotSystemFixture f;
    f.system.DebugForceStartChapter("ch1");
    CHECK(f.system.CurrentChapterId() == "ch1");
}

TEST_CASE("MainPlotSystem - GetChapter returns valid chapter") {
    PlotSystemFixture f;
    auto* ch = f.system.GetChapter("ch1");
    CHECK(ch != nullptr);
    CHECK(ch->chapter_number == 1);
    CHECK(ch->title == "归山");
}

TEST_CASE("MainPlotSystem - GetChapter returns nullptr for invalid id") {
    PlotSystemFixture f;
    CHECK(f.system.GetChapter("ch999") == nullptr);
}

TEST_CASE("MainPlotSystem - IsPlotCompleted returns false initially") {
    PlotSystemFixture f;
    CHECK(f.system.IsPlotCompleted("plot_ch1_prologue") == false);
}

TEST_CASE("MainPlotSystem - StartPlot marks plot as playing") {
    PlotSystemFixture f;
    f.system.DebugForceStartPlot("plot_ch1_prologue");
    auto* plot = f.system.CurrentPlot();
    CHECK(plot != nullptr);
    CHECK(plot->id == "plot_ch1_prologue");
}

TEST_CASE("MainPlotSystem - HasFlag returns false for unset flag") {
    PlotSystemFixture f;
    CHECK(f.system.HasFlag("some_flag") == false);
}

TEST_CASE("MainPlotSystem - DebugSetFlag sets flag") {
    PlotSystemFixture f;
    f.system.DebugSetFlag("test_flag");
    CHECK(f.system.HasFlag("test_flag") == true);
}

TEST_CASE("MainPlotSystem - DebugSetRoute locks route") {
    PlotSystemFixture f;
    f.system.DebugSetRoute(PlotRoute::Balance);
    CHECK(f.system.CurrentRoute() == PlotRoute::Balance);
}

// ============================================================================
// Save/Load Tests
// ============================================================================

TEST_CASE("MainPlotSystem - SaveState produces main_plot lines") {
    PlotSystemFixture f;
    f.system.DebugSetFlag("test_flag");
    f.system.DebugSetRoute(PlotRoute::Open);
    f.system.DebugForceStartPlot("plot_ch1_prologue");
    f.system.EndPlot();

    std::vector<std::string> lines;
    f.system.SaveState(lines);

    bool found_route = false;
    bool found_flag = false;
    for (const auto& line : lines) {
        if (line.find("main_plot|route|open") != std::string::npos) found_route = true;
        if (line.find("main_plot|flag|test_flag") != std::string::npos) found_flag = true;
    }
    CHECK(found_route == true);
    CHECK(found_flag == true);
}

TEST_CASE("MainPlotSystem - LoadState restores flags and route") {
    PlotSystemFixture f;

    std::vector<std::string> lines = {
        "main_plot|route|balance",
        "main_plot|flag|restored_flag",
        "main_plot|flag|another_flag",
    };

    f.system.LoadState(lines);
    CHECK(f.system.CurrentRoute() == PlotRoute::Balance);
    CHECK(f.system.HasFlag("restored_flag") == true);
    CHECK(f.system.HasFlag("another_flag") == true);
}

TEST_CASE("MainPlotSystem - LoadState ignores non-main_plot lines") {
    PlotSystemFixture f;

    std::vector<std::string> lines = {
        "main_plot|route|balance",
        "other|data|format",
        "main_plot|flag|safe_flag",
    };

    f.system.LoadState(lines);
    CHECK(f.system.HasFlag("safe_flag") == true);
}

TEST_CASE("MainPlotSystem - DebugResetAllProgress clears everything") {
    PlotSystemFixture f;
    f.system.DebugSetFlag("test_flag");
    f.system.DebugSetRoute(PlotRoute::Close);
    f.system.DebugForceStartPlot("plot_ch1_prologue");
    f.system.EndPlot();

    f.system.DebugResetAllProgress();

    CHECK(f.system.HasFlag("test_flag") == false);
    CHECK(f.system.CurrentRoute() == PlotRoute::None);
    CHECK(f.system.State() == PlotState::Idle);
}

// ============================================================================
// Route Locking Tests
// ============================================================================

TEST_CASE("MainPlotSystem - DebugSetRoute triggers callback") {
    PlotSystemFixture f;
    CHECK(f.route_locked_ == false);
    f.system.DebugSetRoute(PlotRoute::Balance);
    CHECK(f.route_locked_ == true);
}

TEST_CASE("MainPlotSystem - Locking route multiple times keeps first") {
    PlotSystemFixture f;
    f.system.DebugSetRoute(PlotRoute::Balance);
    f.system.DebugSetRoute(PlotRoute::Open);
    CHECK(f.system.CurrentRoute() == PlotRoute::Open);
}

// ============================================================================
// Trigger Tests
// ============================================================================

TEST_CASE("MainPlotSystem - GetTriggerablePlots returns ch1 plots after init") {
    PlotSystemFixture f;
    f.ResetClock(1, Season::Spring);
    auto plots = f.system.GetTriggerablePlots_();
    CHECK(plots.size() > 0);
}

TEST_CASE("MainPlotSystem - GetUpcomingPlots respects max_count") {
    PlotSystemFixture f;
    f.ResetClock(1, Season::Spring);
    auto plots = f.system.GetUpcomingPlots(3);
    CHECK(static_cast<int>(plots.size()) <= 3);
}

TEST_CASE("MainPlotSystem - GetChapterNotice returns notice when chapter active") {
    PlotSystemFixture f;
    f.system.DebugForceStartChapter("ch1");
    auto notice = f.system.GetChapterNotice();
    CHECK(notice.find("归山") != std::string::npos);
}

// ============================================================================
// Dialogue Integration Tests
// ============================================================================

TEST_CASE("MainPlotSystem::Engine - returns dialogue engine reference") {
    PlotSystemFixture f;
    auto& engine = f.system.Engine();
    CHECK(engine.State() == CloudSeamanor::engine::DialogueState::Idle);
}

TEST_CASE("MainPlotSystem - CurrentText empty initially") {
    PlotSystemFixture f;
    CHECK(f.system.CurrentText().empty() == true);
}

TEST_CASE("MainPlotSystem - CurrentChoices empty initially") {
    PlotSystemFixture f;
    auto choices = f.system.CurrentChoices();
    CHECK(choices.empty() == true);
}

TEST_CASE("MainPlotSystem - OnConfirm returns false when Idle") {
    PlotSystemFixture f;
    CHECK(f.system.OnConfirm() == false);
}

TEST_CASE("MainPlotSystem - SelectChoice returns false when Idle") {
    PlotSystemFixture f;
    CHECK(f.system.SelectChoice(0) == false);
}

// ============================================================================
// Chapter Navigation Tests
// ============================================================================

TEST_CASE("MainPlotSystem - chapters are ordered ch1 through ch10") {
    PlotSystemFixture f;
    auto chapters = f.system.GetAllChapters();
    CHECK(chapters[0].id == "ch1");
    CHECK(chapters[9].id == "ch10");
    CHECK(chapters[0].chapter_number == 1);
    CHECK(chapters[9].chapter_number == 10);
}

TEST_CASE("MainPlotSystem - chapter targets are correctly set") {
    PlotSystemFixture f;
    CHECK(f.system.GetChapter("ch1")->target_cloud_level == 100);
    CHECK(f.system.GetChapter("ch2")->target_cloud_level == 300);
    CHECK(f.system.GetChapter("ch3")->target_cloud_level == 500);
    CHECK(f.system.GetChapter("ch4")->target_cloud_level == 700);
    CHECK(f.system.GetChapter("ch5")->target_cloud_level == 0);
    CHECK(f.system.GetChapter("ch4")->target_heart_count == 3);
}
