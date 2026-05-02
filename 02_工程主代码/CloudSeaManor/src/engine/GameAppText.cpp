#include "CloudSeamanor/app/GameAppText.hpp"

#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/domain/CropData.hpp"
#include "CloudSeamanor/domain/GameClock.hpp"
#include "CloudSeamanor/engine/InputManager.hpp"
#include "CloudSeamanor/domain/Inventory.hpp"
#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/domain/RecipeData.hpp"

#include <algorithm>
#include <cstdint>

namespace CloudSeamanor::engine {

namespace {

std::string MainHouseLevelText_(int level) {
    switch (level) {
    case 1: return "帐篷";
    case 2: return "小木屋";
    case 3: return "砖房";
    case 4: return "大宅";
    default: return "未知";
    }
}

}  // namespace

namespace {

// ============================================================================
// 【TimeSliceLabel】获取当前时段文本
// ============================================================================
std::string TimeSliceLabel(const CloudSeamanor::domain::GameClock& clock) {
    if (clock.Hour() < 10) return "上午";
    if (clock.Hour() < 14) return "中午";
    if (clock.Hour() < 18) return "下午";
    if (clock.Hour() < 22) return "傍晚";
    return "夜晚";
}

// ============================================================================
// 【HasTideForecast】检查明日是否预报大潮
// ============================================================================
bool HasTideForecast(const CloudSeamanor::domain::CloudSystem& cloud_system) {
    return cloud_system.ForecastState() == CloudSeamanor::domain::CloudState::Tide;
}

std::string ActionKeyLabel(Action action) {
    static const InputManager default_input_manager;
    return KeyName(default_input_manager.GetDefaultKey(action));
}

sf::Color WithAlpha(const sf::Color& color, std::uint8_t alpha) {
    return sf::Color(color.r, color.g, color.b, alpha);
}

// ============================================================================
// 【BestPlotToHarvest】找最优收获地块（优先高价值/大潮加成）
// ============================================================================
const TeaPlot* BestPlotToHarvest(const std::vector<TeaPlot>& tea_plots,
                                  CloudSeamanor::domain::CloudState cloud_state) {
    const TeaPlot* best = nullptr;
    int best_score = -1;

    for (const auto& plot : tea_plots) {
        if (!plot.ready) continue;
        int score = 0;
        // 大潮加成：成熟地块优先级最高
        if (cloud_state == CloudSeamanor::domain::CloudState::Tide) {
            score += 100;
        }
        // 品质加成
        switch (plot.quality) {
        case CloudSeamanor::domain::CropQuality::Spirit:  score += 40; break;
        case CloudSeamanor::domain::CropQuality::Rare:    score += 30; break;
        case CloudSeamanor::domain::CropQuality::Fine:    score += 20; break;
        default: break;
        }
        // 品种加成（legendary > premium > rare > core）
        if (plot.crop_id == "star_fruit") score += 50;
        if (plot.crop_id == "spirit_mushroom") score += 30;
        if (plot.crop_id == "moon_rice" || plot.crop_id == "mist_flower") score += 20;

        if (score > best_score) {
            best_score = score;
            best = &plot;
        }
    }
    return best;
}

// ============================================================================
// 【PlotToWater】找最需要浇水的地块（未浇水 + 未成熟 + 已播种）
// ============================================================================
const TeaPlot* PlotToWater(const std::vector<TeaPlot>& tea_plots) {
    const TeaPlot* best = nullptr;
    int best_score = -1;

    for (const auto& plot : tea_plots) {
        if (!plot.seeded || plot.ready || plot.watered) continue;
        int score = 0;
        // 高价值品种优先
        if (plot.crop_id == "star_fruit") score += 10;
        if (plot.crop_id == "spirit_mushroom") score += 8;
        if (plot.crop_id == "moon_rice") score += 6;
        // 快要成熟的地块优先
        if (plot.stage >= plot.growth_stages - 1) score += 5;

        if (score > best_score) {
            best_score = score;
            best = &plot;
        }
    }
    return best;
}

// ============================================================================
// 【BestRecipeToStart】找最适合当前库存的配方（优先高输出价值）
// ============================================================================
const CloudSeamanor::domain::RecipeDefinition* BestRecipeToStart(
    const CloudSeamanor::domain::Inventory& inventory,
    const std::string& machine_id) {
    // 直接从全局表查询可用配方
    const auto& table = CloudSeamanor::domain::GetGlobalRecipeTable();
    const auto available = table.AvailableRecipes(machine_id, {});

    // 构建真实库存计数
    std::unordered_map<std::string, int> real_counts;
    for (const auto& slot : inventory.Slots()) {
        real_counts[slot.item_id] = slot.count;
    }
    const auto doable = table.AvailableRecipes(machine_id, real_counts);

    if (doable.empty()) return nullptr;

    // 选择 process_time 最短（周转最快）的配方
    const CloudSeamanor::domain::RecipeDefinition* best = nullptr;
    int best_time = INT_MAX;
    for (const auto* recipe : doable) {
        if (recipe->process_time < best_time) {
            best_time = recipe->process_time;
            best = recipe;
        }
    }
    return best;
}

// ============================================================================
// 【NextNpcToGift】找最适合送礼的 NPC（优先低好感 + 未送礼）
// ============================================================================
const NpcActor* NextNpcToGift(const std::vector<NpcActor>& npcs,
                               const CloudSeamanor::domain::Inventory& inventory,
                               int current_hour) {
    const NpcActor* best = nullptr;
    int best_score = INT_MAX;

    for (const auto& npc : npcs) {
        if (npc.daily_gifted) continue;
        int score = 0;
        // 好感越低越优先（好感度梯度推动）
        score += npc.favor;
        // 傍晚/晚上优先送高好感 NPC（社交效率）
        if (current_hour >= 18 && npc.favor >= 60) score -= 30;
        // 有茶包时更倾向送有关系的 NPC
        if (inventory.CountOf("TeaPack") > 0 && npc.favor >= 50) score -= 10;

        if (score < best_score) {
            best_score = score;
            best = &npc;
        }
    }
    return best;
}

// ============================================================================
// 【CloudStateTextShort】云海状态短文本
// ============================================================================
const char* CloudStateTextShort(CloudSeamanor::domain::CloudState s) {
    switch (s) {
    case CloudSeamanor::domain::CloudState::Clear:       return "晴";
    case CloudSeamanor::domain::CloudState::Mist:        return "薄雾";
    case CloudSeamanor::domain::CloudState::DenseCloud:  return "浓云";
    case CloudSeamanor::domain::CloudState::Tide:        return "大潮";
    }
    return "未知";
}

// ============================================================================
// 【ProductionRecommendation】动态生产推荐
// ============================================================================
std::string ProductionRecommendation(const CloudSeamanor::domain::GameClock& clock,
                                      CloudSeamanor::domain::CloudState cloud_state,
                                      const CloudSeamanor::domain::CloudSystem& cloud_system,
                                      const RepairProject& main_house_repair,
                                      const CloudSeamanor::domain::Inventory& inventory,
                                      const TeaMachine& tea_machine,
                                      const std::vector<TeaPlot>& tea_plots) {
    const bool tide_forecast = HasTideForecast(cloud_system);
    const int hour = clock.Hour();

    // ===== 大潮特殊处理：最高优先级收获 =====
    if (cloud_state == CloudSeamanor::domain::CloudState::Tide) {
        if (const auto* best = BestPlotToHarvest(tea_plots, cloud_state)) {
            const auto quality_text = CloudSeamanor::domain::CropTable::QualityToPrefixText(best->quality);
            return "[大潮！] 生产：收获 " + quality_text + best->crop_name +
                   "（" + std::to_string(static_cast<int>(
                       CloudSeamanor::domain::CropTable::QualityHarvestMultiplier(best->quality) * 100.0f))
                   + "% 产量加成），这是今天最珍贵的收成！";
        }
    }

    // ===== 有大潮预报：提示明天准备 =====
    if (tide_forecast && hour < 18 && !tea_machine.running) {
        int seeded_count = 0;
        int unwatered_count = 0;
        for (const auto& p : tea_plots) {
            if (p.seeded) ++seeded_count;
            if (p.seeded && !p.watered && !p.ready) ++unwatered_count;
        }
        if (unwatered_count > 0) {
            return "[预告] 生产：明日大潮！今天先把 " + std::to_string(unwatered_count)
                   + " 块地浇好水，明早大潮加成翻倍。";
        }
        if (seeded_count < static_cast<int>(tea_plots.size())) {
            return "[预告] 生产：明日大潮！今天多播种，明天大潮时收获量翻 2.5 倍。";
        }
    }

    // ===== 成熟地块：优先收获 =====
    if (const auto* best = BestPlotToHarvest(tea_plots, cloud_state)) {
        const auto quality_text = CloudSeamanor::domain::CropTable::QualityToPrefixText(best->quality);
        std::string hint = "[高优先] 生产：收获 " + quality_text + best->crop_name;
        if (best->quality >= CloudSeamanor::domain::CropQuality::Rare) {
            hint += "（稀有品质，别错过）";
        } else {
            hint += "，这是今天最直接的产出回报。";
        }
        return hint;
    }

    // ===== 未浇水地块：催促浇水 =====
    if (const auto* water = PlotToWater(tea_plots)) {
        const auto growth_pct = static_cast<int>(
            water->growth / water->growth_time * 100.0f);
        return "[进行中] 生产：给 " + water->crop_name + " 浇水（进度 "
               + std::to_string(growth_pct) + "%），"
               + CloudStateTextShort(cloud_state) + "加成立刻生效。";
    }

    // ===== 制茶机空闲：有茶包则提示送礼，无茶包则提示加工 =====
    if (!tea_machine.running) {
        if (inventory.CountOf("TeaPack") > 0) {
            return "[建议] 生产：茶包已有库存，记得拜访 NPC 时送出推进关系。";
        }
        if (const auto* recipe = BestRecipeToStart(inventory, "tea_machine")) {
            return "[建议] 生产：把 " + ItemDisplayName(recipe->input_item) + " x"
                   + std::to_string(recipe->input_count)
                   + " 放进制茶机制作 " + recipe->name
                   + "（" + std::to_string(recipe->process_time) + "秒）。";
        }
        if (inventory.CountOf("TeaLeaf") >= 2) {
            return "[建议] 生产：把 茶叶 x2 放进制茶机，准备下一份可送礼的茶包。";
        }
    } else {
        const auto pct = static_cast<int>(tea_machine.progress);
        return "[进行中] 生产：制茶机运行中 " + std::to_string(pct)
               + "%，傍晚前查看进度并收产物。";
    }

    // ===== 主屋修缮未完成且材料足够 =====
    if (!main_house_repair.completed
        && inventory.CountOf("Wood") >= main_house_repair.wood_cost
        && inventory.CountOf("Turnip") >= main_house_repair.turnip_cost) {
        return "[建议] 生产：主屋修缮材料已齐，今天顺手完成可解锁新功能。";
    }

    // ===== 时段引导 =====
    if (hour < 10) {
        if (cloud_state == CloudSeamanor::domain::CloudState::DenseCloud) {
            return "[时段重点] 生产：上午 + 浓云高奖励，先巡一遍茶田与成熟作物。";
        }
        return "[时段重点] 生产：上午是农活核心时段，先翻土、播种或补浇今天的茶田。";
    }
    if (hour < 18) {
        return "[建议] 生产：整理茶田与库存，为下午制作和后续收成提前备料。";
    }

    return "[收尾] 生产：今天的核心产出已告一段落，晚上适合整理库存并准备明早。";
}

// ============================================================================
// 【SocialRecommendation】动态社交推荐
// ============================================================================
std::string SocialRecommendation(const CloudSeamanor::domain::GameClock& clock,
                                  const std::vector<NpcActor>& npcs,
                                  const CloudSeamanor::domain::Inventory& inventory,
                                  const CloudSeamanor::domain::CloudSystem& cloud_system) {
    const int hour = clock.Hour();
    const bool tide_forecast = HasTideForecast(cloud_system);
    const int tea_pack_count = inventory.CountOf("TeaPack");

    // ===== 所有 NPC 已送礼：关系已达今日上限 =====
    const auto gifted_all = std::all_of(npcs.begin(), npcs.end(),
        [](const NpcActor& n) { return n.daily_gifted; });
    if (gifted_all) {
        if (hour >= 18) {
            return "[已完成] 社交：今日关系推进已满，傍晚轻松聊聊天就好。";
        }
        return "[已完成] 社交：今天的关系推进已经达标，剩余时间自由安排。";
    }

    // ===== 有大潮预报时提前告知 =====
    if (tide_forecast && !gifted_all && hour < 12) {
        if (const auto* target = NextNpcToGift(npcs, inventory, hour)) {
            return "[预告] 社交：明日大潮！今天先和 " + target->display_name
                   + " 推进关系，明早大潮时送礼加成更高。";
        }
    }

    // ===== 找最适合下一个拜访的 NPC =====
    if (const auto* target = NextNpcToGift(npcs, inventory, hour)) {
        const std::string name = target->display_name;
        const int favor = target->favor;

        if (hour < 12) {
            // 上午：建议排进下午计划
            if (tea_pack_count > 0) {
                return "[排进计划] 社交：下午去拜访 " + name
                       + "（好感 " + std::to_string(favor) + "），送茶包推进今日关系。";
            }
            return "[排进计划] 社交：下午找 " + name
                   + "（好感 " + std::to_string(favor)
                   + "）聊聊，先熟悉行程再准备送礼。";
        }

        if (hour < 18) {
            // 下午：直接执行
            if (tea_pack_count > 0) {
                return "[进行中] 社交：拜访 " + name + "（好感 " + std::to_string(favor)
                       + "），送出茶包推进关系。";
            }
            return "[进行中] 社交：先和 " + name + "（好感 " + std::to_string(favor)
                   + "）对话，观察行程并准备后续送礼。";
        }

        // 傍晚：查漏补缺
        if (tea_pack_count > 0) {
            return "[补做] 社交：今天还没拜访 " + name + "，趁现在去送茶包。";
        }
        return "[补做] 社交：今天的关系还没推进，有茶包时记得送给 " + name + "。";
    }

    return "[建议] 社交：整理好库存中的茶包后，按计划拜访 NPC 推进关系。";
}

// ============================================================================
// 【GrowthRecommendation】动态养成推荐
// ============================================================================
std::string GrowthRecommendation(const CloudSeamanor::domain::GameClock& clock,
                                 const SpiritBeast& spirit_beast,
                                 bool spirit_beast_watered_today,
                                 const CloudSeamanor::domain::CloudSystem& cloud_system) {
    const int hour = clock.Hour();
    const bool tide_forecast = HasTideForecast(cloud_system);
    const bool spirit_interacted_today =
        (clock.Day() == spirit_beast.last_interaction_day);

    // ===== 灵兽还未结缘：今日必做 =====
    if (!spirit_interacted_today) {
        if (hour < 18) {
            return "[进行中] 养成：先和灵兽结缘一次，今天的协助能力就会激活。";
        }
        return "[优先补做] 养成：今晚还没和灵兽结缘，先完成这件小事再收尾。";
    }

    // ===== 大潮预报：提示灵兽协助的战略价值 =====
    if (tide_forecast && spirit_beast_watered_today) {
        return "[预告] 养成：明日大潮！灵兽今日协助已用，记得明早再让它帮忙浇水。";
    }

    // ===== 灵兽协助今日未触发 =====
    if (spirit_beast_watered_today) {
        return "[已完成] 养成：灵兽今日协助已激活，剩余时间轻松收尾。";
    }

    // ===== 灵兽协助还未触发 =====
    if (hour >= 18) {
        return "[补做] 养成：灵兽协助还没触发，留一点农活给它帮忙（浇水优先级最高）。";
    }

    // ===== 正常进行中 =====
    if (spirit_beast.state == SpiritBeastState::Follow) {
        return "[进行中] 养成：灵兽正在跟随中，可以在浇水时激活协助。";
    }
    if (hour >= 18) {
        return "[收尾] 养成：今天的灵兽互动已完成，晚上轻松收尾并查看明日预报。";
    }
    return "[建议] 养成：留一块待浇水地块给灵兽协助，节省体力。";
}

} // namespace

// ============================================================================
// 【SpiritBeastStateToInt】灵兽状态 -> 存档整数
// ============================================================================
int SpiritBeastStateToInt(SpiritBeastState state) {
    switch (state) {
    case SpiritBeastState::Idle:    return 0;
    case SpiritBeastState::Wander:  return 1;
    case SpiritBeastState::Follow:  return 2;
    case SpiritBeastState::Interact: return 3;
    }
    return 1;
}

// ============================================================================
// 【SpiritBeastStateFromInt】存档整数 -> 灵兽状态
// ============================================================================
SpiritBeastState SpiritBeastStateFromInt(int value) {
    switch (value) {
    case 0: return SpiritBeastState::Idle;
    case 1: return SpiritBeastState::Wander;
    case 2: return SpiritBeastState::Follow;
    case 3: return SpiritBeastState::Interact;
    default: return SpiritBeastState::Wander;
    }
}

// ============================================================================
// 【BuildControlsHint】底部控制提示
// ============================================================================
std::string BuildControlsHint() {
    return "移动(" + ActionKeyLabel(Action::MoveUp) + "/" + ActionKeyLabel(Action::MoveLeft)
        + "/" + ActionKeyLabel(Action::MoveDown) + "/" + ActionKeyLabel(Action::MoveRight)
        + ")  交互[" + ActionKeyLabel(Action::Interact) + "]  送礼[" + ActionKeyLabel(Action::GiftNpc)
        + "]  睡觉[" + ActionKeyLabel(Action::Sleep) + "](22:00后)  天气[" + ActionKeyLabel(Action::CloudToggle)
        + "]  保存[" + ActionKeyLabel(Action::QuickSave) + "]  读取[" + ActionKeyLabel(Action::QuickLoad) + "]";
}

std::string BuildNpcInteractionHint() {
    return "NPC 提示：[" + ActionKeyLabel(Action::Interact) + "] 对话  ["
        + ActionKeyLabel(Action::GiftNpc) + "] 赠送茶包";
}

// ============================================================================
// 【BuildDailyGoalText】当日目标文本
// ============================================================================
std::string BuildDailyGoalText(const RepairProject& main_house_repair,
                              const CloudSeamanor::domain::Inventory& inventory,
                              const TeaMachine& tea_machine,
                              const SpiritBeast& spirit_beast,
                              const std::vector<NpcActor>& npcs) {
    (void)spirit_beast;
    if (main_house_repair.level < kMainHouseMaxLevel) {
        const auto cost = QueryMainHouseUpgradeCost(main_house_repair.level);
        return "目标：主屋升级至 Lv." + std::to_string(cost.next_level)
            + "（木材 x" + std::to_string(cost.wood_cost)
            + " + 萝卜 x" + std::to_string(cost.turnip_cost)
            + " + 金币 x" + std::to_string(cost.gold_cost) + "）。";
    }
    if (inventory.CountOf("TeaLeaf") >= 2 && !tea_machine.running
        && inventory.CountOf("TeaPack") == 0) {
        return "目标：完成一轮制茶（萎凋→杀青→揉捻→干燥），做出茶包并送给 NPC。";
    }
    if (inventory.CountOf("TeaPack") > 0) {
        const auto ungifted = std::find_if(npcs.begin(), npcs.end(),
            [](const NpcActor& n) { return !n.daily_gifted; });
        if (ungifted != npcs.end()) {
            return "目标：拜访 " + ungifted->display_name
                   + " 并送出茶包推进关系。";
        }
    }
    return "目标：主屋已达 " + MainHouseLevelText_(main_house_repair.level)
        + "，今天可自由安排经营与探索。";
}

// ============================================================================
// 【BuildDailyRecommendations】每日推荐（完全数据驱动版）
// ============================================================================
std::vector<std::string> BuildDailyRecommendations(
    const CloudSeamanor::domain::GameClock& clock,
    CloudSeamanor::domain::CloudState cloud_state,
    const CloudSeamanor::domain::CloudSystem& cloud_system,
    const RepairProject& main_house_repair,
    const CloudSeamanor::domain::Inventory& inventory,
    const TeaMachine& tea_machine,
    const SpiritBeast& spirit_beast,
    bool spirit_beast_watered_today,
    const std::vector<TeaPlot>& tea_plots,
    const std::vector<NpcActor>& npcs) {
    return {
        ProductionRecommendation(clock, cloud_state, cloud_system,
                                 main_house_repair, inventory, tea_machine, tea_plots),
        SocialRecommendation(clock, npcs, inventory, cloud_system),
        GrowthRecommendation(clock, spirit_beast, spirit_beast_watered_today, cloud_system),
    };
}

// ============================================================================
// 【BuildWeatherAdviceText】天气建议文本
// ============================================================================
std::string BuildWeatherAdviceText(CloudSeamanor::domain::CloudState state,
                                   bool forecast_visible) {
    const std::string forecast_hint = forecast_visible
        ? "22:00 已公布明日预报，可提前安排。"
        : "22:00 会公布明日预报，今晚记得查看。";
    switch (state) {
    case CloudSeamanor::domain::CloudState::Clear:
        return "晴空：今天是稳定推进日，农活、采集、修缮都会顺畅进行。" + forecast_hint;
    case CloudSeamanor::domain::CloudState::Mist:
        return "薄雾：今天是成长加速日，作物生长+15%，很适合播种、浇水和整理茶园。" + forecast_hint;
    case CloudSeamanor::domain::CloudState::DenseCloud:
        return "浓云海：今天是高奖励日，作物生长+40%，稀有灵茶/灵兽出现率+50%，优先照看已浇水作物。" + forecast_hint;
    case CloudSeamanor::domain::CloudState::Tide:
        return "大潮：传说级天气！作物生长+70%且圣品概率+30%，传说灵茶/灵兽必出，灵界掉落翻倍！这是最珍贵的日子。" + forecast_hint;
    }
    return "天气未知。";
}

// ============================================================================
// 【PhaseText】昼夜阶段文本
// ============================================================================
std::string PhaseText(CloudSeamanor::domain::DayPhase phase) {
    switch (phase) {
    case CloudSeamanor::domain::DayPhase::Morning:    return "早晨";
    case CloudSeamanor::domain::DayPhase::Afternoon:  return "下午";
    case CloudSeamanor::domain::DayPhase::Evening:    return "傍晚";
    case CloudSeamanor::domain::DayPhase::Night:      return "夜晚";
    }
    return "未知";
}

// ============================================================================
// 【PhaseTint】昼夜阶段色调
// ============================================================================
sf::Color PhaseTint(CloudSeamanor::domain::DayPhase phase) {
    switch (phase) {
    case CloudSeamanor::domain::DayPhase::Morning:
        return WithAlpha(ColorPalette::Season::SpringYellow, 18);
    case CloudSeamanor::domain::DayPhase::Afternoon:
        return WithAlpha(ColorPalette::BackgroundWhite, 0);
    case CloudSeamanor::domain::DayPhase::Evening:
        return WithAlpha(ColorPalette::Season::AutumnGold, 26);
    case CloudSeamanor::domain::DayPhase::Night:
        return WithAlpha(ColorPalette::Weather::DenseSky, 42);
    }
    return WithAlpha(ColorPalette::BackgroundWhite, 0);
}

// ============================================================================
// 【PickupColorFor】物品掉落颜色
// ============================================================================
sf::Color PickupColorFor(const std::string& item_id) {
    if (item_id == "TeaLeaf") return ColorPalette::StaminaNormal;
    if (item_id == "TeaPack") return ColorPalette::LightBrown;
    if (item_id == "withered_tea_leaf") return ColorPalette::LightBrown;
    if (item_id == "killed_green_tea_leaf") return ColorPalette::LightBrown;
    if (item_id == "rolled_tea_leaf") return ColorPalette::LightBrown;
    if (item_id == "Wood") return ColorPalette::DeepBrown;
    if (item_id == "Turnip") return ColorPalette::Season::SpringPink;
    return ColorPalette::CoinGold;
}

// ============================================================================
// 【ItemDisplayName】物品显示名称
// ============================================================================
std::string ItemDisplayName(const std::string& item_id) {
    if (item_id == "TeaSeed") return "茶种";
    if (item_id == "TurnipSeed") return "萝卜种子";
    if (item_id == "TeaLeaf") return "茶叶";
    if (item_id == "TeaPack") return "茶包";
    if (item_id == "withered_tea_leaf") return "萎凋茶叶";
    if (item_id == "killed_green_tea_leaf") return "杀青茶叶";
    if (item_id == "rolled_tea_leaf") return "揉捻茶叶";
    if (item_id == "Turnip") return "萝卜";
    if (item_id == "Wood") return "木材";
    if (item_id == "SprinklerItem") return "洒水器";
    if (item_id == "FertilizerItem") return "普通肥料";
    if (item_id == "premium_fertilizer") return "优质肥料";
    if (item_id == "spirit_fertilizer") return "灵肥";
    if (item_id == "cloud_fertilizer") return "云华肥";
    if (item_id == "tea_soul_fertilizer") return "茶魂肥";
    if (item_id == "spirit_grass") return "灵草";
    if (item_id == "cloud_dew") return "云露";
    if (item_id == "spirit_dust") return "灵尘";
    if (item_id == "star_fragment") return "星辰碎片";
    if (item_id == "PesticideItem") return "杀虫剂";
    if (item_id == "SpiritSickle") return "灵气镰刀";
    if (item_id == "ToolHoe") return "锄头（耐久）";
    if (item_id == "ToolAxe") return "斧头（耐久）";
    if (item_id == "ToolPickaxe") return "镐子（耐久）";
    if (item_id == "ToolSickle") return "镰刀（耐久）";
    if (item_id == "spirit_essence") return "灵粹";
    if (item_id == "cloud_elixir") return "云华灵药";
    if (item_id == "immortal_tea") return "不朽灵茶";
    if (item_id == "taichu_tea") return "太初灵茶";
    if (item_id == "taichu_tea_base") return "太初灵茶原液";

    // ========== 灵茶种类名称 ==========
    // 入门级灵茶
    if (item_id == "mist_green_tea") return "初雾绿茶";
    if (item_id == "dew_white_tea") return "晨露白茶";
    // 中级灵茶
    if (item_id == "sunset_oolong") return "暮霞乌龙";
    if (item_id == "cloudberry_red") return "云莓红茶";
    if (item_id == "moonlight_silver") return "望月银茶";
    if (item_id == "autumn_frost_oolong") return "秋霜乌龙";
    if (item_id == "pine_smoke_black") return "松烟黑茶";
    if (item_id == "summer_cloud_tea") return "夏云白茶";
    // 高级灵茶
    if (item_id == "spirit_orchid_tea") return "灵兰花茶";
    if (item_id == "frost_needle_tea") return "霜针灵茶";
    if (item_id == "winter_snow_silver") return "冬雪银针";
    if (item_id == "rock_bone_tea") return "岩骨黑茶";
    if (item_id == "star_frost_tea") return "星霜雪茶";
    // 特殊/传说级灵茶
    if (item_id == "cloud_top_golden") return "云巅金芽";
    if (item_id == "dream_cloud_purple") return "梦云紫茶";
    if (item_id == "tide_heart_tea") return "潮心古茶";
    if (item_id == "origin_tea") return "太初元茶";

    // ========== 灵茶加工品名称 ==========
    if (item_id == "mist_green_tea_pack") return "初雾绿茶包";
    if (item_id == "dew_white_tea_pack") return "晨露白茶包";
    if (item_id == "sunset_oolong_pack") return "暮霞乌龙包";
    if (item_id == "cloudberry_red_pack") return "云莓红茶包";
    if (item_id == "moonlight_silver_pack") return "望月银茶包";
    if (item_id == "autumn_frost_oolong_pack") return "秋霜乌龙包";
    if (item_id == "pine_smoke_black_pack") return "松烟黑茶包";
    if (item_id == "summer_cloud_tea_pack") return "夏云白茶包";
    if (item_id == "spirit_orchid_tea_pack") return "灵兰花茶包";
    if (item_id == "frost_needle_tea_pack") return "霜针灵茶包";
    if (item_id == "winter_snow_silver_pack") return "冬雪银针包";
    if (item_id == "rock_bone_tea_pack") return "岩骨黑茶包";
    if (item_id == "star_frost_tea_pack") return "星霜雪茶包";
    if (item_id == "cloud_top_golden_pack") return "云巅金芽礼盒";
    if (item_id == "dream_cloud_purple_pack") return "梦云紫茶包";
    if (item_id == "tide_heart_tea_pack") return "潮心古茶包";
    if (item_id == "origin_tea_pack") return "太初元茶包";
    if (item_id == "tide_heart_tea_brew") return "潮心古茶酿";
    if (item_id == "tea_essence") return "茶灵精华";
    if (item_id == "spirit_tea_blend") return "灵雾拼配茶";
    if (item_id == "steamed_tea") return "蒸汽茶";
    if (item_id == "fermented_tea") return "发酵茶";
    if (item_id == "fermented_tea_plus") return "熟成发酵茶";

    if (item_id == "JadeRing") return "翡翠戒指";
    if (item_id == "Feed") return "饲料";
    if (item_id == "Egg") return "鸡蛋";
    if (item_id == "Milk") return "牛奶";
    if (item_id == "tea_egg") return "茶叶蛋";
    if (item_id == "veggie_soup") return "蔬菜汤";
    if (item_id == "warm_milk") return "热牛奶";
    if (item_id == "mist_carp") return "雾鲤";
    if (item_id == "cloud_koi") return "云锦鲤";
    if (item_id == "moon_silverfish") return "月银鱼";
    if (item_id == "tea_shrimp") return "茶虾";
    if (item_id == "tide_eel") return "潮鳗";
    if (item_id == "fish_oil") return "鱼油凝脂";
    if (item_id == "tea_room_screen") return "茶室屏风";
    if (item_id == "tea_room_lamp") return "茶室灯盏";
    if (item_id == "tea_room_vase") return "茶室花瓶";
    if (item_id == "tea_room_scroll") return "茶室挂轴";
    if (item_id == "tea_room_mat") return "茶室蒲团";
    if (item_id == "tea_room_bonsai") return "茶室盆景";
    if (item_id == "heirloom_album") return "传承册";
    if (item_id == "pet_cat_license") return "猫咪认养证";
    if (item_id == "pet_dog_license") return "小狗认养证";
    if (item_id == "pet_bird_license") return "飞鸟认养证";
    // RecipeData 新增名称映射
    if (item_id == "white_tea_pack") return "白茶包";
    if (item_id == "oolong_tea_pack") return "乌龙茶包";
    if (item_id == "spirit_tea_pack") return "灵茶包";
    if (item_id == "fermented_tea_pack") return "发酵茶包";
    // 洒水器名称
    if (item_id == "sprinkler_copper") return "铜洒水器";
    if (item_id == "sprinkler_silver") return "银洒水器";
    if (item_id == "sprinkler_gold") return "金洒水器";
    if (item_id == "sprinkler_spirit") return "灵金洒水器";
    // 锄头名称
    if (item_id == "hoe_copper") return "铜锄";
    if (item_id == "hoe_silver") return "银锄";
    if (item_id == "hoe_gold") return "金锄";
    if (item_id == "hoe_spirit") return "灵金锄";
    // 水壶名称
    if (item_id == "watering_can_copper") return "铜水壶";
    if (item_id == "watering_can_silver") return "银水壶";
    if (item_id == "watering_can_gold") return "金水壶";
    if (item_id == "watering_can_spirit") return "灵金水壶";
    // 金属锭
    if (item_id == "copper_ingot") return "铜锭";
    if (item_id == "silver_ingot") return "银锭";
    if (item_id == "gold_ingot") return "金锭";
    if (item_id == "spirit_ingot") return "灵金锭";
    // 镰刀名称
    if (item_id == "sickle_copper") return "铜镰刀";
    if (item_id == "sickle_silver") return "银镰刀";
    if (item_id == "sickle_gold") return "金镰刀";
    if (item_id == "sickle_spirit") return "灵金镰刀";
    // 剪刀名称
    if (item_id == "scissors_copper") return "铜剪刀";
    if (item_id == "scissors_silver") return "银剪刀";
    if (item_id == "scissors_gold") return "金剪刀";
    if (item_id == "scissors_spirit") return "灵金剪刀";
    // 斧头名称
    if (item_id == "axe_copper") return "铜斧头";
    if (item_id == "axe_silver") return "银斧头";
    if (item_id == "axe_gold") return "金斧头";
    if (item_id == "axe_spirit") return "灵金斧头";
    // 镐子名称
    if (item_id == "pickaxe_copper") return "铜镐子";
    if (item_id == "pickaxe_silver") return "银镐子";
    if (item_id == "pickaxe_gold") return "金镐子";
    if (item_id == "pickaxe_spirit") return "灵金镐子";

    // ========== Buff效果名称 ==========
    if (item_id == "buff_focus_small") return "清心（灵茶Buff）";
    if (item_id == "buff_stamina_small") return "养气（灵茶Buff）";
    if (item_id == "buff_trade_bonus") return "通商（灵茶Buff）";
    if (item_id == "buff_mood_soft") return "悦心（灵茶Buff）";
    if (item_id == "buff_agility_small") return "轻身（灵茶Buff）";
    if (item_id == "buff_defense_small") return "护体（灵茶Buff）";
    if (item_id == "buff_creativity") return "灵感（灵茶Buff）";
    if (item_id == "buff_defense_mid") return "金钟（灵茶Buff）";
    if (item_id == "buff_focus_mid") return "入定（灵茶Buff）";
    if (item_id == "buff_spirit_rate") return "灵聚（灵茶Buff）";
    if (item_id == "buff_dream_creative") return "梦创（灵茶Buff）";
    if (item_id == "buff_spirit_rate_legendary") return "灵潮（灵茶Buff）";
    if (item_id == "buff_mastery") return "通玄（灵茶Buff）";

    return item_id;
}

// ============================================================================
// 【SpiritBeastStateText】灵兽状态文本
// ============================================================================
const char* SpiritBeastStateText(SpiritBeastState state) {
    switch (state) {
    case SpiritBeastState::Idle:    return "休息";
    case SpiritBeastState::Wander:  return "游荡";
    case SpiritBeastState::Follow:  return "跟随";
    case SpiritBeastState::Interact: return "互动";
    }
    return "未知";
}

const char* SpiritBeastPersonalityText(SpiritBeastPersonality personality) {
    switch (personality) {
    case SpiritBeastPersonality::Lively: return "活泼";
    case SpiritBeastPersonality::Lazy: return "慵懒";
    case SpiritBeastPersonality::Curious: return "好奇";
    }
    return "活泼";
}

int NpcHeartLevelFromFavor(int favor) {
    constexpr int kHeartThresholds[] = {0, 50, 120, 220, 350, 500, 700, 950, 1250, 1600, 2000};
    int level = 0;
    for (int i = 0; i <= 10; ++i) {
        if (favor >= kHeartThresholds[i]) {
            level = i;
        } else {
            break;
        }
    }
    return level;
}

std::string NpcHeartText(int heart_level) {
    const int clamped = std::clamp(heart_level, 0, 10);
    std::string result;
    result.reserve(10);
    for (int i = 0; i < 10; ++i) {
        result += (i < clamped) ? "♥" : "♡";
    }
    return result;
}

std::string NpcCloudStageText(int heart_level) {
    const int clamped = std::clamp(heart_level, 0, 10);
    if (clamped >= 10) {
        return "七彩祥云";
    }
    if (clamped >= 7) {
        return "霞云";
    }
    if (clamped >= 4) {
        return "层云";
    }
    return "薄云";
}

// ============================================================================
// 【CombineOverlayColor】叠加颜色
// ============================================================================
sf::Color CombineOverlayColor(sf::Color base, sf::Color tint) {
    return sf::Color(
        static_cast<std::uint8_t>(std::min(255, static_cast<int>(base.r) + static_cast<int>(tint.r) / 4)),
        static_cast<std::uint8_t>(std::min(255, static_cast<int>(base.g) + static_cast<int>(tint.g) / 4)),
        static_cast<std::uint8_t>(std::min(255, static_cast<int>(base.b) + static_cast<int>(tint.b) / 4)),
        static_cast<std::uint8_t>(std::min(255, static_cast<int>(base.a) + static_cast<int>(tint.a))));
}

// ============================================================================
// 【BackgroundColorFor】背景颜色
// ============================================================================
sf::Color BackgroundColorFor(const CloudSeamanor::domain::CloudState state) {
    switch (state) {
    case CloudSeamanor::domain::CloudState::Clear:
        return ColorPalette::Weather::ClearSky;
    case CloudSeamanor::domain::CloudState::Mist:
        return ColorPalette::Weather::MistSky;
    case CloudSeamanor::domain::CloudState::DenseCloud:
        return ColorPalette::Weather::DenseSky;
    case CloudSeamanor::domain::CloudState::Tide:
        return ColorPalette::Weather::TideSky;
    }
    return ColorPalette::Weather::DenseSky;
}

// ============================================================================
// 【CloudGrowthMultiplier】作物生长倍率
// ============================================================================
float CloudGrowthMultiplier(const CloudSeamanor::domain::CloudState state) {
    switch (state) {
    case CloudSeamanor::domain::CloudState::Clear:      return 1.0f;
    case CloudSeamanor::domain::CloudState::Mist:       return 1.15f;
    case CloudSeamanor::domain::CloudState::DenseCloud: return 1.4f;
    case CloudSeamanor::domain::CloudState::Tide:       return 1.7f;
    }
    return 1.0f;
}

// ============================================================================
// 【AuraColorFor】灵气覆盖层颜色
// ============================================================================
sf::Color AuraColorFor(const CloudSeamanor::domain::CloudState state) {
    switch (state) {
    case CloudSeamanor::domain::CloudState::Clear:
        return WithAlpha(ColorPalette::Season::SpringYellow, 42);
    case CloudSeamanor::domain::CloudState::Mist:
        return WithAlpha(ColorPalette::Season::SummerBlue, 54);
    case CloudSeamanor::domain::CloudState::DenseCloud:
        return WithAlpha(ColorPalette::Season::WinterLavender, 76);
    case CloudSeamanor::domain::CloudState::Tide:
        return WithAlpha(ColorPalette::Weather::TideGlow, 95);
    }
    return WithAlpha(ColorPalette::BackgroundWhite, 40);
}

} // namespace CloudSeamanor::engine
