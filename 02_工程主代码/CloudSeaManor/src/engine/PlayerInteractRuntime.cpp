#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/PlayerInteractRuntime.hpp"
#include "CloudSeamanor/CloudSystem.hpp"
#include "CloudSeamanor/DynamicLifeSystem.hpp"
#include "CloudSeamanor/GameAppNpc.hpp"
#include "CloudSeamanor/GameAppFarming.hpp"
#include "CloudSeamanor/GameAppText.hpp"
#include "CloudSeamanor/GameConstants.hpp"
#include "CloudSeamanor/GameClock.hpp"
#include "CloudSeamanor/Interactable.hpp"
#include "CloudSeamanor/Inventory.hpp"
#include "CloudSeamanor/PickupDrop.hpp"
#include "CloudSeamanor/Stamina.hpp"
#include "CloudSeamanor/WorkshopSystem.hpp"
#include "CloudSeamanor/GameAppText.hpp"
#include "CloudSeamanor/engine/systems/DecorationSystem.hpp"
#include "CloudSeamanor/engine/systems/ShopSystem.hpp"
#include "CloudSeamanor/EventBus.hpp"

#include "CloudSeamanor/DialogueEngine.hpp"
#include "CloudSeamanor/Profiling.hpp"

#include <algorithm>

namespace CloudSeamanor::engine {

namespace {

const DecorationSystem kDecorationSystem;
const ShopSystem kShopSystem;
constexpr sf::Color kGiftHeartLovedColor(220, 64, 96);
constexpr sf::Color kGiftHeartLikedColor(255, 153, 204);
constexpr sf::Color kGiftHeartDislikedColor(160, 160, 160);
constexpr sf::Color kGiftHeartNeutralColor(255, 182, 193);

void UpdateUiAfterInteraction(PlayerInteractRuntimeContext& ctx) {
    ctx.update_hud_text();
    ctx.refresh_window_title();
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
                       const sf::Color& color) {
    for (std::size_t i = from_index; i < particles.size(); ++i) {
        sf::Color c = color;
        c.a = particles[i].shape.getFillColor().a;
        particles[i].shape.setFillColor(c);
    }
}

void UnlockAchievement_(
    PlayerInteractRuntimeContext& ctx,
    const std::string& id,
    const std::string& title_hint) {
    auto it = ctx.achievements.find(id);
    if (it != ctx.achievements.end() && it->second) {
        return;
    }
    ctx.achievements[id] = true;
    ctx.push_hint("成就解锁：" + title_hint, 2.6f);
}

void AdoptPet_(
    PlayerInteractRuntimeContext& ctx,
    const std::string& pet_type,
    const std::string& source_label) {
    if (!ctx.pet_adopted) {
        ctx.pet_adopted = true;
        ctx.pet_type = pet_type;
        ctx.push_hint("你收养了一只宠物（" + pet_type + "）。来源：" + source_label + "。", 2.4f);
        UnlockAchievement_(ctx, "first_pet", "初次收养");
    } else if (ctx.pet_type != pet_type) {
        ctx.pet_type = pet_type;
        ctx.push_hint("你将当前陪伴宠物切换为：" + pet_type + "。", 2.0f);
    } else {
        ctx.push_hint("宠物正在院子里玩耍。", 2.0f);
    }
    ctx.achievements["pet_collected_" + pet_type] = true;
    const bool has_cat = ctx.achievements["pet_collected_cat"];
    const bool has_dog = ctx.achievements["pet_collected_dog"];
    const bool has_bird = ctx.achievements["pet_collected_bird"];
    if (has_cat && has_dog && has_bird) {
        UnlockAchievement_(ctx, "pet_all_types", "全类型宠物收集");
    }
}

const char* PetTypeFromCommodity_(const std::string& item_id) {
    if (item_id == "pet_cat_license") return "cat";
    if (item_id == "pet_dog_license") return "dog";
    if (item_id == "pet_bird_license") return "bird";
    return nullptr;
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
            ctx.spawn_heart_particles(beast.shape.getPosition(), ctx.heart_particles);
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
            ctx.spawn_heart_particles(beast.shape.getPosition(), ctx.heart_particles);
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
        if (beast.personality == SpiritBeastPersonality::Lively) {
            ctx.achievements["beast_type_lively"] = true;
        } else if (beast.personality == SpiritBeastPersonality::Lazy) {
            ctx.achievements["beast_type_lazy"] = true;
        } else if (beast.personality == SpiritBeastPersonality::Curious) {
            ctx.achievements["beast_type_curious"] = true;
        }
        GlobalEventBus().Emit(Event{
            "beast_type_collected",
            {
                {"lively", ctx.achievements["beast_type_lively"] ? "1" : "0"},
                {"lazy", ctx.achievements["beast_type_lazy"] ? "1" : "0"},
                {"curious", ctx.achievements["beast_type_curious"] ? "1" : "0"},
            }});
        if (beast.favor >= 100) {
            UnlockAchievement_(ctx, "beast_bond_max", "灵兽羁绊满级");
        }
        UnlockAchievement_(ctx, "beast_bond", "初次结缘");
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
    } else if (!ctx.inventory.TryRemoveItem("TeaPack", 1)) {
        ctx.dialogue_text = "你需要 茶包 x1 才能送礼。";
        ctx.push_hint(ctx.dialogue_text, 2.6f);
    } else {
        npc.daily_gifted = true;
        npc.last_gift_day = ctx.current_day;
        int favor_change = 1;
        sf::Color heart_color = kGiftHeartNeutralColor;
        if (ContainsItem(npc.prefs.loved, "TeaPack")) {
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
            npc.married = true;
            ctx.push_hint(npc.display_name + " 与你立下了婚约。", 3.2f);
        }
        const std::size_t before_hearts = ctx.heart_particles.size();
        ctx.spawn_heart_particles(npc.shape.getPosition(), ctx.heart_particles);
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
                    .on_node_change = [](const DialogueNode&) {},
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
            ctx.spawn_heart_particles(ctx.spirit_beast.shape.getPosition(), ctx.heart_particles);
            ctx.push_hint("你和灵兽建立了新的羁绊。它今天的协助能力已经激活。", 2.8f);
            if (ctx.inventory.TryRemoveItem("spirit_grass", 1)) {
                ctx.inventory.AddItem("spirit_dust", 2);
                ctx.push_hint("你喂食了灵兽，获得灵尘回礼 x2。", 2.2f);
            }
            UnlockAchievement_(ctx, "beast_bond", "初次结缘");
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
                if (!ConsumeToolDurability("ToolAxe")) break;
                plot.obstacle_hits_left = std::max(0, plot.obstacle_hits_left - 1);
                if (plot.obstacle_hits_left <= 0) {
                    cleared_now = true;
                } else {
                    ctx.push_hint("你砍向石头。剩余 " + std::to_string(plot.obstacle_hits_left) + " 次。", 2.2f);
                }
                break;
            }
            case PlotObstacleType::Stump: {
                if (!ConsumeToolDurability("ToolPickaxe")) break;
                plot.obstacle_hits_left = std::max(0, plot.obstacle_hits_left - 1);
                if (plot.obstacle_hits_left <= 0) {
                    cleared_now = true;
                } else {
                    ctx.push_hint("你敲击树桩。剩余 " + std::to_string(plot.obstacle_hits_left) + " 次。", 2.2f);
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
        } else if (!plot.seeded) {
            if (ctx.inventory.TryRemoveItem(plot.seed_item_id, 1)) {
                plot.seeded = true;
                plot.in_greenhouse = ctx.greenhouse_tag_next_planting;
                plot.growth = 0.0f;
                plot.stage = 1;
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
            ctx.push_hint(plot.crop_name + " 已浇水。当前云海只会提供正向加成，可以安心等它成长。", 3.0f);
            ctx.log_info(plot.crop_name + " 地块已浇水。");
        } else if (plot.ready) {
            plot.quality = CloudSeamanor::domain::CropTable::CalculateQuality(
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
            const float quality_mult = CloudSeamanor::domain::CropTable::QualityHarvestMultiplier(plot.quality);
            int actual_amount = std::max(1, static_cast<int>(
                static_cast<float>(plot.harvest_amount) * quality_mult));
            if (plot.disease_days >= 3) {
                actual_amount = 0;
            }
            const auto quality_text = std::string(CloudSeamanor::domain::CropTable::QualityToText(plot.quality));
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
            ctx.last_trade_quality = plot.quality;
            plot.quality = CloudSeamanor::domain::CropQuality::Normal;
            if (actual_amount > 0) {
                ctx.push_hint("已收获 " + quality_text + " " + ItemDisplayName(plot.harvest_item_id) + " x"
                    + std::to_string(actual_amount) + "。", 2.8f);
                ctx.log_info(plot.crop_name + " 已收获（品质：" + quality_text + "，数量：" + std::to_string(actual_amount) + "）。");
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
            const int amount =
                (ctx.cloud_system.CurrentState() == CloudSeamanor::domain::CloudState::Tide) ? 1 : 2;
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
            const PriceTableEntry* sell_entry = kShopSystem.SelectFirstSellable(
                ctx.price_table, "purchaser", ctx.inventory);
            int income = 0;
            if (sell_entry && kShopSystem.TrySellOne(
                                  *sell_entry,
                                  ctx.last_trade_quality,
                                  ctx.inventory,
                                  ctx.gold,
                                  ctx.weekly_sell_count,
                                  income)) {
                ctx.push_hint("收购商买下了 " + ItemDisplayName(sell_entry->item_id)
                    + "，获得 " + std::to_string(income) + " 金。", 2.6f);
            } else {
                ctx.push_hint("没有可出售物品。", 2.2f);
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
        if (target.Label() == "General Store") {
            const PriceTableEntry* selected = kShopSystem.SelectGeneralStoreItem(
                ctx.price_table, ctx.daily_general_store_stock);
            if (!selected) {
                ctx.push_hint("今日杂货店已售罄。", 2.2f);
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
            if (!kShopSystem.CanOpenTideShop(ctx.cloud_system.CurrentState())) {
                ctx.push_hint("大潮商店仅在大潮日开放。", 2.2f);
                UpdateUiAfterInteraction(ctx);
                return true;
            }
            const PriceTableEntry* selected = kShopSystem.SelectBySource(
                ctx.price_table, "tide_shop");
            if (selected && kShopSystem.TryPurchase(
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
            ctx.pickups.emplace_back(target.Shape().getPosition() + sf::Vector2f(14.0f, -10.0f),
                target.RewardItem(), target.RewardAmount());
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
            ctx.push_hint("已从 " + target.Label() + " 采集，记得把掉落物捡起来。", 2.6f);
            ctx.log_info("已从 " + target.Label() + " 采集，已生成掉落物。");
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
                ctx.push_hint("加工完成：灵粹 x1。", 2.4f);
                break;
            }
            if (ctx.inventory.CountOf("cloud_dew") >= 2 && ctx.inventory.CountOf("spirit_dust") >= 1) {
                (void)ctx.inventory.TryRemoveItem("cloud_dew", 2);
                (void)ctx.inventory.TryRemoveItem("spirit_dust", 1);
                ctx.inventory.AddItem("cloud_elixir", 1);
                ctx.push_hint("加工完成：云华灵药 x1。", 2.4f);
                break;
            }
            if (ctx.inventory.CountOf("cloud_elixir") >= 1 && ctx.inventory.CountOf("TeaPack") >= 1) {
                (void)ctx.inventory.TryRemoveItem("cloud_elixir", 1);
                (void)ctx.inventory.TryRemoveItem("TeaPack", 1);
                ctx.inventory.AddItem("immortal_tea", 1);
                ctx.push_hint("加工完成：不朽灵茶 x1。", 2.8f);
                break;
            }
            if (machine && machine->is_processing) {
                ctx.push_hint("加工中 " + std::to_string(static_cast<int>(machine->progress)) + "%...", 2.2f);
                ctx.log_info("制茶机正在运行中。");
            } else if (ctx.tea_machine.queued_output > 0) {
                ctx.inventory.AddItem("TeaPack", 1);
                ctx.tea_machine.queued_output -= 1;
                ctx.push_hint("领取成功：茶包 x1。", 2.6f);
                ctx.log_info("玩家领取了制茶机产物 TeaPack x1。");
            } else if (ctx.workshop.StartProcessing("tea_machine", "green_tea", ctx.inventory)) {
                ctx.push_hint("按 E 开始加工已触发（消耗 茶叶 x2）。", 2.8f);
                ctx.log_info("制茶机已启动（工坊系统）。");
            } else {
                ctx.push_hint("没有原料可加工。", 2.4f);
                ctx.log_info("茶叶不足，无法烘制茶包。");
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
                if (ctx.gold >= 80) {
                    ctx.gold -= 80;
                    ctx.inn_gold_reserve += 80;
                    ctx.mail_orders.push_back(MailOrderEntry{"TeaPack", 1, ctx.current_day + 1});
                    ctx.push_hint("客栈接单成功，次日结算返还茶包订单。", 2.4f);
                } else {
                    ctx.push_hint("客栈运营资金不足（需要 80 金）。", 2.2f);
                }
                break;
            }
            if (target.Label() == "Decoration Bench") {
                kDecorationSystem.TryCraftDecoration(
                    ctx.inventory,
                    ctx.decoration_score,
                    [&ctx](const std::string& msg) { ctx.push_hint(msg, 2.2f); },
                    [&ctx](const std::string& id) { UnlockAchievement_(ctx, id, "家园设计师"); });
                GlobalEventBus().Emit(Event{
                    "build",
                    {
                        {"type", "decoration"},
                        {"decoration_score", std::to_string(ctx.decoration_score)}
                    }
                });
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
            const bool can_upgrade = (ctx.main_house_repair.level < 3);
            const int next_level = ctx.main_house_repair.level + 1;
            const int wood_cost = (next_level == 2 ? 10 : 20);
            const int turnip_cost = (next_level == 2 ? 2 : 4);
            const int gold_cost = (next_level == 2 ? 500 : 2000);
            const std::vector<CloudSeamanor::domain::ItemCount> repair_costs{
                {"Wood", wood_cost},
                {"Turnip", turnip_cost}
            };
            if (can_upgrade && ctx.gold >= gold_cost && ctx.inventory.TryRemoveItems(repair_costs)) {
                ctx.gold -= gold_cost;
                ctx.main_house_repair.level = next_level;
                ctx.main_house_repair.completed = true;
                ctx.main_house_repair.gold_cost = gold_cost;
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
                if (ctx.main_house_repair.level >= 2) {
                    (void)ctx.workshop.Upgrade(2, 2);
                }
                if (ctx.main_house_repair.level >= 3) {
                    (void)ctx.workshop.Upgrade(3, 4);
                    if (!ctx.greenhouse_unlocked) {
                        ctx.greenhouse_unlocked = true;
                        ctx.push_hint("温室入口已解锁，前往温室门牌交互后可标记后续播种地块。", 2.8f);
                    }
                }
            } else if (!can_upgrade) {
                ctx.push_hint("主屋已满级。", 2.0f);
                ctx.log_info("主屋已满级。");
            } else {
                ctx.push_hint("升级需要 木材 x" + std::to_string(wood_cost)
                    + " 萝卜 x" + std::to_string(turnip_cost)
                    + " 金币 x" + std::to_string(gold_cost) + "。", 2.8f);
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
