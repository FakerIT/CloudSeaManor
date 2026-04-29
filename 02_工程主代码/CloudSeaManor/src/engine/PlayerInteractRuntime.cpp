#include "CloudSeamanor/engine/PlayerInteractRuntime.hpp"
#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/domain/DynamicLifeSystem.hpp"
#include "CloudSeamanor/engine/GameWorldState.hpp"
#include "CloudSeamanor/app/GameAppNpc.hpp"
#include "CloudSeamanor/app/GameAppFarming.hpp"
#include "CloudSeamanor/app/GameAppText.hpp"
#include "CloudSeamanor/infrastructure/GameConstants.hpp"
#include "CloudSeamanor/domain/GameClock.hpp"
#include "CloudSeamanor/engine/Interactable.hpp"
#include "CloudSeamanor/domain/Inventory.hpp"
#include "CloudSeamanor/engine/PickupDrop.hpp"
#include "CloudSeamanor/domain/Stamina.hpp"
#include "CloudSeamanor/domain/WorkshopSystem.hpp"
#include "CloudSeamanor/app/GameAppText.hpp"
#include "CloudSeamanor/engine/systems/DecorationSystem.hpp"
#include "CloudSeamanor/engine/systems/ShopSystem.hpp"
#include "CloudSeamanor/engine/EventBus.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"
#include "CloudSeamanor/domain/HungerTable.hpp"
#include "CloudSeamanor/domain/ToolSystem.hpp"
#include "CloudSeamanor/domain/DiarySystem.hpp"

#include "CloudSeamanor/engine/DialogueEngine.hpp"
#include "CloudSeamanor/Profiling.hpp"

#include <algorithm>
#include <cstdint>

namespace CloudSeamanor::engine {

namespace {

const DecorationSystem kDecorationSystem;
const ShopSystem kShopSystem;
constexpr std::uint32_t kGiftHeartLovedColor = PackRgba(220, 64, 96, 255);
constexpr std::uint32_t kGiftHeartLikedColor = PackRgba(255, 153, 204, 255);
constexpr std::uint32_t kGiftHeartDislikedColor = PackRgba(160, 160, 160, 255);
constexpr std::uint32_t kGiftHeartNeutralColor = PackRgba(255, 182, 193, 255);

bool IsTeaSeedItemId_(const std::string& seed_item_id) {
    return seed_item_id.find("Tea") != std::string::npos
        || seed_item_id.find("tea") != std::string::npos;
}

void UpdateUiAfterInteraction(PlayerInteractRuntimeContext& ctx) {
    ctx.update_hud_text();
    ctx.refresh_window_title();
}

std::string LatestDiarySummary_(const std::vector<DiaryEntryState>& entries) {
    if (entries.empty()) {
        return "日记本还是空白的。去收获、净化或参加节庆，留下第一篇记录吧。";
    }
    const CloudSeamanor::domain::DiarySystem diary_system;
    const auto& latest = entries.back();
    if (const auto* def = diary_system.FindById(latest.entry_id)) {
        return "【庄园日记】" + def->title + "： " + def->summary;
    }
    return "【庄园日记】已记录第 " + std::to_string(latest.day_unlocked) + " 天的山庄见闻。";
}

std::string CatchFishByHour_(int hour) {
    if (hour < 10) return "mist_carp";
    if (hour < 18) return "cloud_koi";
    return "moon_silverfish";
}

std::string CatchFishForSeason_(const CloudSeamanor::domain::GameClock& clock, int attempts) {
    using CloudSeamanor::domain::Season;
    switch (clock.Season()) {
    case Season::Spring:
        return (attempts % 2 == 0) ? "tea_shrimp" : "mist_carp";
    case Season::Summer:
        return (clock.Hour() >= 18) ? "tide_eel" : "cloud_koi";
    case Season::Autumn:
        return "cloud_koi";
    case Season::Winter:
        return "moon_silverfish";
    }
    return CatchFishByHour_(clock.Hour());
}

std::string BranchAForSkill_(const std::string& skill_id) {
    if (skill_id == "灵农") return "abundance";
    if (skill_id == "灵觅") return "swiftstep";
    if (skill_id == "灵钓") return "tidewatch";
    if (skill_id == "灵矿") return "refine";
    if (skill_id == "灵卫") return "ward";
    return "abundance";
}

std::string BranchBForSkill_(const std::string& skill_id) {
    if (skill_id == "灵农") return "conservation";
    if (skill_id == "灵觅") return "pathfinder";
    if (skill_id == "灵钓") return "deepcurrent";
    if (skill_id == "灵矿") return "prospect";
    if (skill_id == "灵卫") return "barrier";
    return "conservation";
}

bool ResolvePendingBranchChoice_(PlayerInteractRuntimeContext& ctx, bool choose_a) {
    if (ctx.pending_skill_branches.empty()) {
        return false;
    }
    const std::string skill_id = ctx.pending_skill_branches.front();
    ctx.pending_skill_branches.erase(ctx.pending_skill_branches.begin());
    ctx.skill_branches[skill_id] = choose_a ? BranchAForSkill_(skill_id) : BranchBForSkill_(skill_id);
    ctx.push_hint(
        "【分支选择】" + skill_id + " 已选择 "
            + (choose_a ? "A" : "B") + " 分支：" + ctx.skill_branches[skill_id],
        3.0f);
    return true;
}

int ApplyNpcFavorDelta(NpcActor& npc, int delta) {
    constexpr int kDailyFavorCap = 50;
    auto MoodFavorMultiplier_ = [](NpcMood mood) -> float {
        switch (mood) {
        case NpcMood::Sad:
        case NpcMood::Angry:
            return 0.5f;
        case NpcMood::Happy:
        case NpcMood::Normal:
        default:
            return 1.0f;
        }
    };
    if (delta > 0) {
        const float mult = MoodFavorMultiplier_(npc.mood);
        delta = std::max(1, static_cast<int>(static_cast<float>(delta) * mult));
        const int remain = std::max(0, kDailyFavorCap - npc.daily_favor_gain);
        delta = std::min(delta, remain);
    }
    npc.favor += delta;
    if (delta > 0) {
        npc.daily_favor_gain += delta;
    }
    npc.heart_level = NpcHeartLevelFromFavor(npc.favor);
    return delta;
}

void TintRecentHearts_(std::vector<HeartParticle>& particles,
                       std::size_t from_index,
                       std::uint32_t color_rgba) {
    for (std::size_t i = from_index; i < particles.size(); ++i) {
        const std::uint32_t a = (particles[i].color_rgba & 0xFFu);
        particles[i].color_rgba = (color_rgba & 0xFFFFFF00u) | a;
    }
}

void AdoptPet_(
    PlayerInteractRuntimeContext& ctx,
    const std::string& pet_type,
    const std::string& source_label) {
    if (!ctx.pet_adopted) {
        ctx.pet_adopted = true;
        ctx.pet_type = pet_type;
        ctx.push_hint("你收养了一只宠物（" + pet_type + "）。来源：" + source_label + "。", 2.4f);
    } else if (ctx.pet_type != pet_type) {
        ctx.pet_type = pet_type;
        ctx.push_hint("你将当前陪伴宠物切换为：" + pet_type + "。", 2.0f);
    } else {
        ctx.push_hint("宠物正在院子里玩耍。", 2.0f);
    }
    GlobalEventBus().Emit(Event{
        "pet_collected",
        {
            {"pet_type", pet_type},
        }});
}

const char* PetTypeFromCommodity_(const std::string& item_id) {
    if (item_id == "pet_cat_license") return "cat";
    if (item_id == "pet_dog_license") return "dog";
    if (item_id == "pet_bird_license") return "bird";
    return nullptr;
}

// ============================================================================
// 【GetBestSickleCollectEfficiency】获取最佳镰刀收割效率加成
// ============================================================================
float GetBestSickleCollectEfficiency_(const CloudSeamanor::domain::Inventory& inventory) {
    using namespace CloudSeamanor::domain;
    float best_efficiency = 1.0f;

    // 检查各等级镰刀
    if (inventory.CountOf("sickle_copper") > 0) {
        best_efficiency = std::max(best_efficiency, 1.15f);
    }
    if (inventory.CountOf("sickle_silver") > 0) {
        best_efficiency = std::max(best_efficiency, 1.25f);
    }
    if (inventory.CountOf("sickle_gold") > 0) {
        best_efficiency = std::max(best_efficiency, 1.40f);
    }
    if (inventory.CountOf("sickle_spirit") > 0) {
        best_efficiency = std::max(best_efficiency, 1.60f);
    }
    // 旧版灵气镰刀
    if (inventory.CountOf("SpiritSickle") > 0) {
        best_efficiency = std::max(best_efficiency, 1.30f);
    }
    return best_efficiency;
}

int ObstacleHitPowerFromEfficiency_(float efficiency) {
    if (efficiency >= 2.0f) return 3;
    if (efficiency >= 1.4f) return 2;
    return 1;
}

float GetBestAxeObstacleEfficiency_(const CloudSeamanor::domain::Inventory& inventory) {
    float best_efficiency = 1.0f;
    if (inventory.CountOf("axe_copper") > 0) {
        best_efficiency = std::max(best_efficiency, 1.2f);
    }
    if (inventory.CountOf("axe_silver") > 0) {
        best_efficiency = std::max(best_efficiency, 1.4f);
    }
    if (inventory.CountOf("axe_gold") > 0) {
        best_efficiency = std::max(best_efficiency, 1.6f);
    }
    if (inventory.CountOf("axe_spirit") > 0) {
        best_efficiency = std::max(best_efficiency, 2.0f);
    }
    return best_efficiency;
}

float GetBestPickaxeObstacleEfficiency_(const CloudSeamanor::domain::Inventory& inventory) {
    float best_efficiency = 1.0f;
    if (inventory.CountOf("pickaxe_copper") > 0) {
        best_efficiency = std::max(best_efficiency, 1.2f);
    }
    if (inventory.CountOf("pickaxe_silver") > 0) {
        best_efficiency = std::max(best_efficiency, 1.4f);
    }
    if (inventory.CountOf("pickaxe_gold") > 0) {
        best_efficiency = std::max(best_efficiency, 1.6f);
    }
    if (inventory.CountOf("pickaxe_spirit") > 0) {
        best_efficiency = std::max(best_efficiency, 2.0f);
    }
    return best_efficiency;
}

std::vector<const PriceTableEntry*> CollectPetCommodityOptions_(
    const std::vector<PriceTableEntry>& price_table,
    const std::string& source) {
    std::vector<const PriceTableEntry*> options;
    for (const auto& entry : price_table) {
        if (entry.buy_from == source && entry.buy_price > 0 && entry.category == "pet") {
            options.push_back(&entry);
        }
    }
    return options;
}

bool HandlePetCommodityPurchase_(
    PlayerInteractRuntimeContext& ctx,
    const PriceTableEntry& selected,
    const std::string& source_label) {
    const char* pet_type = PetTypeFromCommodity_(selected.item_id);
    if (pet_type == nullptr) {
        return false;
    }
    AdoptPet_(ctx, pet_type, source_label);
    return true;
}

const PriceTableEntry* FindPrice_(const std::vector<PriceTableEntry>& table, const std::string& item_id) {
    for (const auto& e : table) {
        if (e.item_id == item_id) return &e;
    }
    return nullptr;
}

std::vector<std::string> FestivalOfferItemIds_(const std::string& festival_id) {
    if (festival_id == "spring_festival" || festival_id == "spring_awakening") {
        return {"TeaSeed", "FertilizerItem", "Feed"};
    }
    if (festival_id == "lantern_festival" || festival_id == "summer_lantern") {
        return {"JadeRing", "star_fragment", "SpiritSickle"};
    }
    if (festival_id == "flower_festival") {
        return {"TeaSeed", "FertilizerItem", "JadeRing"};
    }
    if (festival_id == "qingming_festival") {
        return {"TeaSeed", "Wood", "FertilizerItem"};
    }
    if (festival_id == "dragon_boat") {
        return {"Feed", "TeaPack", "SpiritSickle"};
    }
    if (festival_id == "qixi_festival") {
        return {"JadeRing", "TeaPack", "star_fragment"};
    }
    if (festival_id == "mid_autumn") {
        return {"TeaPack", "JadeRing", "FertilizerItem"};
    }
    if (festival_id == "double_ninth") {
        return {"TeaPack", "TeaSeed", "Feed"};
    }
    if (festival_id == "winter_solstice") {
        return {"Feed", "TeaPack", "FertilizerItem"};
    }
    if (festival_id == "tea_culture_day") {
        return {"TeaPack", "TeaSeed", "FertilizerItem"};
    }
    if (festival_id == "harvest_festival" || festival_id == "autumn_harvest") {
        return {"SprinklerItem", "TeaSeed", "FertilizerItem"};
    }
    if (festival_id == "cloud_tide_ritual" || festival_id == "cloud_tide") {
        return {"star_fragment", "JadeRing", "SpiritSickle"};
    }
    return {"TeaSeed", "FertilizerItem"};
}

bool HandleFestivalBooth_(PlayerInteractRuntimeContext& ctx) {
    if (ctx.active_festival_id.empty()) {
        ctx.push_hint("节日摊位：当前没有节日活动。", 2.0f);
        return true;
    }
    const std::string& fid = ctx.active_festival_id;
    const std::string key = "fest_booth|" + fid + "|" + std::to_string(ctx.current_day);
    if (ctx.weekly_buy_count[key] > 0) {
        ctx.push_hint("今日已在节日摊位完成互动。", 2.0f);
        return true;
    }

    if (fid == "lantern_festival" || fid == "summer_lantern") {
        ctx.gold += 40;
        ctx.stamina.Recover(12.0f);
        ctx.push_hint("【元宵·灯谜】“云底灯如星”（打一物）——谜底：天灯。你赢得小奖！", 3.0f);
    } else if (fid == "qixi_festival") {
        const int bonus = 30 + (ctx.current_day % 17);
        ctx.gold += bonus;
        ctx.push_hint("【七夕·许愿】流星应声：金币 +" + std::to_string(bonus) + "。", 2.8f);
    } else if (fid == "dragon_boat") {
        if (ctx.inventory.CountOf("spirit_grass") >= 1 && ctx.inventory.CountOf("TeaPack") >= 1) {
            (void)ctx.inventory.TryRemoveItem("spirit_grass", 1);
            (void)ctx.inventory.TryRemoveItem("TeaPack", 1);
            ctx.stamina.Recover(40.0f);
            ctx.gold += 60;
            ctx.push_hint("【端午·包粽】竹叶裹香：体力大幅恢复，金币 +60。", 3.0f);
        } else {
            ctx.push_hint("包粽子需要：灵草 x1 + 茶包 x1。", 2.4f);
            return true;
        }
    } else if (fid == "qingming_festival") {
        ctx.gold += 55;
        ctx.push_hint("【清明·祭祖】心香一瓣：金币 +55。踏青双倍灵草已在今日生效。", 2.8f);
    } else if (fid == "mid_autumn") {
        ctx.stamina.Recover(25.0f);
        ctx.push_hint("【中秋·赏月】咬一口“月亮”：体力 +25。", 2.6f);
    } else if (fid == "double_ninth") {
        ctx.stamina.Recover(18.0f);
        ctx.push_hint("【重阳·登高】一口气登上心尖：体力 +18。", 2.6f);
    } else if (fid == "tea_culture_day") {
        const int score = 70 + (static_cast<int>(ctx.inventory.CountOf("TeaPack")) * 3 + ctx.current_day) % 26;
        const int prize = 30 + score;
        ctx.gold += prize;
        ctx.push_hint("【茶文化节·斗茶】评委打分 " + std::to_string(score) + "：奖金 +" + std::to_string(prize) + "。", 3.0f);
    } else if (fid == "harvest_festival" || fid == "autumn_harvest") {
        const int crates = static_cast<int>(ctx.inventory.CountOf("Turnip"))
            + static_cast<int>(ctx.inventory.CountOf("TeaLeaf"));
        ctx.gold += 45 + std::min(80, crates * 2);
        ctx.push_hint("【丰收祭·评选】今日收成登记完成：额外金币奖励已发放。", 2.8f);
    } else if (fid == "flower_festival") {
        ctx.inventory.AddItem("TeaSeed", 1);
        ctx.push_hint("【花朝·簪花】花神赠你种子 x1。", 2.6f);
    } else if (fid == "winter_solstice") {
        ctx.stamina.Recover(20.0f);
        ctx.push_hint("【冬至·饺子】热气腾腾：体力 +20。", 2.4f);
    } else {
        ctx.gold += 25;
        ctx.push_hint("【节日摊位】" + fid + " 小礼：金币 +25。", 2.4f);
    }

    ctx.weekly_buy_count[key] = 1;
    return true;
}

const PriceTableEntry* FindFestivalOffer_(
    const std::vector<PriceTableEntry>& table,
    const std::vector<std::string>& offer_ids,
    int current_day) {
    if (offer_ids.empty()) {
        return nullptr;
    }
    const int start = std::max(0, current_day) % static_cast<int>(offer_ids.size());
    for (std::size_t i = 0; i < offer_ids.size(); ++i) {
        const std::string& item_id = offer_ids[(static_cast<std::size_t>(start) + i) % offer_ids.size()];
        const auto* entry = FindPrice_(table, item_id);
        if (entry && entry->buy_price > 0) {
            return entry;
        }
    }
    return nullptr;
}

bool BuildRelationshipDialogueMenu_(
    PlayerInteractRuntimeContext& ctx,
    const NpcActor& npc,
    const std::string& original_start_id) {
    auto& rel_state = ctx.relationship_state;
    const auto& rel_system = ctx.relationship_system;

    const bool has_token = ctx.inventory.CountOf(rel_system.Confession().token_item_id) > 0;
    const bool can_confess = rel_system.CanOfferConfession(
        rel_state,
        ctx.current_day,
        ctx.current_hour,
        ctx.cloud_system.CurrentState(),
        npc.favor,
        npc.heart_level,
        has_token);

    const bool is_target = (!rel_state.target_npc_id.empty() && rel_state.target_npc_id == npc.id);
    const bool can_schedule = (rel_state.stage == CloudSeamanor::domain::RelationshipStage::Engaged && is_target);

    if (!can_confess && !can_schedule) {
        return false;
    }

    if (ctx.dialogue_nodes == nullptr || ctx.dialogue_start_id == nullptr) {
        return false;
    }

    // 关系菜单节点：把“正常对话”作为一个入口选项，避免污染原对话树结构。
    DialogueNode menu;
    menu.id = "rel_menu_root";
    menu.speaker = npc.display_name;
    menu.text = npc.display_name + " 正看着你。你想……";

    // 继续聊天（原对话）
    menu.choices.push_back(DialogueChoice{
        .id = "rel_chat",
        .text = "继续聊天",
        .next_node_id = original_start_id
    });

    if (can_confess) {
        menu.choices.push_back(DialogueChoice{
            .id = "rel_confess",
            .text = "告白（需要 " + ItemDisplayName(rel_system.Confession().token_item_id) + "）",
            .next_node_id = "rel_action_confess"
        });
    }
    if (can_schedule) {
        menu.choices.push_back(DialogueChoice{
            .id = "rel_schedule",
            .text = "预约婚礼（-"
                + std::to_string(rel_system.Wedding().gold_cost)
                + " 金 + "
                + ItemDisplayName(rel_system.Wedding().gift_item_id)
                + " x" + std::to_string(rel_system.Wedding().gift_item_count) + "）",
            .next_node_id = "rel_action_schedule"
        });
    }

    // 注入菜单节点并改写起始节点
    ctx.dialogue_nodes->insert(ctx.dialogue_nodes->begin(), std::move(menu));
    *ctx.dialogue_start_id = "rel_menu_root";
    return true;
}

} // namespace

bool HandleGiftInteraction(PlayerInteractRuntimeContext& ctx) {
    if (ctx.spirit_beast_highlighted && ctx.highlighted_npc_index < 0) {
        auto& interaction = ctx.interaction_state;
        if (!interaction.spirit_beast_menu_open) {
            interaction.spirit_beast_menu_open = true;
            interaction.spirit_beast_menu_selection = 0;
            ctx.dialogue_text = "灵兽互动：1 喂食  2 抚摸  3 派遣（Enter/G 确认）";
            ctx.push_hint("已打开灵兽互动菜单：1喂食 2抚摸 3派遣", 2.4f);
            UpdateUiAfterInteraction(ctx);
            return true;
        }

        auto& beast = ctx.spirit_beast;
        const int selected = std::clamp(interaction.spirit_beast_menu_selection, 0, 2);
        if (selected == 0 && ctx.inventory.TryRemoveItem("Feed", 1)) {
            beast.favor += 20;
            beast.dispatched_for_pest_control = false;
            ctx.spawn_heart_particles(beast.position, ctx.heart_particles);
            ctx.push_hint("你喂食了灵兽（+20 羁绊）。", 2.4f);
            if (beast.favor >= 100) {
                ctx.push_hint("灵兽羁绊已达到满级，可派遣自动除虫。", 2.8f);
            }
        } else if (selected == 2 && beast.favor >= 60) {
            beast.dispatched_for_pest_control = true;
            ctx.push_hint("已派遣灵兽执行自动除虫。", 2.4f);
        } else {
            // 兜底分支：抚摸互动（或喂食缺饲料/派遣羁绊不足）。
            beast.favor += 5;
            ctx.spawn_heart_particles(beast.position, ctx.heart_particles);
            if (selected == 0) {
                ctx.push_hint("饲料不足，改为抚摸灵兽（+5 羁绊）。", 2.8f);
            } else if (selected == 2) {
                ctx.push_hint("羁绊不足，改为抚摸灵兽（+5 羁绊）。", 2.8f);
            } else {
                ctx.push_hint("你轻抚了灵兽（+5 羁绊）。", 2.6f);
            }
        }
        beast.favor = std::clamp(beast.favor, 0, 100);
        interaction.spirit_beast_menu_open = false;
        GlobalEventBus().Emit(Event{
            "beast_bond",
            {
                {"favor", std::to_string(beast.favor)}
            }});
        const auto HasAch_ = [&ctx](const std::string& id) -> bool {
            const auto it = ctx.achievements.find(id);
            return it != ctx.achievements.end() && it->second;
        };
        const bool lively = (beast.personality == SpiritBeastPersonality::Lively) || HasAch_("beast_type_lively");
        const bool lazy = (beast.personality == SpiritBeastPersonality::Lazy) || HasAch_("beast_type_lazy");
        const bool curious = (beast.personality == SpiritBeastPersonality::Curious) || HasAch_("beast_type_curious");
        GlobalEventBus().Emit(Event{
            "beast_type_collected",
            {
                {"lively", lively ? "1" : "0"},
                {"lazy", lazy ? "1" : "0"},
                {"curious", curious ? "1" : "0"},
            }});
        UpdateUiAfterInteraction(ctx);
        return true;
    }

    if (ctx.highlighted_npc_index < 0) {
        return false;
    }

    auto& npc = ctx.npcs[static_cast<std::size_t>(ctx.highlighted_npc_index)];
    const int gifted_today_count = static_cast<int>(std::count_if(
        ctx.npcs.begin(), ctx.npcs.end(),
        [day = ctx.current_day](const NpcActor& actor) { return actor.last_gift_day == day; }));
    if (gifted_today_count >= 3) {
        ctx.dialogue_text = "今天已经送出 3 份礼物了，明天再来吧~";
        ctx.push_hint(ctx.dialogue_text, 2.8f);
    } else if (npc.last_gift_day == ctx.current_day || npc.daily_gifted) {
        npc.daily_gifted = true;
        ctx.dialogue_text = npc.display_name + "：今天已经送过礼物了，明天再来吧~";
        ctx.push_hint(ctx.dialogue_text, 2.6f);
    } else {
        std::string gift_item_id = "TeaPack";
        if (npc.id == "yunseng" && npc.development_stage == 0 && !ctx.inventory.HasItem("TeaPack")) {
            // P9-NPC-012: 云生流浪阶段偏好食物类赠礼，允许饲料作为临时食物礼物。
            gift_item_id = "Feed";
        }
        if (!ctx.inventory.TryRemoveItem(gift_item_id, 1)) {
            if (gift_item_id == "Feed") {
                ctx.dialogue_text = "你需要 茶包 x1（或饲料 x1）才能给现在的云生送礼。";
            } else {
                ctx.dialogue_text = "你需要 茶包 x1 才能送礼。";
            }
            ctx.push_hint(ctx.dialogue_text, 2.6f);
            UpdateUiAfterInteraction(ctx);
            return true;
        }

        npc.daily_gifted = true;
        npc.last_gift_day = ctx.current_day;
        int favor_change = 1;
        std::uint32_t heart_color = kGiftHeartNeutralColor;
        if (gift_item_id == "Feed" && npc.id == "yunseng" && npc.development_stage == 0) {
            favor_change = 20;
            heart_color = kGiftHeartLikedColor;
            ctx.dialogue_text = npc.display_name + " 小心收好食物，神色放松了许多。（+20好感）";
        } else if (ContainsItem(npc.prefs.loved, "TeaPack")) {
            favor_change = 30;
            heart_color = kGiftHeartLovedColor;
            ctx.dialogue_text = npc.display_name + " 眼睛亮了起来！（+30好感）";
        } else if (ContainsItem(npc.prefs.liked, "TeaPack")) {
            favor_change = 15;
            heart_color = kGiftHeartLikedColor;
            ctx.dialogue_text = npc.display_name + " 微微点头。（+15好感）";
        } else if (ContainsItem(npc.prefs.disliked, "TeaPack")) {
            favor_change = -5;
            heart_color = kGiftHeartDislikedColor;
            ctx.dialogue_text = npc.display_name + " 不太高兴。（-5好感）";
        } else {
            favor_change = 1;
            heart_color = kGiftHeartNeutralColor;
            ctx.dialogue_text = npc.display_name + " 收到了。（+1好感）";
        }
        const int old_heart = npc.heart_level;
        const int applied_delta = ApplyNpcFavorDelta(npc, favor_change);
        if (applied_delta != favor_change && favor_change > 0) {
            ctx.dialogue_text += "（今日好感已接近上限）";
        }
        ctx.dynamic_life.AddPlayerPoints(
            npc.id, static_cast<float>(applied_delta) * GameConstants::Npc::FavorToDynamicLifeMultiplier);
        if (auto* state = ctx.dynamic_life.GetNpcState(npc.id); state && state->stage_changed_today) {
            ctx.push_hint(npc.display_name + " 进入了新的人生阶段！", 4.0f);
            ctx.log_info(npc.display_name + " 阶段跃迁：" + ctx.dynamic_life.GetStageName(state->stage));
        }
        ctx.push_hint(ctx.dialogue_text, 3.0f);
        if (npc.heart_level > old_heart) {
            ctx.push_hint(
                npc.display_name + " 好感提升了！现在 " + NpcHeartText(npc.heart_level) + " 了！",
                3.0f);
        }
        if (!npc.married && npc.heart_level >= 8 && npc.favor >= 800) {
            ctx.push_hint(
                npc.display_name + " 对你更加亲近了。或许需要一次正式的“告白”，才能推进关系终局。",
                3.2f);
        }
        const std::size_t before_hearts = ctx.heart_particles.size();
        ctx.spawn_heart_particles(npc.position, ctx.heart_particles);
        TintRecentHearts_(ctx.heart_particles, before_hearts, heart_color);
        GlobalEventBus().Emit(Event{
            "gift",
            {
                {"npc_id", npc.id},
                {"favor_delta", std::to_string(applied_delta)}
            }
        });
    }

    UpdateUiAfterInteraction(ctx);
    return true;
}

bool HandlePrimaryInteraction(PlayerInteractRuntimeContext& ctx) {
    CSM_ZONE_SCOPED;
    if (ctx.dialogue_engine.IsActive()) {
        // 对话进行中不重复触发新的交互链，避免心事件重入。
        return false;
    }

    if (ctx.highlighted_npc_index >= 0) {
        auto& npc = ctx.npcs[static_cast<std::size_t>(ctx.highlighted_npc_index)];
        if (!npc.visible) {
            ctx.push_hint("对方现在不在这里。", 2.0f);
            UpdateUiAfterInteraction(ctx);
            return true;
        }

        // BE-028 NPC 委托：优先结算“已完成待领取”的委托奖励，避免被每日对话限制挡住。
        if (ctx.try_claim_npc_delivery_rewards) {
            if (ctx.try_claim_npc_delivery_rewards(npc.id)) {
                UpdateUiAfterInteraction(ctx);
                return true;
            }
        }

        // 检查每日交谈限制（同一天同 NPC 只能触发一次完整对话）
        if (npc.last_talk_day == ctx.current_day && npc.daily_talked) {
            ctx.dialogue_text = npc.display_name + "：今天已经聊过了，改天再来吧~";
            ctx.push_hint("今天已经和 " + npc.display_name + " 交谈过了。", 2.0f);
            UpdateUiAfterInteraction(ctx);
            return true;
        }

        // 使用 NpcDialogueManager 生成动态对话
        bool dialogue_started = false;
        if (ctx.dialogue_manager && ctx.dialogue_nodes && ctx.dialogue_start_id) {
            NpcDialogueContext dl_ctx = BuildNpcDialogueContext(
                npc,
                ctx.game_clock,
                ctx.cloud_system.CurrentState(),
                ctx.player_name,
                ctx.farm_name);

            // 心事件优先于日常对话
            bool is_heart_event = false;
            if (const HeartEventEntry* evt = ctx.dialogue_manager->CheckHeartEventTrigger(npc.id, dl_ctx)) {
                auto nodes = ctx.dialogue_manager->LoadHeartEventDialogue(npc.id, evt->heart_threshold);
                if (!nodes.empty()) {
                    *ctx.dialogue_nodes = nodes;
                    *ctx.dialogue_start_id = nodes.front().id;
                    // 记录心事件信息用于完成后结算
                    ctx.current_heart_event_id = evt->event_id;
                    ctx.current_heart_event_reward = evt->reward_favor;
                    ctx.current_heart_event_flag = evt->reward_flag;
                    ctx.interaction_state.current_heart_event_id = evt->event_id;
                    ctx.interaction_state.current_heart_event_reward = evt->reward_favor;
                    ctx.interaction_state.current_heart_event_flag = evt->reward_flag;
                    is_heart_event = true;
                }
            } else {
                auto nodes = ctx.dialogue_manager->SelectDailyDialogue(npc.id, dl_ctx);
                if (!nodes.empty()) {
                    *ctx.dialogue_nodes = nodes;
                    *ctx.dialogue_start_id = nodes.front().id;
                }
            }

            // 如果有对话节点，启动 DialogueEngine 并挂载到 PixelGameHud
            if (!ctx.dialogue_nodes->empty() && !ctx.dialogue_start_id->empty()) {
                auto& engine = ctx.dialogue_engine;
                engine.SetDataRoot(ctx.dialogue_data_root);
                DialogueContext dlg_ctx;
                dlg_ctx.player_name = ctx.player_name;
                dlg_ctx.farm_name = ctx.farm_name;
                dlg_ctx.npc_name = npc.display_name;
                dlg_ctx.player_favor = npc.favor;
                dlg_ctx.current_day = ctx.game_clock.Day();
                dlg_ctx.current_season =
                    CloudSeamanor::domain::GameClock::SeasonName(ctx.game_clock.Season());
                dlg_ctx.current_weather = ctx.cloud_system.CurrentStateText();
                dlg_ctx.item_name = ctx.inventory.HasItem("TeaPack") ? "茶包" : "";
                dlg_ctx.has_item = ctx.inventory.HasItem("TeaPack");
                engine.SetCallbacks(DialogueCallbacks{
                    .on_text_update = [&ctx](const std::string& partial_text) {
                        ctx.dialogue_text = partial_text;
                        ctx.update_ui();
                    },
                    .on_node_change = [&ctx, &npc](const DialogueNode& node) {
                        // 关系动作节点：在进入节点时执行副作用，然后由节点文本承载反馈。
                        if (node.id == "rel_action_confess") {
                            const auto result = ctx.relationship_system.TryConfess(
                                ctx.relationship_state,
                                ctx.current_day,
                                npc.favor,
                                npc.heart_level);
                            if (result == CloudSeamanor::domain::ConfessionResult::Accepted) {
                                ctx.relationship_state.target_npc_id = npc.id;
                                (void)ctx.inventory.TryRemoveItem(ctx.relationship_system.Confession().token_item_id, 1);
                                ctx.push_hint("【告白】" + npc.display_name + " 接受了你的心意。你们订婚了。", 3.6f);
                            } else if (result == CloudSeamanor::domain::ConfessionResult::Rejected) {
                                ctx.relationship_state.confession_cooldown_until_day =
                                    ctx.current_day + ctx.relationship_system.Confession().cooldown_days_on_fail;
                                const int penalty = -ctx.relationship_system.Confession().favor_penalty_on_fail;
                                (void)ApplyNpcFavorDelta(npc, penalty);
                                ctx.push_hint("【告白】对方婉拒了。需要一些时间平复。", 3.2f);
                            } else {
                                ctx.push_hint("现在还不适合告白。", 2.2f);
                            }
                        } else if (node.id == "rel_action_schedule") {
                            const int tea_pack_count = static_cast<int>(ctx.inventory.CountOf(
                                ctx.relationship_system.Wedding().gift_item_id));
                            const int wedding_day = ctx.relationship_system.TryScheduleWedding(
                                ctx.relationship_state,
                                ctx.current_day,
                                ctx.gold,
                                tea_pack_count,
                                ctx.festivals);
                            if (wedding_day > 0) {
                                ctx.gold -= ctx.relationship_system.Wedding().gold_cost;
                                (void)ctx.inventory.TryRemoveItem(
                                    ctx.relationship_system.Wedding().gift_item_id,
                                    ctx.relationship_system.Wedding().gift_item_count);
                                npc.memory_tag = "wedding_scheduled";
                                npc.memory_until_day = wedding_day + 7;
                                ctx.push_hint("【婚礼】已预约：第 " + std::to_string(wedding_day) + " 天。", 3.2f);
                            } else {
                                ctx.push_hint("预约婚礼失败：金币或物品不足，或当前状态不允许。", 2.6f);
                            }
                        }
                    },
                    .on_choices_change = [](const std::vector<DialogueChoice>&) {},
                    .on_complete = [&ctx, &npc, is_heart_event]() {
                        if (is_heart_event && !ctx.current_heart_event_id.empty()) {
                            if (ctx.dialogue_manager) {
                                ctx.dialogue_manager->MarkHeartEventComplete(npc.id, ctx.current_heart_event_id);
                            }
                            int reward = ctx.current_heart_event_reward;
                            if (reward > 0) {
                                ApplyNpcFavorDelta(npc, reward);
                                ctx.push_hint(npc.display_name + " 的心事件已完成！获得好感 +"
                                    + std::to_string(reward) + "。", 3.5f);
                            }
                            npc.memory_tag = "heart_event_done";
                            npc.memory_until_day = ctx.current_day + 10;
                            CloudSeamanor::infrastructure::Logger::LogNpcHeartEvent(
                                "npc=" + npc.id + ", event=" + ctx.current_heart_event_id
                                + ", reward_favor=" + std::to_string(reward));
                            if (!ctx.current_heart_event_flag.empty()) {
                                ctx.log_info("心事件奖励标记已激活：" + ctx.current_heart_event_flag);
                            }
                            ctx.current_heart_event_id.clear();
                            ctx.current_heart_event_reward = 0;
                            ctx.current_heart_event_flag.clear();
                            ctx.interaction_state.current_heart_event_id.clear();
                            ctx.interaction_state.current_heart_event_reward = 0;
                            ctx.interaction_state.current_heart_event_flag.clear();
                        }
                        ctx.dialogue_text = "对话结束了。";
                        ctx.update_ui();
                    },
                    .on_favor_change = nullptr
                });
                if (!is_heart_event) {
                    (void)BuildRelationshipDialogueMenu_(ctx, npc, *ctx.dialogue_start_id);
                    // 注入后，起始节点可能被改写，需要同步 npc_name。
                    dlg_ctx.npc_name = npc.display_name;
                }

                // 若存在关系入口，则确保动作节点存在（在节点进入时执行副作用）
                if (!is_heart_event && ctx.dialogue_nodes != nullptr) {
                    const std::string confess_text = "你取出信物，想说的话在喉间滚了滚……\n"
                        "“我喜欢你。愿意和我一起走下去吗？”";
                    ctx.dialogue_nodes->push_back(DialogueNode{
                        .id = "rel_action_confess",
                        .speaker = npc.display_name,
                        .text = confess_text,
                        .choices = {}
                    });
                    const std::string schedule_text = "你与 " + npc.display_name
                        + " 商量着婚礼的日子，并约定在那天相见。";
                    ctx.dialogue_nodes->push_back(DialogueNode{
                        .id = "rel_action_schedule",
                        .speaker = npc.display_name,
                        .text = schedule_text,
                        .choices = {}
                    });
                }

                engine.StartDialogue(*ctx.dialogue_nodes, *ctx.dialogue_start_id, dlg_ctx);
                dialogue_started = true;
                npc.daily_talked = true;
                npc.last_talk_day = ctx.current_day;
            }
        }

        // DialogueEngine 启动后，通过引擎回调驱动，不再覆盖 dialogue_text
        if (dialogue_started) {
            const int old_heart = npc.heart_level;
            const int applied_delta = ApplyNpcFavorDelta(npc, 2);
            if (npc.memory_until_day < ctx.current_day) {
                npc.memory_tag.clear();
                npc.memory_until_day = 0;
            }
            ctx.push_hint("与" + npc.display_name + "交谈... 关系更近了一步。（+" + std::to_string(applied_delta) + "）", 2.8f);
            if (npc.heart_level > old_heart) {
                ctx.push_hint(npc.display_name + " 对你的感情加深了！", 2.8f);
            }
            UpdateUiAfterInteraction(ctx);
            return true;
        }

        // 生成简短描述用于 HUD 显示（无对话数据时降级）
        const int hour = ctx.game_clock.Hour();
        if (hour >= 6 && hour < 12) {
            ctx.dialogue_text = npc.display_name + "：早安，今天的云海很适合慢慢做事。";
        } else if (hour >= 12 && hour < 18) {
            ctx.dialogue_text = npc.display_name + "：午后风正好，要不要一起喝杯茶？";
        } else if (hour >= 18 && hour < 22) {
            ctx.dialogue_text = npc.display_name + "：傍晚啦，今天过得顺利吗？";
        } else {
            ctx.dialogue_text = npc.display_name + "：夜深了，早点休息，明天再聊吧。";
        }
        if (npc.memory_until_day >= ctx.current_day && !npc.memory_tag.empty()) {
            if (npc.memory_tag == "heart_event_done") {
                ctx.dialogue_text += "\n“上次那件事，我一直记在心里。”";
            } else if (npc.memory_tag == "wedding_scheduled") {
                ctx.dialogue_text += "\n“婚礼筹备别太累，我会一直在。”";
            }
        }
        ctx.dialogue_text += "\n（地点：" +
            LocationDisplayName(ctx.npc_text_mappings, npc.current_location) +
            "，状态：" + ActivityDisplayName(ctx.npc_text_mappings, npc.current_activity) + "）";

        const int old_heart = npc.heart_level;
        const int applied_delta = ApplyNpcFavorDelta(npc, 2);
        ctx.push_hint("与" + npc.display_name + "交谈... 关系更近了一步。（+" + std::to_string(applied_delta) + "）", 2.8f);
        if (npc.heart_level > old_heart) {
            ctx.push_hint(npc.display_name + " 对你的感情加深了！", 2.8f);
        }
        UpdateUiAfterInteraction(ctx);
        return true;
    }

    if (ctx.spirit_beast_highlighted) {
        if (!ctx.spirit_beast.daily_interacted) {
            ctx.spirit_beast.daily_interacted = true;
            ctx.spirit_beast.last_interaction_day = ctx.current_day;
            ctx.spirit_beast.state = SpiritBeastState::Interact;
            ctx.spirit_beast.interact_timer = GameConstants::SpiritBeast::InteractDuration;
            ctx.spawn_heart_particles(ctx.spirit_beast.position, ctx.heart_particles);
            ctx.push_hint("你和灵兽建立了新的羁绊。它今天的协助能力已经激活。", 2.8f);
            if (ctx.inventory.TryRemoveItem("spirit_grass", 1)) {
                ctx.inventory.AddItem("spirit_dust", 2);
                ctx.push_hint("你喂食了灵兽，获得灵尘回礼 x2。", 2.2f);
            } else if (ctx.inventory.TryRemoveItem("mist_carp", 1)
                       || ctx.inventory.TryRemoveItem("cloud_koi", 1)
                       || ctx.inventory.TryRemoveItem("moon_silverfish", 1)
                       || ctx.inventory.TryRemoveItem("tea_shrimp", 1)
                       || ctx.inventory.TryRemoveItem("tide_eel", 1)) {
                ctx.spirit_beast.favor += 8;
                ctx.push_hint("你用鱼获喂了灵兽，它显得格外亲近。", 2.2f);
            }
            ctx.log_info("你轻轻抚摸了灵兽。");
        } else {
            ctx.push_hint("今天已经和灵兽结缘过了。", 2.2f);
            ctx.log_info("今天已经和灵兽结缘过了。");
        }
        UpdateUiAfterInteraction(ctx);
        return true;
    }

    if (ctx.highlighted_plot_index >= 0) {
        auto& plot = ctx.tea_plots[static_cast<std::size_t>(ctx.highlighted_plot_index)];
        if (!plot.cleared) {
            // BE-013：障碍物清除（石头/树桩/杂草）+ 工具耐久扣减
            auto ConsumeToolDurability = [&](const char* tool_id) -> bool {
                if (ctx.inventory.CountOf(tool_id) <= 0) {
                    ctx.push_hint(std::string("工具已损坏：") + ItemDisplayName(tool_id) + " 为 0。", 2.4f);
                    return false;
                }
                (void)ctx.inventory.TryRemoveItem(tool_id, 1);
                return true;
            };

            bool cleared_now = false;
            switch (plot.obstacle_type) {
            case PlotObstacleType::Stone: {
                if (!ConsumeToolDurability("ToolPickaxe")) break;
                const int hit_power = ObstacleHitPowerFromEfficiency_(
                    GetBestPickaxeObstacleEfficiency_(ctx.inventory));
                plot.obstacle_hits_left = std::max(0, plot.obstacle_hits_left - hit_power);
                if (plot.obstacle_hits_left <= 0) {
                    cleared_now = true;
                } else {
                    ctx.push_hint(
                        "你敲击石头。剩余 " + std::to_string(plot.obstacle_hits_left)
                            + " 次（镐子效率 x" + std::to_string(hit_power) + "）。",
                        2.2f);
                }
                break;
            }
            case PlotObstacleType::Stump: {
                if (!ConsumeToolDurability("ToolAxe")) break;
                const int hit_power = ObstacleHitPowerFromEfficiency_(
                    GetBestAxeObstacleEfficiency_(ctx.inventory));
                plot.obstacle_hits_left = std::max(0, plot.obstacle_hits_left - hit_power);
                if (plot.obstacle_hits_left <= 0) {
                    cleared_now = true;
                } else {
                    ctx.push_hint(
                        "你砍向树桩。剩余 " + std::to_string(plot.obstacle_hits_left)
                            + " 次（斧头效率 x" + std::to_string(hit_power) + "）。",
                        2.2f);
                }
                break;
            }
            case PlotObstacleType::Weed: {
                if (!ConsumeToolDurability("ToolSickle")) break;
                plot.obstacle_hits_left = 0;
                cleared_now = true;
                break;
            }
            case PlotObstacleType::None:
            default:
                ctx.push_hint("这块地还未开垦。", 2.2f);
                break;
            }

            if (cleared_now) {
                plot.cleared = true;
                plot.obstacle_type = PlotObstacleType::None;
                plot.obstacle_hits_left = 0;
                plot.tilled = false;
                plot.seeded = false;
                plot.watered = false;
                plot.ready = false;
                plot.growth = 0.0f;
                plot.stage = 0;
                ctx.push_hint("障碍物已清除：现在可以翻土种植了。", 2.6f);
            }
            ctx.refresh_plot_visual(plot, true);
            UpdateUiAfterInteraction(ctx);
            return true;
        }
        if ((plot.disease || plot.pest) && ctx.inventory.TryRemoveItem("PesticideItem", 1)) {
            plot.disease = false;
            plot.pest = false;
            plot.disease_days = 0;
            ctx.push_hint("已完成病虫害处理，地块恢复健康。", 2.4f);
            ctx.refresh_plot_visual(plot, true);
            UpdateUiAfterInteraction(ctx);
            return true;
        }
        if (!plot.tilled) {
            if (ctx.inventory.CountOf("ToolHoe") <= 0) {
                ctx.push_hint("锄头耐久为 0，无法翻土。", 2.4f);
                UpdateUiAfterInteraction(ctx);
                return true;
            }
            (void)ctx.inventory.TryRemoveItem("ToolHoe", 1);
            plot.tilled = true;
            ctx.push_hint(plot.crop_name + " 地块已翻土。下一步：播种。", 2.4f);
            ctx.log_info(plot.crop_name + " 地块已翻土。");
        } else if (!plot.sprinkler_installed && ctx.inventory.TryRemoveItem("SprinklerItem", 1)) {
            plot.sprinkler_installed = true;
            plot.sprinkler_days_left = 30;
            ctx.push_hint("已放置洒水器（持续 30 天）。", 2.6f);
            ctx.log_info("放置洒水器成功。");
        } else if (!plot.fertilized && ctx.inventory.TryRemoveItem("FertilizerItem", 1)) {
            plot.fertilized = true;
            plot.fertilizer_type = "basic";
            ctx.push_hint("已施加普通肥料（生长速度 +20%）。", 2.6f);
            ctx.log_info("施肥成功。");
        } else if (!plot.fertilized && ctx.inventory.TryRemoveItem("premium_fertilizer", 1)) {
            plot.fertilized = true;
            plot.fertilizer_type = "premium";
            ctx.push_hint("已施加优质肥料（生长速度 +50%，品质 +1 档）。", 2.6f);
            ctx.log_info("优质施肥成功。");
        } else if (!plot.fertilized && ctx.inventory.TryRemoveItem("spirit_fertilizer", 1)) {
            plot.fertilized = true;
            plot.fertilizer_type = "spirit";
            ctx.push_hint("已施加灵肥（生长速度 +80%，品质 +2 档）。", 2.6f);
            ctx.log_info("灵肥施肥成功。");
        } else if (!plot.fertilized && ctx.inventory.TryRemoveItem("cloud_fertilizer", 1)) {
            plot.fertilized = true;
            plot.fertilizer_type = "cloud_essence";
            ctx.push_hint("已施加云华肥（生长速度 +100%，品质 +3 档）。", 2.6f);
            ctx.log_info("云华肥施肥成功。");
        } else if (!plot.fertilized && ctx.inventory.TryRemoveItem("tea_soul_fertilizer", 1)) {
            plot.fertilized = true;
            plot.fertilizer_type = "tea_soul";
            ctx.push_hint("已施加茶魂肥（生长速度 +120%，品质 +2 档+茶魂加成）。", 2.6f);
            ctx.log_info("茶魂肥施肥成功。");
        } else if (!plot.seeded) {
            const bool is_tea_seed = IsTeaSeedItemId_(plot.seed_item_id);
            if (plot.layer == TeaPlotLayer::TeaGardenExclusive && !is_tea_seed) {
                ctx.push_hint("该地块为茶园专属地块，只能种植灵茶类作物。", 2.6f);
                UpdateUiAfterInteraction(ctx);
                return true;
            }
            if (plot.layer == TeaPlotLayer::NormalFarm && is_tea_seed) {
                ctx.push_hint("该地块为普通农田，请前往茶园专属地块种植灵茶。", 2.6f);
                UpdateUiAfterInteraction(ctx);
                return true;
            }
            if (ctx.inventory.TryRemoveItem(plot.seed_item_id, 1)) {
                plot.seeded = true;
                plot.in_greenhouse = ctx.greenhouse_tag_next_planting;
                plot.growth = 0.0f;
                plot.stage = 1;
                if (ctx.play_sfx) ctx.play_sfx("plant");
                if (plot.in_greenhouse) {
                    ctx.push_hint(plot.crop_name + " 已播种（温室地块）。给它浇水后就会开始生长。", 2.6f);
                } else {
                    ctx.push_hint(plot.crop_name + " 已播种。给它浇水后就会开始生长。", 2.6f);
                }
                ctx.log_info(plot.crop_name + " 已播种。");
            } else {
                ctx.push_hint("缺少种子：" + ItemDisplayName(plot.seed_item_id) + "。", 2.6f);
                ctx.log_info("缺少种子：" + plot.seed_item_id);
            }
        } else if (!plot.watered) {
            plot.watered = true;
            if (ctx.play_sfx) ctx.play_sfx("water");
            ctx.push_hint(plot.crop_name + " 已浇水。当前云海只会提供正向加成，可以安心等它成长。", 3.0f);
            ctx.log_info(plot.crop_name + " 地块已浇水。");
        } else if (plot.ready) {
            const auto harvest_quality = CloudSeamanor::domain::CropTable::CalculateQuality(
                ctx.cloud_system.CurrentState(),
                false);
            const float cloud_density = ctx.cloud_system.CurrentSpiritDensity();
            const float tea_buff = 1.0f + ctx.skills.GetBonus(CloudSeamanor::domain::SkillType::SpiritFarm)
                * GameConstants::Skill::SkillBonusToBuffRatio;
            const float beast_share = ctx.spirit_beast.daily_interacted
                ? GameConstants::SpiritBeast::AssistHarvestMultiplier : 1.0f;
            if (ctx.skills.AddExp(CloudSeamanor::domain::SkillType::SpiritFarm,
                    GameConstants::Skill::SpiritFarmExpBase, cloud_density, tea_buff, beast_share)) {
                const int new_level = ctx.skills.GetLevel(CloudSeamanor::domain::SkillType::SpiritFarm);
                ctx.on_skill_level_up(CloudSeamanor::domain::SkillType::SpiritFarm, new_level);
                ctx.push_hint("灵农技能提升至 Lv." + std::to_string(new_level) + "！加成效果增强。", 3.2f);
                ctx.log_info("灵农技能升级至 Lv." + std::to_string(new_level) + "！");
            }
            const float quality_mult = CloudSeamanor::domain::CropTable::QualityHarvestMultiplier(harvest_quality);
            // 镰刀收割效率加成
            const float sickle_efficiency = GetBestSickleCollectEfficiency_(ctx.inventory);
            int actual_amount = std::max(1, static_cast<int>(
                static_cast<float>(plot.harvest_amount) * quality_mult * sickle_efficiency));
            if (plot.disease_days >= 3) {
                actual_amount = 0;
            }
            const auto quality_text = std::string(CloudSeamanor::domain::CropTable::QualityToText(harvest_quality));
            if (actual_amount > 0) {
                const auto add_result = ctx.inventory.TryAddItem(plot.harvest_item_id, actual_amount);
                if (!add_result) {
                    ctx.push_hint("背包已满，无法收获更多作物", 2.8f);
                    UpdateUiAfterInteraction(ctx);
                    return true;
                }
            }
            plot.seeded = false;
            plot.watered = false;
            plot.ready = false;
            plot.growth = 0.0f;
            plot.stage = 0;
            plot.disease = false;
            plot.pest = false;
            plot.disease_days = 0;
            ctx.last_trade_quality = harvest_quality;
            if (actual_amount > 0) {
                if (ctx.play_sfx) ctx.play_sfx("harvest");
                const std::string sickle_hint = (sickle_efficiency > 1.0f)
                    ? "（镰刀加成 +" + std::to_string(static_cast<int>((sickle_efficiency - 1.0f) * 100)) + "%）"
                    : "";
                ctx.push_hint("已收获 " + quality_text + " " + ItemDisplayName(plot.harvest_item_id) + " x"
                    + std::to_string(actual_amount) + sickle_hint + "。", 2.8f);
                ctx.log_info(plot.crop_name + " 已收获（品质：" + quality_text + "，数量：" + std::to_string(actual_amount) + sickle_hint + "）。");
                GlobalEventBus().Emit(Event{
                    "harvest",
                    {
                        {"item_id", plot.harvest_item_id},
                        {"count", std::to_string(actual_amount)}
                    }
                });
            } else {
                ctx.push_hint("病虫害未及时处理，本轮作物减产为 0。", 2.8f);
                ctx.log_info(plot.crop_name + " 因病虫害未处理导致本轮减产为 0。");
            }
        } else {
            ctx.push_hint(plot.crop_name + " 还在生长中。" + PlotStatusText(plot) + "。", 2.2f);
            ctx.log_info(plot.crop_name + " 仍在生长中。");
        }
        ctx.refresh_plot_visual(plot, true);
        UpdateUiAfterInteraction(ctx);
        return true;
    }

    if (ctx.highlighted_index >= 0 && ctx.stamina.Current() >= ctx.stamina_interact_cost) {
        const auto& target = ctx.interactables[static_cast<std::size_t>(ctx.highlighted_index)];
        ctx.stamina.Consume(ctx.stamina_interact_cost);

        if (target.Label() == "Tea Bush") {
            auto bush_it = std::find_if(
                ctx.tea_bushes.begin(),
                ctx.tea_bushes.end(),
                [&](const CloudSeamanor::domain::TeaBush& b) {
                    return b.id == target.EnemyId();
                });
            if (bush_it == ctx.tea_bushes.end()) {
                ctx.push_hint("茶灌木数据未找到。", 2.0f);
                UpdateUiAfterInteraction(ctx);
                return true;
            }
            if (!bush_it->CanHarvest(ctx.current_day)) {
                ctx.push_hint(
                    "茶灌木尚未成熟，还需 " + std::to_string(bush_it->DaysUntilHarvest(ctx.current_day)) + " 天。",
                    2.2f);
                UpdateUiAfterInteraction(ctx);
                return true;
            }
            const auto quality = CloudSeamanor::domain::CalculateTeaBushQuality(
                *bush_it, ctx.cloud_system, CloudSeamanor::domain::CropQuality::Normal);
            const int yield = std::max(1, 1 + static_cast<int>(quality) / 2);
            const std::string item_id = bush_it->tea_id + "_leaf";
            ctx.inventory.AddItem(item_id, yield);
            bush_it->last_harvest_day = ctx.current_day;
            ctx.push_hint("采摘成功：" + ItemDisplayName(item_id) + " x" + std::to_string(yield), 2.2f);
            UpdateUiAfterInteraction(ctx);
            return true;
        }

        if (target.Label() == "Spirit Gateway") {
            const bool to_spirit_realm = !ctx.in_spirit_realm;
            if (ctx.request_spirit_realm_travel) {
                ctx.request_spirit_realm_travel(to_spirit_realm);
                ctx.log_info("触发灵界入口交互：请求过场传送。");
            } else {
                // 兜底：没有注入过场逻辑时仍允许切换标记
                ctx.in_spirit_realm = to_spirit_realm;
            }
            UpdateUiAfterInteraction(ctx);
            return true;
        }
        if (target.Label() == "Spirit Gateway Return") {
            if (ctx.in_spirit_realm) {
                if (ctx.request_spirit_realm_travel) {
                    ctx.request_spirit_realm_travel(false);
                } else {
                    ctx.in_spirit_realm = false;
                }
                ctx.push_hint("你通过返程传送门回到了主世界。", 2.4f);
            } else {
                ctx.push_hint("这里是灵界返程门。", 1.8f);
            }
            UpdateUiAfterInteraction(ctx);
            return true;
        }
        if (target.Label() == "Spirit Plant") {
            const std::string cooldown_key = target.Label() + "_" + std::to_string(ctx.highlighted_index);
            const auto it = ctx.spirit_plant_last_harvest_hour.find(cooldown_key);
            if (it != ctx.spirit_plant_last_harvest_hour.end()
                && (ctx.current_game_hour - it->second) < 1) {
                ctx.push_hint("灵草还未恢复，请稍后再来。", 2.6f);
                UpdateUiAfterInteraction(ctx);
                return true;
            }
            int amount = 1;
            std::string item_id = "spirit_grass";
            if (ctx.cloud_system.CurrentState() == CloudSeamanor::domain::CloudState::Tide) {
                amount = 2; // A-24: 大潮掉落翻倍
                item_id = "cloud_dew";
            }
            if (ctx.festival_runtime_state != nullptr
                && ctx.festival_runtime_state->qingming_double_spirit_gather_day == ctx.current_day) {
                amount *= 2;
            }
            ctx.inventory.AddItem(item_id, amount);
            ctx.spirit_plant_last_harvest_hour[cooldown_key] = ctx.current_game_hour;
            ctx.push_hint("采集到 " + ItemDisplayName(item_id) + " x" + std::to_string(amount), 2.8f);
            ctx.log_info("灵物采集成功。");
            UpdateUiAfterInteraction(ctx);
            return true;
        }
        if (target.Label() == "Spirit Plant Rare") {
            const std::string cooldown_key = target.Label() + "_" + std::to_string(ctx.highlighted_index);
            const auto it = ctx.spirit_plant_last_harvest_hour.find(cooldown_key);
            if (it != ctx.spirit_plant_last_harvest_hour.end()
                && (ctx.current_game_hour - it->second) < 2) {
                ctx.push_hint("稀有灵草尚未恢复。", 2.6f);
                UpdateUiAfterInteraction(ctx);
                return true;
            }
            const std::string item_id =
                (ctx.cloud_system.CurrentState() == CloudSeamanor::domain::CloudState::Tide)
                ? "star_fragment" : "spirit_dust";
            int amount =
                (ctx.cloud_system.CurrentState() == CloudSeamanor::domain::CloudState::Tide) ? 1 : 2;
            if (ctx.festival_runtime_state != nullptr
                && ctx.festival_runtime_state->qingming_double_spirit_gather_day == ctx.current_day) {
                amount *= 2;
            }
            ctx.inventory.AddItem(item_id, amount);
            ctx.spirit_plant_last_harvest_hour[cooldown_key] = ctx.current_game_hour;
            ctx.push_hint("采集到稀有灵物 " + ItemDisplayName(item_id) + " x" + std::to_string(amount), 2.8f);
            UpdateUiAfterInteraction(ctx);
            return true;
        }
        if (target.Label() == "Shop Stall") {
            auto& interaction = ctx.interaction_state;
            const auto pet_options = CollectPetCommodityOptions_(ctx.price_table, "shop");
            const bool can_use_pet_menu = !pet_options.empty();
            const PriceTableEntry* selected = nullptr;
            if (can_use_pet_menu && !interaction.pet_shop_menu_open) {
                interaction.pet_shop_menu_open = true;
                interaction.pet_shop_menu_selection = 0;
                ctx.dialogue_text = "宠物选购：1 猫  2 狗  3 鸟（Enter/E 确认）";
                ctx.push_hint("已打开宠物选购：1猫 2狗 3鸟", 2.4f);
                UpdateUiAfterInteraction(ctx);
                return true;
            }
            if (can_use_pet_menu) {
                const int idx = std::clamp(
                    interaction.pet_shop_menu_selection, 0, static_cast<int>(pet_options.size()) - 1);
                selected = pet_options[static_cast<std::size_t>(idx)];
            } else {
                selected = kShopSystem.SelectShopStallItem(ctx.price_table, ctx.pet_adopted);
            }
            if (!selected) {
                ctx.push_hint("商店价格表为空。", 2.2f);
            } else if (!kShopSystem.CanPurchaseByWeeklyLimit(*selected, ctx.weekly_buy_count)) {
                ctx.push_hint(
                    "本周已达到该商品购买上限（"
                    + std::to_string(kShopSystem.WeeklyPurchaseLimitPerItem()) + "）。",
                    2.6f);
            } else if (kShopSystem.TryPurchase(
                           *selected,
                           ctx.gold,
                           ctx.inventory,
                           ctx.weekly_buy_count,
                           [&](const PriceTableEntry& entry) {
                               return HandlePetCommodityPurchase_(ctx, entry, "shop");
                           })) {
                interaction.pet_shop_menu_open = false;
                if (ctx.play_sfx) ctx.play_sfx("shop_purchase");
                ctx.push_hint("商店购买：" + ItemDisplayName(selected->item_id) + " x1（-"
                    + std::to_string(selected->buy_price) + " 金）。", 2.6f);
            } else {
                ctx.push_hint("金币不足，无法购买。", 2.2f);
            }
            UpdateUiAfterInteraction(ctx);
            return true;
        }
        if (target.Label() == "Purchaser") {
            auto& interaction = ctx.interaction_state;
            const auto sellable_items = kShopSystem.CollectSellableBySource(
                ctx.price_table, "purchaser", ctx.inventory);
            if (sellable_items.empty()) {
                ctx.push_hint("没有可出售物品。", 2.2f);
                interaction.purchaser_menu_open = false;
                UpdateUiAfterInteraction(ctx);
                return true;
            }

            if (!interaction.purchaser_menu_open) {
                interaction.purchaser_menu_open = true;
                interaction.purchaser_menu_selection = 0;
                const std::size_t preview_count = std::min<std::size_t>(3, sellable_items.size());
                std::string preview = "收购菜单：";
                for (std::size_t i = 0; i < preview_count; ++i) {
                    if (i > 0) {
                        preview += "  ";
                    }
                    preview += std::to_string(i + 1) + "." + ItemDisplayName(sellable_items[i]->item_id);
                }
                preview += "（数字键选择，Enter/E确认）";
                ctx.push_hint(preview, 3.4f);
                UpdateUiAfterInteraction(ctx);
                return true;
            }

            const int selected_index = std::clamp(
                interaction.purchaser_menu_selection, 0, static_cast<int>(sellable_items.size()) - 1);
            const PriceTableEntry* sell_entry = sellable_items[static_cast<std::size_t>(selected_index)];
            int income = 0;
            if (sell_entry && kShopSystem.TrySellOne(
                                  *sell_entry,
                                  ctx.last_trade_quality,
                                  ctx.inventory,
                                  ctx.gold,
                                  ctx.weekly_sell_count,
                                  income)) {
                if (ctx.festival_runtime_state != nullptr
                    && ctx.festival_runtime_state->harvest_festival_sell_bonus_day == ctx.current_day) {
                    const int extra = std::max(1, income * 15 / 100);
                    ctx.gold += extra;
                    ctx.push_hint("【丰收祭】出售加成 +" + std::to_string(extra) + " 金。", 1.8f);
                }
                ctx.push_hint("收购商买下了 " + ItemDisplayName(sell_entry->item_id)
                    + "，获得 " + std::to_string(income) + " 金。", 2.6f);
                interaction.purchaser_menu_open = false;
            } else {
                ctx.push_hint("没有可出售物品。", 2.2f);
                interaction.purchaser_menu_open = false;
            }
            UpdateUiAfterInteraction(ctx);
            return true;
        }
        if (target.Label() == "Mailbox") {
            const PriceTableEntry* selected = FindPrice_(ctx.price_table, "TeaSeed");
            if (!selected || selected->buy_price <= 0) {
                ctx.push_hint("邮购价格配置缺失。", 2.2f);
            } else if (ctx.gold >= selected->buy_price) {
                ctx.gold -= selected->buy_price;
                ctx.mail_orders.push_back(MailOrderEntry{
                    selected->item_id, 1, ctx.current_day + 1
                });
                if (ctx.play_sfx) ctx.play_sfx("shop_purchase");
                ctx.push_hint("邮购下单成功：" + ItemDisplayName(selected->item_id)
                    + " 将在次日送达。", 2.8f);
            } else {
                ctx.push_hint("金币不足，无法邮购。", 2.2f);
            }
            UpdateUiAfterInteraction(ctx);
            return true;
        }
        if (target.Label() == "Festival Booth") {
            (void)HandleFestivalBooth_(ctx);
            UpdateUiAfterInteraction(ctx);
            return true;
        }
        if (target.Label() == "General Store") {
            const PriceTableEntry* selected = kShopSystem.SelectGeneralStoreItem(
                ctx.price_table, ctx.daily_general_store_stock);
            if (!selected) {
                ctx.push_hint("今日杂货店已售罄。", 2.2f);
            } else if (!kShopSystem.CanPurchaseByWeeklyLimit(*selected, ctx.weekly_buy_count)) {
                ctx.push_hint(
                    "本周已达到该商品购买上限（"
                    + std::to_string(kShopSystem.WeeklyPurchaseLimitPerItem()) + "）。",
                    2.6f);
            } else if (kShopSystem.TryPurchase(
                           *selected,
                           ctx.gold,
                           ctx.inventory,
                           ctx.weekly_buy_count,
                           [&](const PriceTableEntry& entry) {
                               return HandlePetCommodityPurchase_(ctx, entry, "general_store");
                           })) {
                if (ctx.play_sfx) ctx.play_sfx("shop_purchase");
                ctx.push_hint("杂货店购入：" + ItemDisplayName(selected->item_id), 2.2f);
            } else {
                ctx.push_hint("金币不足。", 2.2f);
            }
            UpdateUiAfterInteraction(ctx);
            return true;
        }
        if (target.Label() == "Tide Shop") {
            if (!ctx.active_festival_id.empty()) {
                const auto offer_ids = FestivalOfferItemIds_(ctx.active_festival_id);
                const PriceTableEntry* selected = FindFestivalOffer_(ctx.price_table, offer_ids, ctx.current_day);
                if (!selected) {
                    ctx.push_hint("节日商店暂时无货，请稍后再来。", 2.2f);
                    UpdateUiAfterInteraction(ctx);
                    return true;
                }

                const std::string daily_limit_key = "festival_shop_day_" + std::to_string(ctx.current_day)
                    + "_" + selected->item_id;
                if (ctx.weekly_buy_count[daily_limit_key] > 0) {
                    ctx.push_hint("该节日商品今日已售罄。", 2.2f);
                    UpdateUiAfterInteraction(ctx);
                    return true;
                }
                if (!kShopSystem.CanPurchaseByWeeklyLimit(*selected, ctx.weekly_buy_count)) {
                    ctx.push_hint(
                        "本周已达到该商品购买上限（"
                        + std::to_string(kShopSystem.WeeklyPurchaseLimitPerItem()) + "）。",
                        2.6f);
                    UpdateUiAfterInteraction(ctx);
                    return true;
                }

                if (kShopSystem.TryPurchase(
                        *selected,
                        ctx.gold,
                        ctx.inventory,
                        ctx.weekly_buy_count,
                        nullptr)) {
                    ctx.weekly_buy_count[daily_limit_key] += 1;
                    if (ctx.play_sfx) ctx.play_sfx("shop_purchase");
                    ctx.push_hint(
                        "节日商店购入：" + ItemDisplayName(selected->item_id) + "（" + ctx.active_festival_id + " 限定）",
                        2.6f);
                    GlobalEventBus().Emit(Event{
                        "FestivalShopPurchaseEvent",
                        {
                            {"festival_id", ctx.active_festival_id},
                            {"item_id", selected->item_id},
                            {"price", std::to_string(selected->buy_price)},
                            {"day", std::to_string(ctx.current_day)}
                        }
                    });
                } else {
                    ctx.push_hint("金币不足，无法购买节日限定商品。", 2.2f);
                }
                UpdateUiAfterInteraction(ctx);
                return true;
            }
            if (!kShopSystem.CanOpenTideShop(ctx.cloud_system.CurrentState())) {
                ctx.push_hint("大潮商店仅在大潮日开放。", 2.2f);
                UpdateUiAfterInteraction(ctx);
                return true;
            }
            const PriceTableEntry* selected = kShopSystem.SelectBySource(
                ctx.price_table, "tide_shop");
            if (selected && !kShopSystem.CanPurchaseByWeeklyLimit(*selected, ctx.weekly_buy_count)) {
                ctx.push_hint(
                    "本周已达到该商品购买上限（"
                    + std::to_string(kShopSystem.WeeklyPurchaseLimitPerItem()) + "）。",
                    2.6f);
            } else if (selected && kShopSystem.TryPurchase(
                                *selected,
                                ctx.gold,
                                ctx.inventory,
                                ctx.weekly_buy_count,
                                nullptr)) {
                if (ctx.play_sfx) ctx.play_sfx("shop_purchase");
                ctx.push_hint("大潮商店购入：" + ItemDisplayName(selected->item_id), 2.2f);
            } else if (!selected) {
                ctx.push_hint("大潮商店暂时无货。", 2.2f);
            } else {
                ctx.push_hint("金币不足。", 2.2f);
            }
            UpdateUiAfterInteraction(ctx);
            return true;
        }
        if (target.Label() == "Spirit Beast") {
            const bool has_sickle = ctx.inventory.CountOf("SpiritSickle") > 0;
            const int amount = has_sickle ? 3 : 1;
            ctx.inventory.AddItem("spirit_dust", amount);
            ctx.push_hint(has_sickle ? "灵镰攻击成功，获得灵尘 x3。" : "普通采集获得灵尘 x1。", 2.4f);
            UpdateUiAfterInteraction(ctx);
            return true;
        }
        if (target.Label() == "Spirit Beast Zone") {
            ctx.push_hint("这里有灵兽出没，按 J 可尝试进入净化战斗。", 2.2f);
            UpdateUiAfterInteraction(ctx);
            return true;
        }

        switch (target.Type()) {
        case CloudSeamanor::domain::InteractableType::GatheringNode: {
            // 剪刀采集效率加成
            float scissors_efficiency = 1.0f;
            if (ctx.inventory.CountOf("scissors_copper") > 0) scissors_efficiency = std::max(scissors_efficiency, 1.15f);
            if (ctx.inventory.CountOf("scissors_silver") > 0) scissors_efficiency = std::max(scissors_efficiency, 1.20f);
            if (ctx.inventory.CountOf("scissors_gold") > 0) scissors_efficiency = std::max(scissors_efficiency, 1.35f);
            if (ctx.inventory.CountOf("scissors_spirit") > 0) scissors_efficiency = std::max(scissors_efficiency, 1.50f);

            int amount = std::max(1, static_cast<int>(static_cast<float>(target.RewardAmount()) * scissors_efficiency));
            ctx.pickups.emplace_back(target.Shape().getPosition() + sf::Vector2f(14.0f, -10.0f),
                target.RewardItem(), amount);
            ctx.refresh_pickup_visual(ctx.pickups.back());
            {
                const float cloud_density = ctx.cloud_system.CurrentSpiritDensity();
                const float beast_share = ctx.spirit_beast.daily_interacted
                    ? GameConstants::SpiritBeast::AssistHarvestMultiplier : 1.0f;
                if (ctx.skills.AddExp(CloudSeamanor::domain::SkillType::SpiritForage,
                        GameConstants::Skill::SpiritForageExpBase, cloud_density, 1.0f, beast_share)) {
                    const int new_level = ctx.skills.GetLevel(CloudSeamanor::domain::SkillType::SpiritForage);
                    ctx.on_skill_level_up(CloudSeamanor::domain::SkillType::SpiritForage, new_level);
                    ctx.push_hint("灵觅技能提升至 Lv." + std::to_string(new_level) + "！", 3.0f);
                }
            }
            const std::string scissors_hint = (scissors_efficiency > 1.0f)
                ? "（剪刀加成 +" + std::to_string(static_cast<int>((scissors_efficiency - 1.0f) * 100)) + "%）"
                : "";
            ctx.push_hint("已从 " + target.Label() + " 采集 x" + std::to_string(amount) + scissors_hint + "，记得把掉落物捡起来。", 2.6f);
            ctx.log_info("已从 " + target.Label() + " 采集 x" + std::to_string(amount) + scissors_hint + "。");
            break;
        }
        case CloudSeamanor::domain::InteractableType::Workstation: {
            const auto* machine = ctx.workshop.GetMachine("tea_machine");
            if (ctx.inventory.CountOf("spirit_dust") >= 3 && ctx.inventory.CountOf("SpiritSickle") == 0) {
                (void)ctx.inventory.TryRemoveItem("spirit_dust", 3);
                ctx.inventory.AddItem("SpiritSickle", 1);
                ctx.push_hint("工坊打造完成：灵气镰刀。", 2.6f);
                break;
            }
            if (ctx.inventory.CountOf("spirit_grass") >= 3) {
                (void)ctx.inventory.TryRemoveItem("spirit_grass", 3);
                ctx.inventory.AddItem("spirit_essence", 1);
                ctx.push_hint("加工完成：灵粹 x1（可售约 130 金）。", 2.4f);
                break;
            }
            if (ctx.inventory.CountOf("cloud_dew") >= 2 && ctx.inventory.CountOf("spirit_dust") >= 1) {
                (void)ctx.inventory.TryRemoveItem("cloud_dew", 2);
                (void)ctx.inventory.TryRemoveItem("spirit_dust", 1);
                ctx.inventory.AddItem("cloud_elixir", 1);
                ctx.push_hint("加工完成：云华灵药 x1（可售约 220 金）。", 2.4f);
                break;
            }
            if (ctx.inventory.CountOf("cloud_elixir") >= 1 && ctx.inventory.CountOf("TeaPack") >= 1) {
                (void)ctx.inventory.TryRemoveItem("cloud_elixir", 1);
                (void)ctx.inventory.TryRemoveItem("TeaPack", 1);
                ctx.inventory.AddItem("immortal_tea", 1);
                ctx.push_hint("加工完成：不朽灵茶 x1（可售约 680 金）。", 2.8f);
                break;
            }
            if (machine && machine->is_processing) {
                ctx.push_hint("加工中 " + std::to_string(static_cast<int>(machine->progress)) + "%...", 2.2f);
                ctx.log_info("制茶机正在运行中。");
            } else if (ctx.tea_machine.queued_output > 0) {
                ctx.inventory.AddItem("TeaPack", 1);
                ctx.tea_machine.queued_output -= 1;
                if (ctx.play_sfx) ctx.play_sfx("ui_select");
                ctx.push_hint("领取成功：茶包 x1。", 2.6f);
                ctx.log_info("玩家领取了制茶机产物 TeaPack x1。");
            } else {
                static const std::array<const char*, 6> kTeaRecipePriority{
                    "mist_green_tea_process",
                    "sunset_oolong_process",
                    "frost_needle_tea_process",
                    "tea_essence_extract",
                    "spirit_tea_blend",
                    "green_tea"
                };
                bool started = false;
                std::string started_name = "茶包";
                int locked_recipe_count = 0;
                for (const auto* recipe_id : kTeaRecipePriority) {
                    const auto* recipe = ctx.workshop.GetRecipe(recipe_id);
                    if (recipe == nullptr) {
                        continue;
                    }
                    if (!ctx.workshop.IsRecipeUnlocked(*recipe)) {
                        ++locked_recipe_count;
                        continue;
                    }
                    if (ctx.inventory.CountOf(recipe->input_item) < recipe->input_count) {
                        continue;
                    }
                    if (ctx.workshop.StartProcessing("tea_machine", recipe->id, ctx.inventory)) {
                        started = true;
                        started_name = recipe->name;
                        break;
                    }
                }
                if (started) {
                    if (ctx.play_sfx) ctx.play_sfx("ui_select");
                    ctx.push_hint("工坊已开始加工：" + started_name + "。", 2.8f);
                    ctx.log_info("制茶机已启动（工坊系统配方链）。");
                } else {
                    if (locked_recipe_count > 0) {
                        ctx.push_hint("部分配方尚未解锁：提升工坊等级可开启更多加工链。", 2.6f);
                    } else {
                        ctx.push_hint("没有可用加工配方或原料不足。", 2.4f);
                    }
                    ctx.log_info("工坊配方无法启动：缺少原料或槽位。");
                }
            }
            break;
        }
        case CloudSeamanor::domain::InteractableType::Storage: {
            if (target.Label() == "Greenhouse Gate") {
                if (!ctx.greenhouse_unlocked) {
                    ctx.push_hint("主屋升到 Lv.3 后可解锁温室。", 2.4f);
                } else {
                    ctx.greenhouse_tag_next_planting = !ctx.greenhouse_tag_next_planting;
                    ctx.push_hint(
                        ctx.greenhouse_tag_next_planting
                            ? "温室入口已激活：后续播种地块将标记为温室地块。"
                            : "温室入口已关闭：后续播种将恢复为普通地块。",
                        2.6f);
                }
                break;
            }
            if (target.Label() == "Main House") {
                // 早餐/进食：使用 HungerTable 驱动饱食恢复
                if (ctx.hunger.HasEatenBreakfast()) {
                    ctx.push_hint("你已经吃过早餐了。", 2.0f);
                    break;
                }
                if (ctx.current_hour >= 10) {
                    ctx.push_hint("早餐时间已过（10:00 前）。", 2.2f);
                    break;
                }

                const auto* table = &CloudSeamanor::domain::GetGlobalHungerTable();
                // 优先消耗更“像早餐”的物品
                static const std::array<const char*, 6> kBreakfastItems{
                    "fried_rice", "rice_ball", "noodles", "veggie_soup", "warm_milk", "tea_egg"
                };
                bool ate = false;
                for (const auto* item_id : kBreakfastItems) {
                    if (ctx.inventory.CountOf(item_id) > 0 && ctx.inventory.TryRemoveItem(item_id, 1)) {
                        if (const auto* def = table->Get(item_id)) {
                            ctx.hunger.RestoreFromFood(def->hunger_restore, def->quality_multiplier);
                        } else {
                            ctx.hunger.Restore(18);
                        }
                        ctx.hunger.SetEatenBreakfast(true);
                        ctx.push_hint("你吃了早餐，饱食度恢复。", 2.4f);
                        ate = true;
                        break;
                    }
                }
                if (!ate) {
                    // 没有早餐物品：仍允许“清茶淡饭”式恢复，避免惩罚
                    ctx.hunger.Restore(10);
                    ctx.hunger.SetEatenBreakfast(true);
                    ctx.push_hint("你喝了点热水充饥，饱食度小幅恢复。", 2.4f);
                }
                break;
            }
            if (target.Label() == "Coop Barn") {
                if (ctx.inventory.CountOf("Feed") <= 0) {
                    ctx.push_hint("畜棚：暂无饲料，明天可在杂货店补货。", 2.2f);
                } else {
                    (void)ctx.inventory.TryRemoveItem("Feed", 1);
                    ctx.coop_fed_today += 1;
                    ctx.push_hint("畜棚：已投喂，明早会产出鸡蛋/牛奶。", 2.2f);
                }
                break;
            }
            if (target.Label() == "Inn Desk") {
                if (!ctx.pending_skill_branches.empty()) {
                    ctx.push_hint("有技能已到 10 级：请打开技能分支面板进行独立选择。", 2.4f);
                    break;
                }
                if (ctx.gold >= 80) {
                    ctx.gold -= 80;
                    ctx.inn_gold_reserve += 80;
                    ctx.push_hint("客栈运营投入 +80。次日日结将提升接待收益。", 2.4f);
                    if (!ctx.mail_orders.empty()) {
                        ctx.push_hint("当前已有远程订单在路上，可在邮件面板查看。", 2.0f);
                    }
                } else {
                    ctx.push_hint("客栈运营资金不足（需要 80 金）。", 2.2f);
                }
                break;
            }
            if (target.Label() == "Decoration Bench") {
                if (!ctx.pending_skill_branches.empty()) {
                    ctx.push_hint("有技能分支待定：请在独立技能面板中选择 A / B 路线。", 2.4f);
                    break;
                }
                constexpr const char* kDecorIds[] = {
                    "tea_room_screen", "tea_room_lamp", "tea_room_vase",
                    "tea_room_scroll", "tea_room_mat", "tea_room_bonsai"};
                if (!ctx.placed_objects.empty()
                    && ctx.placed_objects.back().room == "tea_room"
                    && ctx.placed_objects.back().custom_data == "preview") {
                    ctx.push_hint("DIY 摆放模式已开启：用方向键移动，R 旋转，Enter 确认。", 2.4f);
                    break;
                }
                const int placed_count = static_cast<int>(std::count_if(
                    ctx.placed_objects.begin(), ctx.placed_objects.end(), [](const PlacedObject& obj) {
                        return obj.room == "tea_room";
                    }));
                if (placed_count >= 24) {
                    for (auto it = ctx.placed_objects.rbegin(); it != ctx.placed_objects.rend(); ++it) {
                        if (it->room == "tea_room") {
                            ctx.push_hint("DIY：茶室可编辑格已满，已收回最后一个摆件。", 2.4f);
                            ctx.placed_objects.erase(std::next(it).base());
                            break;
                        }
                    }
                    break;
                }
                kDecorationSystem.TryCraftDecoration(
                    ctx.inventory,
                    ctx.decoration_score,
                    [&ctx](const std::string& msg) { ctx.push_hint(msg, 2.2f); },
                    [](const std::string& id) {
                        GlobalEventBus().Emit(Event{
                            "achievement_unlock",
                            {
                                {"id", id},
                                {"title", "家园设计师"},
                            }});
                    });
                GlobalEventBus().Emit(Event{
                    "build",
                    {
                        {"type", "decoration"},
                        {"decoration_score", std::to_string(ctx.decoration_score)}
                    }
                });
                ctx.placed_objects.push_back(PlacedObject{
                    .object_id = kDecorIds[placed_count % 6],
                    .tile_x = placed_count % 6,
                    .tile_y = placed_count / 6,
                    .rotation = 0,
                    .room = "tea_room",
                    .custom_data = "preview"});
                ctx.push_hint("DIY：已进入摆放模式。方向键移动，R 旋转，Enter 确认，Backspace 收回。", 2.8f);
                break;
            }
            if (target.Label().find("Fishing") != std::string::npos) {
                if (ctx.start_fishing_qte) {
                    ctx.start_fishing_qte(target.Label());
                } else {
                    ctx.push_hint("垂钓 QTE 初始化失败。", 2.0f);
                }
                break;
            }
            if (target.Label() == "Pet House") {
                AdoptPet_(ctx, "cat", "pet_house");
                break;
            }
            if (target.Label() == "Pet Event") {
                AdoptPet_(ctx, "bird", "event");
                break;
            }
            const bool can_upgrade = (ctx.main_house_repair.level < kMainHouseMaxLevel);
            const MainHouseUpgradeCost cost = QueryMainHouseUpgradeCost(ctx.main_house_repair.level);
            const std::vector<CloudSeamanor::domain::ItemCount> repair_costs{
                {"Wood", cost.wood_cost},
                {"Turnip", cost.turnip_cost}
            };
            if (can_upgrade && cost.next_level > 0 && ctx.gold >= cost.gold_cost
                && ctx.inventory.TryRemoveItems(repair_costs)) {
                ctx.gold -= cost.gold_cost;
                ctx.main_house_repair.level = cost.next_level;
                ctx.main_house_repair.completed = (ctx.main_house_repair.level >= 2);
                ctx.main_house_repair.wood_cost = cost.wood_cost;
                ctx.main_house_repair.turnip_cost = cost.turnip_cost;
                ctx.main_house_repair.gold_cost = cost.gold_cost;
                if (!ctx.obstacle_shapes.empty()) {
                    ctx.obstacle_shapes.front().setFillColor(sf::Color(148, 122, 92));
                    ctx.obstacle_shapes.front().setOutlineColor(sf::Color(86, 62, 38));
                }
                ctx.push_hint("主屋升级到 Lv." + std::to_string(ctx.main_house_repair.level) + "。", 3.0f);
                ctx.log_info("主屋升级完成。");
                GlobalEventBus().Emit(Event{
                    "build",
                    {
                        {"type", "main_house"},
                        {"house_level", std::to_string(ctx.main_house_repair.level)}
                    }
                });
                if (cost.workshop_level > 0 && cost.workshop_slots > 0) {
                    (void)ctx.workshop.Upgrade(cost.workshop_level, cost.workshop_slots);
                }
                if (cost.unlock_greenhouse && !ctx.greenhouse_unlocked) {
                    ctx.greenhouse_unlocked = true;
                    ctx.push_hint("温室入口已解锁，前往温室门牌交互后可标记后续播种地块。", 2.8f);
                }
                if (ctx.main_house_repair.level >= kMainHouseMaxLevel) {
                    ctx.push_hint("主屋已升至最高阶：大宅。", 2.6f);
                }
            } else if (!can_upgrade) {
                ctx.push_hint("主屋已满级。", 2.0f);
                ctx.push_hint(LatestDiarySummary_(ctx.diary_entries), 3.4f);
                ctx.log_info("主屋已满级。");
            } else {
                ctx.push_hint("升级需要 木材 x" + std::to_string(cost.wood_cost)
                    + " 萝卜 x" + std::to_string(cost.turnip_cost)
                    + " 金币 x" + std::to_string(cost.gold_cost) + "。", 2.8f);
                ctx.push_hint("茶室书桌上可翻阅最近的庄园日记。", 2.2f);
                ctx.log_info("主屋升级材料不足。");
            }
            break;
        }
        }

        UpdateUiAfterInteraction(ctx);
        return true;
    }

    if (ctx.highlighted_index >= 0) {
        ctx.push_hint("体力不足，无法进行交互。", 2.4f);
        ctx.log_info("体力不足，无法进行交互。");
    } else {
        ctx.push_hint("附近没有可交互目标。靠近作物、设施、灵兽或 NPC 试试看。", 2.4f);
        ctx.log_info("按下交互键时，附近没有可交互目标。");
    }
    UpdateUiAfterInteraction(ctx);
    return true;
}

} // namespace CloudSeamanor::engine
