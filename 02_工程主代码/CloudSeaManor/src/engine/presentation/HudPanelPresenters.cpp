#include "CloudSeamanor/engine/presentation/HudPanelPresenters.hpp"

#include "CloudSeamanor/engine/AudioManager.hpp"
#include "CloudSeamanor/app/GameAppText.hpp"
#include "CloudSeamanor/engine/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/infrastructure/GameConfig.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"

#include <algorithm>
#include <array>
#include <unordered_map>

namespace CloudSeamanor::engine {

// ============================================================================
// 【ApplyRuntimeConfiguration】运行时面板配置
// 将 GameApp::Run() 中的面板初始化配置下沉到此 Presenter，
// GameApp 只负责调用一个方法并传入必要的运行时引用。
// ============================================================================
void HudPanelPresenters::ApplyRuntimeConfiguration(
    PixelGameHud& hud,
    GameRuntime& runtime,
    const engine::audio::AudioManager* audio,
    bool fullscreen) {
    const auto& cfg = runtime.Config();

    PixelSettingsPanel::TextConfig settings_text;
    settings_text.slots_title = cfg.GetString("settings_slots_title", settings_text.slots_title);
    settings_text.save_slot_prefix = cfg.GetString("settings_save_slot_prefix", settings_text.save_slot_prefix);
    settings_text.load_slot_prefix = cfg.GetString("settings_load_slot_prefix", settings_text.load_slot_prefix);
    settings_text.bgm_prefix = cfg.GetString("settings_bgm_prefix", settings_text.bgm_prefix);
    settings_text.sfx_prefix = cfg.GetString("settings_sfx_prefix", settings_text.sfx_prefix);
    settings_text.display_mode_prefix = cfg.GetString("settings_display_mode_prefix", settings_text.display_mode_prefix);
    settings_text.fullscreen_text = cfg.GetString("settings_fullscreen_text", settings_text.fullscreen_text);
    settings_text.windowed_text = cfg.GetString("settings_windowed_text", settings_text.windowed_text);
    settings_text.operation_hint = cfg.GetString("settings_operation_hint", settings_text.operation_hint);
    settings_text.slot_prefix = cfg.GetString("settings_slot_prefix", settings_text.slot_prefix);
    settings_text.empty_slot_text = cfg.GetString("settings_empty_slot_text", settings_text.empty_slot_text);
    settings_text.has_save_text = cfg.GetString("settings_has_save_text", settings_text.has_save_text);
    settings_text.day_prefix = cfg.GetString("settings_day_prefix", settings_text.day_prefix);
    settings_text.no_preview_text = cfg.GetString("settings_no_preview_text", settings_text.no_preview_text);
    hud.MutableSettingsPanel().SetTextConfig(settings_text);
    hud.MutableSettingsPanel().SetRuntimeValues(
        audio ? audio->BGMVolume() : 0.7f,
        audio ? audio->SFXVolume() : 1.0f,
        fullscreen);
    hud.ConfigureNotificationTimings(
        cfg.GetFloat("ui_notification_fade_in", 0.3f),
        cfg.GetFloat("ui_notification_hold", 3.0f),
        cfg.GetFloat("ui_notification_fade_out", 0.3f),
        cfg.GetFloat("ui_cloud_report_duration", 4.0f));

    infrastructure::Logger::Info(
        "HudPanelPresenters: 运行时面板配置已应用");
}

namespace {

std::string AchievementDisplayName_(std::string_view id) {
    if (id == "first_crop") return "初次收获";
    if (id == "ten_crops") return "小有规模";
    if (id == "gift_expert") return "送礼达人";
    if (id == "master_builder") return "大宅建成";
    if (id == "beast_bond") return "初次结缘";
    if (id == "beast_bond_max") return "灵兽羁绊满级";
    if (id == "beast_all_types") return "全类型灵兽结缘";
    if (id == "home_designer") return "家园设计师";
    if (id == "small_tycoon") return "小富豪";
    if (id == "first_pet") return "初次收养";
    if (id == "pet_all_types") return "全类型宠物收集";
    if (id == "pet_collected_cat") return "收集宠物: 猫灵";
    if (id == "pet_collected_dog") return "收集宠物: 犬灵";
    if (id == "pet_collected_bird") return "收集宠物: 羽灵";
    if (id == "beast_type_lively") return "灵兽性格: 活泼";
    if (id == "beast_type_lazy") return "灵兽性格: 慵懒";
    if (id == "beast_type_curious") return "灵兽性格: 好奇";
    if (id == "tide_purifier") return "大潮祭净化者";
    if (id == "fest_calendar") return "节庆旅人";
    return id.empty() ? "未知成就" : std::string(id);
}

std::string PetDisplayName_(std::string_view pet_type) {
    if (pet_type == "cat") return "猫灵";
    if (pet_type == "dog") return "犬灵";
    if (pet_type == "bird") return "羽灵";
    return pet_type.empty() ? "未知灵兽" : std::string(pet_type);
}

}  // namespace

void HudPanelPresenters::UpdateTeaGardenPanel(PixelGameHud& hud, GameRuntime& runtime) {
    const auto& world_state = runtime.WorldState();
    const auto& cloud_system = runtime.Systems().GetCloud();

    TeaGardenPanelViewData tea_view;
    tea_view.cloud_state_text = cloud_system.CurrentStateText();
    tea_view.spirit_bonus = cloud_system.SpiritEnergyGain();
    tea_view.quality_bonus_percent = std::max(0, static_cast<int>((runtime.CloudMultiplier() - 1.0f) * 100.0f));
    tea_view.cloud_state_prefix = runtime.Config().GetString("tea_garden_cloud_state_prefix", tea_view.cloud_state_prefix);
    tea_view.spirit_bonus_prefix = runtime.Config().GetString("tea_garden_spirit_bonus_prefix", tea_view.spirit_bonus_prefix);
    tea_view.quality_bonus_prefix = runtime.Config().GetString("tea_garden_quality_bonus_prefix", tea_view.quality_bonus_prefix);
    tea_view.cloud_preview_prefix = runtime.Config().GetString("tea_garden_cloud_preview_prefix", tea_view.cloud_preview_prefix);
    tea_view.plots_title = runtime.Config().GetString("tea_garden_plots_title", tea_view.plots_title);
    tea_view.quality_hint_text = runtime.Config().GetString("tea_garden_quality_hint_text", tea_view.quality_hint_text);
    tea_view.bush_countdown_title = runtime.Config().GetString("tea_garden_bush_countdown_title", tea_view.bush_countdown_title);
    tea_view.actions_text = runtime.Config().GetString("tea_garden_actions_text", tea_view.actions_text);
    const auto bonus_pct = [](CloudSeamanor::domain::CloudState s) {
        return static_cast<int>((CloudGrowthMultiplier(s) - 1.0f) * 100.0f);
    };
    tea_view.cloud_preview_text =
        "晴+" + std::to_string(bonus_pct(CloudSeamanor::domain::CloudState::Clear)) + "%  "
        "雾+" + std::to_string(bonus_pct(CloudSeamanor::domain::CloudState::Mist)) + "%  "
        "浓云+" + std::to_string(bonus_pct(CloudSeamanor::domain::CloudState::DenseCloud)) + "%  "
        "大潮+" + std::to_string(bonus_pct(CloudSeamanor::domain::CloudState::Tide)) + "%";
    const auto& plots = world_state.GetTeaPlots();
    int tea_plot_count = 0;
    int normal_plot_count = 0;
    for (const auto& plot : plots) {
        if (plot.layer == TeaPlotLayer::TeaGardenExclusive) {
            ++tea_plot_count;
        } else {
            ++normal_plot_count;
        }
    }
    tea_view.actions_text += "  | 主屋Lv" + std::to_string(world_state.GetMainHouseRepair().level)
        + "  | 茶园地块 " + std::to_string(tea_plot_count)
        + "  普通农田 " + std::to_string(normal_plot_count);
    for (const auto& plot : plots) {
        if (plot.layer != TeaPlotLayer::TeaGardenExclusive) {
            continue;
        }
        TeaGardenPlotLineViewData line;
        line.name = plot.crop_name.empty() ? "茶园地块" : plot.crop_name;
        line.progress_percent = static_cast<int>(std::max(0.0f, std::min(1.0f, plot.growth)) * 100.0f);
        switch (plot.quality) {
        case CloudSeamanor::domain::CropQuality::Normal: line.quality_text = "普通"; break;
        case CloudSeamanor::domain::CropQuality::Fine: line.quality_text = "优质"; break;
        case CloudSeamanor::domain::CropQuality::Rare: line.quality_text = "珍品"; break;
        case CloudSeamanor::domain::CropQuality::Spirit: line.quality_text = "灵品"; break;
        case CloudSeamanor::domain::CropQuality::Holy: line.quality_text = "圣品"; break;
        }
        tea_view.plots.push_back(std::move(line));
        if (tea_view.plots.size() >= 3) {
            break;
        }
    }
    if (tea_view.plots.empty()) {
        tea_view.plots.push_back(TeaGardenPlotLineViewData{
            .name = "暂无茶园专属地块",
            .progress_percent = 0,
            .quality_text = "-"});
    }
    tea_view.bush_countdown_lines.clear();
    const auto& bushes = world_state.GetTeaBushes();
    for (const auto& bush : bushes) {
        const int remain_days = bush.DaysUntilHarvest(world_state.GetClock().Day());
        const std::string state_text = (remain_days <= 0)
            ? "可采摘"
            : (std::to_string(remain_days) + " 天后");
        const std::string display_name = bush.name.empty() ? bush.id : bush.name;
        tea_view.bush_countdown_lines.push_back(display_name + "： " + state_text);
    }
    if (tea_view.bush_countdown_lines.empty()) {
        tea_view.bush_countdown_lines.push_back("暂无茶灌木");
    }
    hud.UpdateTeaGardenPanel(tea_view);
}

void HudPanelPresenters::UpdateFestivalPanel(PixelGameHud& hud, GameRuntime& runtime) {
    const auto& world_state = runtime.WorldState();
    FestivalPanelViewData festival_view;
    const auto& festivals = runtime.Systems().GetFestivals();
    const auto& all_festivals = festivals.GetAllFestivals();
    festival_view.total_events = static_cast<int>(all_festivals.size());
    festival_view.completed_events = 0;
    for (const auto& fest : all_festivals) {
        if (fest.participated) {
            ++festival_view.completed_events;
        }
    }
    if (const auto* today = festivals.GetTodayFestival()) {
        festival_view.active_name = today->name;
        festival_view.countdown_days = 0;
    } else {
        const auto upcoming = festivals.GetUpcomingFestivals(1);
        if (!upcoming.empty()) {
            festival_view.active_name =
                runtime.Config().GetString("festival_next_prefix", "下一节日: ")
                + upcoming.front()->name;
            festival_view.countdown_days = std::max(0, upcoming.front()->day - world_state.GetClock().DayInSeason());
        }
    }
    const auto upcoming_three = festivals.GetUpcomingFestivals(3);
    festival_view.upcoming.clear();
    for (const auto* fest : upcoming_three) {
        if (fest == nullptr) continue;
        festival_view.upcoming.push_back(fest->name + "(第" + std::to_string(fest->day) + "天)");
    }
    festival_view.reward_text = runtime.Config().GetString(
        "festival_reward_text", festival_view.reward_text);
    festival_view.upcoming_prefix = runtime.Config().GetString(
        "festival_upcoming_prefix", festival_view.upcoming_prefix);
    festival_view.upcoming_empty_text = runtime.Config().GetString(
        "festival_upcoming_empty_text", festival_view.upcoming_empty_text);
    festival_view.participation_text = runtime.Config().GetString(
        "festival_participation_text", festival_view.participation_text);
    festival_view.selected_participation_text = runtime.Config().GetString(
        "festival_selected_participation_text", festival_view.selected_participation_text);
    festival_view.actions_text = runtime.Config().GetString(
        "festival_actions_text", festival_view.actions_text);
    const bool tide_afterglow_ready =
        runtime.PlotSystem().HasFlag("festival_tide_boss_victory")
        && runtime.PlotSystem().HasFlag("festival_joined_cloud_tide_ritual")
        && !runtime.PlotSystem().HasFlag("plot_ch3_tide_afterglow_done");
    if (tide_afterglow_ready) {
        festival_view.upcoming.insert(festival_view.upcoming.begin(), "主线：潮后余晖（已可触发）");
        festival_view.reward_text += " | 主线提示：潮后余晖可触发";
        festival_view.actions_text += " | 前往灵界/祭坛附近触发后续剧情";
    }
    hud.UpdateFestivalPanel(festival_view);
}

void HudPanelPresenters::UpdateShopPanel(PixelGameHud& hud, GameRuntime& runtime) {
    const auto& world_state = runtime.WorldState();
    ShopPanelViewData shop_view;
    shop_view.items_header_text = runtime.Config().GetString("shop_items_header_text", shop_view.items_header_text);
    shop_view.player_gold_prefix = runtime.Config().GetString("shop_player_gold_prefix", shop_view.player_gold_prefix);
    shop_view.selected_item_empty_text = runtime.Config().GetString("shop_selected_empty_text", shop_view.selected_item_empty_text);
    shop_view.actions_text = runtime.Config().GetString("shop_actions_text", shop_view.actions_text);
    shop_view.player_gold = world_state.GetGold();
    shop_view.items.clear();
    const auto& prices = world_state.GetPriceTable();
    const std::size_t shop_count = std::min<std::size_t>(3, prices.size());
    for (std::size_t i = 0; i < shop_count; ++i) {
        const auto& entry = prices[i];
        ShopPanelItemViewData item;
        item.name = ItemDisplayName(entry.item_id);
        item.price = std::max(0, entry.buy_price);
        item.stock = std::max(0, world_state.GetInventory().CountOf(entry.item_id));
        shop_view.items.push_back(std::move(item));
    }
    if (!shop_view.items.empty()) {
        const auto& first_item = shop_view.items.front();
        const bool affordable = shop_view.player_gold >= first_item.price;
        const std::string selected_prefix = runtime.Config().GetString("shop_selected_prefix", "选中商品:");
        const std::string price_prefix = runtime.Config().GetString("shop_price_prefix", "单价:");
        const std::string stock_prefix = runtime.Config().GetString("shop_stock_prefix", "库存:");
        const std::string affordable_text = runtime.Config().GetString("shop_affordable_text", "可购买");
        const std::string unaffordable_text = runtime.Config().GetString("shop_unaffordable_text", "金币不足");
        shop_view.selected_item_desc =
            selected_prefix + " " + first_item.name
            + "  " + price_prefix + std::to_string(first_item.price)
            + "  " + stock_prefix + std::to_string(first_item.stock)
            + "  " + (affordable ? affordable_text : unaffordable_text);
    } else {
        shop_view.selected_item_desc = runtime.Config().GetString("shop_empty_text", "暂无可购买商品");
    }
    hud.UpdateShopPanel(shop_view);
}

void HudPanelPresenters::UpdateMailPanel(PixelGameHud& hud, GameRuntime& runtime) {
    const auto& world_state = runtime.WorldState();
    MailPanelViewData mail_view;
    mail_view.list_title_text = runtime.Config().GetString(
        "mail_list_title_text", mail_view.list_title_text);
    mail_view.empty_detail_text = runtime.Config().GetString(
        "mail_empty_detail_text", mail_view.empty_detail_text);
    mail_view.unread_prefix_text = runtime.Config().GetString(
        "mail_unread_prefix_text", mail_view.unread_prefix_text);
    mail_view.unread_suffix_text = runtime.Config().GetString(
        "mail_unread_suffix_text", mail_view.unread_suffix_text);
    mail_view.actions_text = runtime.Config().GetString(
        "mail_actions_text", mail_view.actions_text);
    const auto& mail_orders = world_state.GetMailOrders();
    const int today = world_state.GetClock().Day();
    const std::string mail_sender_text = runtime.Config().GetString("mail_sender_text", "商会");
    const std::string mail_time_prefix = runtime.Config().GetString("mail_time_prefix", "第");
    const std::string mail_time_suffix = runtime.Config().GetString("mail_time_suffix", "天送达");
    const std::string mail_detail_prefix = runtime.Config().GetString("mail_detail_prefix", "详情: 附件");
    const std::string mail_detail_eta_prefix = runtime.Config().GetString("mail_detail_eta_prefix", "预计");
    const std::string mail_detail_eta_suffix = runtime.Config().GetString("mail_detail_eta_suffix", "天后送达");
    const std::string mail_detail_empty = runtime.Config().GetString("mail_detail_empty_text", "详情: 当前没有待处理邮件");
    const std::string mail_status_arrived = runtime.Config().GetString("mail_status_arrived_text", "已到达");
    const std::string mail_status_pending = runtime.Config().GetString("mail_status_pending_text", "运输中");
    const std::string mail_rule_prefix = runtime.Config().GetString("mail_rule_prefix", "规则");
    const std::string filter_rule_id = runtime.Config().GetString("mail_panel_filter_rule_id", "");

    std::unordered_map<std::string, int> rule_unread_counts;
    int arrived_count = 0;
    for (const auto& order : mail_orders) {
        if (order.claimed) {
            continue;
        }
        if (!filter_rule_id.empty() && filter_rule_id != "all" && order.source_rule_id != filter_rule_id) {
            continue;
        }
        MailPanelEntryViewData entry;
        const std::string source = order.source_rule_id.empty() ? "manual" : order.source_rule_id;
        entry.sender = order.sender.empty() ? mail_sender_text : order.sender;
        entry.subject = order.subject.empty()
            ? (ItemDisplayName(order.item_id) + " x" + std::to_string(order.count))
            : order.subject;
        entry.time_text = mail_time_prefix + std::to_string(order.deliver_day) + mail_time_suffix;
        if (order.deliver_day <= today) {
            ++arrived_count;
            if (!order.opened) {
                ++rule_unread_counts[source];
            }
            mail_view.arrived_entries.push_back(std::move(entry));
        } else {
            mail_view.pending_entries.push_back(std::move(entry));
        }
    }
    mail_view.unread_count = 0;
    for (const auto& order : mail_orders) {
        if (!order.claimed && order.deliver_day <= today && !order.opened) {
            ++mail_view.unread_count;
        }
    }
    if (!mail_orders.empty()) {
        const MailOrderEntry* best = nullptr;
        for (const auto& order : mail_orders) {
            if (order.claimed) continue;
            if (!filter_rule_id.empty() && filter_rule_id != "all" && order.source_rule_id != filter_rule_id) {
                continue;
            }
            if (order.deliver_day <= today) {
                best = &order;
                break;
            }
        }
        if (best == nullptr) {
            for (const auto& order : mail_orders) {
                if (order.claimed) continue;
                if (!filter_rule_id.empty() && filter_rule_id != "all" && order.source_rule_id != filter_rule_id) {
                    continue;
                }
                best = &order;
                break;
            }
        }
        if (best == nullptr) {
            mail_view.detail_text = mail_detail_empty;
            hud.UpdateMailPanel(mail_view);
            return;
        }
        const int remaining_days = std::max(0, best->deliver_day - today);
        const std::string status_text = (best->deliver_day <= today) ? mail_status_arrived : mail_status_pending;
        const std::string rule_text = best->source_rule_id.empty() ? "" : ("  [" + mail_rule_prefix + ":" + best->source_rule_id + "]");
        const std::string body_text = best->body.empty()
            ? (ItemDisplayName(best->item_id) + " x" + std::to_string(best->count))
            : best->body;
        std::string attachment_text = ItemDisplayName(best->item_id) + " x" + std::to_string(best->count);
        if (!best->secondary_item_id.empty() && best->secondary_count > 0) {
            attachment_text += " + " + ItemDisplayName(best->secondary_item_id) + " x" + std::to_string(best->secondary_count);
        }
        mail_view.detail_text =
            mail_detail_prefix + " " + body_text + "（附件: " + attachment_text + "）"
            + "  " + mail_detail_eta_prefix + std::to_string(remaining_days) + mail_detail_eta_suffix
            + "  [" + status_text + "]"
            + (best->receipt_sent ? "[已回执]" : "[待回执]") + rule_text;
    } else {
        mail_view.detail_text = mail_detail_empty;
    }
    if (!rule_unread_counts.empty()) {
        std::string grouped = "  分组:";
        for (const auto& [rule_id, count] : rule_unread_counts) {
            grouped += " [" + rule_id + ":" + std::to_string(count) + "]";
        }
        mail_view.actions_text += grouped;
    }
    hud.UpdateMailPanel(mail_view);
}

void HudPanelPresenters::UpdateAchievementPanel(PixelGameHud& hud, GameRuntime& runtime) {
    const auto& world_state = runtime.WorldState();
    AchievementPanelViewData achievement_view;
    const auto& achievements = world_state.GetAchievements();
    achievement_view.progress_prefix = runtime.Config().GetString(
        "achievement_progress_prefix", achievement_view.progress_prefix);
    achievement_view.legend_text = runtime.Config().GetString(
        "achievement_legend_text", achievement_view.legend_text);
    achievement_view.unlocked_mark = runtime.Config().GetString(
        "achievement_unlocked_mark", achievement_view.unlocked_mark);
    achievement_view.locked_mark = runtime.Config().GetString(
        "achievement_locked_mark", achievement_view.locked_mark);
    achievement_view.unlock_banner_text = runtime.Config().GetString(
        "achievement_unlock_banner_text", achievement_view.unlock_banner_text);
    achievement_view.total_count = static_cast<int>(achievements.size());
    achievement_view.unlocked_count = 0;
    for (const auto& [id, unlocked] : achievements) {
        if (unlocked) {
            ++achievement_view.unlocked_count;
            achievement_view.unlocked_titles.push_back(AchievementDisplayName_(id));
        } else {
            achievement_view.locked_titles.push_back(AchievementDisplayName_(id));
        }
    }
    if (achievement_view.total_count == 0) {
        achievement_view.total_count = 1;
        achievement_view.locked_titles.push_back(
            runtime.Config().GetString("achievement_empty_text", "待解锁成就"));
    }
    hud.UpdateAchievementPanel(achievement_view);
}

void HudPanelPresenters::UpdateSpiritBeastPanel(PixelGameHud& hud, GameRuntime& runtime) {
    const auto& world_state = runtime.WorldState();
    SpiritBeastPanelViewData beast_view;
    const auto& beast = world_state.GetSpiritBeast();
    beast_view.beast_name = beast.custom_name;
    beast_view.favor = beast.favor;
    beast_view.dispatched = beast.dispatched_for_pest_control;
    beast_view.trait = beast.trait;
    beast_view.category_text = runtime.Config().GetString("spirit_beast_category_text", beast_view.category_text);
    beast_view.cards_title = runtime.Config().GetString("spirit_beast_cards_title", beast_view.cards_title);
    beast_view.influence_hint_text = runtime.Config().GetString("spirit_beast_influence_hint_text", beast_view.influence_hint_text);
    beast_view.recruit_hint_text = runtime.Config().GetString("spirit_beast_recruit_hint_text", beast_view.recruit_hint_text);
    beast_view.actions_text = runtime.Config().GetString("spirit_beast_actions_text", beast_view.actions_text);
    beast_view.dispatch_in_progress_text = runtime.Config().GetString("spirit_beast_dispatch_in_progress_text", beast_view.dispatch_in_progress_text);
    beast_view.dispatch_idle_text = runtime.Config().GetString("spirit_beast_dispatch_idle_text", beast_view.dispatch_idle_text);
    beast_view.state_text = runtime.Config().GetString("spirit_beast_state_idle_text", "待机");
    switch (beast.state) {
    case SpiritBeastState::Idle: beast_view.state_text = runtime.Config().GetString("spirit_beast_state_idle_text", "待机"); break;
    case SpiritBeastState::Wander: beast_view.state_text = runtime.Config().GetString("spirit_beast_state_wander_text", "巡游"); break;
    case SpiritBeastState::Follow: beast_view.state_text = runtime.Config().GetString("spirit_beast_state_follow_text", "跟随"); break;
    case SpiritBeastState::Interact: beast_view.state_text = runtime.Config().GetString("spirit_beast_state_interact_text", "互动"); break;
    }
    beast_view.dispatch_remaining_seconds = runtime.SpiritBeastDispatchRemainingSeconds();
    hud.UpdateSpiritBeastPanel(beast_view);
}

void HudPanelPresenters::UpdateBuildingPanel(PixelGameHud& hud, GameRuntime& runtime) {
    const auto& world_state = runtime.WorldState();
    BuildingPanelViewData building_view;
    building_view.player_gold = world_state.GetGold();
    building_view.main_house_level = world_state.GetMainHouseRepair().level;
    building_view.greenhouse_unlocked = world_state.GetGreenhouseUnlocked();
    building_view.workshop_level = runtime.Systems().GetWorkshop().Level();
    building_view.category_prefix = runtime.Config().GetString(
        "building_category_prefix", building_view.category_prefix);
    building_view.list_title = runtime.Config().GetString(
        "building_list_title", building_view.list_title);
    building_view.upgrade_requirement_text = runtime.Config().GetString(
        "building_upgrade_requirement_text", building_view.upgrade_requirement_text);
    building_view.preview_text = runtime.Config().GetString(
        "building_preview_text", building_view.preview_text);
    building_view.actions_text = runtime.Config().GetString(
        "building_actions_text", building_view.actions_text);
    const auto& weekly_reports = world_state.GetWeeklyReports();
    std::string weekly_report_line = "本周经营报告  暂无（每 7 天自动生成）";
    if (!weekly_reports.empty()) {
        weekly_report_line = "本周经营报告  ";
        for (std::size_t i = 0; i < weekly_reports.size(); ++i) {
            if (i > 0) {
                weekly_report_line += " || ";
            }
            weekly_report_line += weekly_reports[i];
        }
    }
    building_view.building_lines = {
        runtime.Config().GetString(
            "building_line_main_house",
            "主屋 Lv" + std::to_string(building_view.main_house_level) + "   状态: 可升级   效果: 解锁更多系统"),
        runtime.Config().GetString(
            "building_line_greenhouse",
            "温室 " + std::string(building_view.greenhouse_unlocked ? "已解锁" : "未解锁")
                + "   状态: " + (building_view.greenhouse_unlocked ? "可使用" : "需主屋升级")),
        runtime.Config().GetString(
            "building_line_workshop",
            "工坊 Lv" + std::to_string(building_view.workshop_level) + "   状态: 可升级   效果: 队列 +1"),
        runtime.Config().GetString(
            "building_line_livestock",
            "畜棚 今日产出  鸡蛋 x" + std::to_string(world_state.GetLivestockEggsToday())
                + "  牛奶 x" + std::to_string(world_state.GetLivestockMilkToday())
                + "  喂养计数 " + std::to_string(world_state.GetCoopFedToday())),
        runtime.Config().GetString(
            "building_line_inn",
            "客栈 今日来访 " + std::to_string(world_state.GetInnVisitorsToday())
                + "  日结 +" + std::to_string(world_state.GetInnIncomeToday())
                + "  口碑 " + std::to_string(world_state.GetInnReputation())),
        runtime.Config().GetString(
            "building_line_decor_business",
            "装饰评分 " + std::to_string(world_state.GetDecorationScore())
                + "  -> 客栈来访 +" + std::to_string(std::clamp(world_state.GetDecorationScore(), 0, 60) / 2) + "%  "
                + "畜棚: 鸡蛋+(" + std::string(world_state.GetDecorationScore() >= 30 ? "+1" : "+0") + ")  "
                + "牛奶阶梯 " + std::string(world_state.GetDecorationScore() >= 45 ? "3档" : (world_state.GetDecorationScore() >= 20 ? "2档" : "1档"))),
        runtime.Config().GetString("building_line_weekly_report", weekly_report_line),
    };
    if (building_view.main_house_level < kMainHouseMaxLevel) {
        const auto cost = QueryMainHouseUpgradeCost(building_view.main_house_level);
        building_view.upgrade_requirement_text =
            "下一阶主屋 Lv" + std::to_string(cost.next_level)
            + " 需要：木材 x" + std::to_string(cost.wood_cost)
            + "  萝卜 x" + std::to_string(cost.turnip_cost)
            + "  金币 x" + std::to_string(cost.gold_cost);
    } else {
        building_view.upgrade_requirement_text = "主屋已满级（大宅）。";
    }
    hud.UpdateBuildingPanel(building_view);
}

void HudPanelPresenters::UpdateContractPanel(PixelGameHud& hud, GameRuntime& runtime) {
    ContractPanelViewData contract_view;
    const auto& contracts = runtime.Systems().GetContracts();
    contract_view.volumes_line_prefix = runtime.Config().GetString("contract_volumes_line_prefix", contract_view.volumes_line_prefix);
    contract_view.tracking_line_prefix = runtime.Config().GetString("contract_tracking_line_prefix", contract_view.tracking_line_prefix);
    contract_view.tracking_name_separator = runtime.Config().GetString("contract_tracking_name_separator", contract_view.tracking_name_separator);
    contract_view.bonus_prefix = runtime.Config().GetString("contract_bonus_prefix", contract_view.bonus_prefix);
    contract_view.tasks_title = runtime.Config().GetString("contract_tasks_title", contract_view.tasks_title);
    contract_view.chapter_reward_hint_text = runtime.Config().GetString("contract_chapter_reward_hint_text", contract_view.chapter_reward_hint_text);
    contract_view.today_recommendation_title = runtime.Config().GetString("contract_today_recommendation_title", contract_view.today_recommendation_title);
    contract_view.today_recommendation_text = contracts.TodayRecommendation();
    contract_view.completed_volumes = contracts.CompletedVolumeCount();
    contract_view.total_volumes = static_cast<int>(contracts.Volumes().size());
    {
        int next_locked = -1;
        for (const auto& v : contracts.Volumes()) {
            if (!v.unlocked) { next_locked = v.volume_id; break; }
        }
        if (next_locked > 1) {
            contract_view.unlock_hint_text =
                "未解锁提示：完成第" + std::to_string(next_locked - 1) + "卷后解锁第" + std::to_string(next_locked) + "卷。";
        } else if (next_locked == 1) {
            contract_view.unlock_hint_text = "未解锁提示：请推进契约卷册。";
        } else {
            contract_view.unlock_hint_text.clear();
        }
    }
    if (const auto* tracking = contracts.GetTrackingVolume()) {
        contract_view.tracking_volume_id = tracking->volume_id;
        contract_view.tracking_volume_name = tracking->name;
        contract_view.tracking_bonus = tracking->permanent_bonus_description;
        contract_view.task_lines.clear();
        const std::size_t count = std::min<std::size_t>(3, tracking->items.size());
        for (std::size_t i = 0; i < count; ++i) {
            const auto& item = tracking->items[i];
            const std::string prefix = item.completed ? "✓ " : "○ ";
            contract_view.task_lines.push_back(prefix + item.name + "  需求: " + std::to_string(item.required_count));
        }
    } else {
        contract_view.tracking_volume_id = 1;
        contract_view.tracking_volume_name = "未解锁";
        contract_view.tracking_bonus = "完成前置卷解锁后可追踪";
        contract_view.task_lines = {"（未解锁）", "提示：完成上一卷全部条目后自动解锁下一卷"};
    }
    hud.UpdateContractPanel(contract_view);
}

void HudPanelPresenters::UpdateNpcDetailPanel(PixelGameHud& hud, GameRuntime& runtime) {
    const auto& world_state = runtime.WorldState();
    NpcDetailPanelViewData npc_view;
    const auto& npcs = world_state.GetNpcs();
    const int highlighted_npc = world_state.GetInteraction().highlighted_npc_index;
    const NpcActor* selected_npc = nullptr;
    if (highlighted_npc >= 0 && highlighted_npc < static_cast<int>(npcs.size())) {
        selected_npc = &npcs[static_cast<std::size_t>(highlighted_npc)];
    } else if (!npcs.empty()) {
        selected_npc = &npcs.front();
    }
    if (selected_npc != nullptr) {
        npc_view.name = selected_npc->display_name;
        npc_view.heart_level = selected_npc->heart_level;
        npc_view.favor = selected_npc->favor;
        npc_view.talked_today = selected_npc->daily_talked;
        npc_view.gifted_today = selected_npc->daily_gifted;
        npc_view.location = (selected_npc->heart_level >= 4)
            ? (selected_npc->current_location.empty() ? "云海山庄" : selected_npc->current_location)
            : "???（好感4级解锁）";
        npc_view.cloud_stage_text = NpcCloudStageText(selected_npc->heart_level);
        npc_view.legendary_style_unlocked = selected_npc->heart_level >= 10;
    }
    npc_view.title_suffix = runtime.Config().GetString("npc_detail_title_suffix", npc_view.title_suffix);
    npc_view.location_prefix = runtime.Config().GetString("npc_detail_location_prefix", npc_view.location_prefix);
    npc_view.favor_prefix = runtime.Config().GetString("npc_detail_favor_prefix", npc_view.favor_prefix);
    npc_view.heart_event_text = runtime.Config().GetString("npc_detail_heart_event_text", npc_view.heart_event_text);
    npc_view.talked_done_text = runtime.Config().GetString("npc_detail_talked_done_text", npc_view.talked_done_text);
    npc_view.talked_todo_text = runtime.Config().GetString("npc_detail_talked_todo_text", npc_view.talked_todo_text);
    npc_view.gifted_done_text = runtime.Config().GetString("npc_detail_gifted_done_text", npc_view.gifted_done_text);
    npc_view.gifted_todo_text = runtime.Config().GetString("npc_detail_gifted_todo_text", npc_view.gifted_todo_text);
    npc_view.event_hint_text = runtime.Config().GetString("npc_detail_event_hint_text", npc_view.event_hint_text);
    npc_view.actions_text = runtime.Config().GetString("npc_detail_actions_text", npc_view.actions_text);
    hud.UpdateNpcDetailPanel(npc_view);
}

void HudPanelPresenters::UpdateSpiritRealmPanel(PixelGameHud& hud, GameRuntime& runtime) {
    const auto& world_state = runtime.WorldState();
    SpiritRealmPanelViewData spirit_realm_view;
    spirit_realm_view.mode_text = runtime.Config().GetString("spirit_realm_mode_text", "轻松");
    spirit_realm_view.max_count = world_state.GetSpiritRealmDailyMax();
    spirit_realm_view.in_spirit_realm = world_state.GetInSpiritRealm();
    spirit_realm_view.remaining_count = world_state.GetSpiritRealmDailyRemaining();
    spirit_realm_view.drop_bonus_percent = std::max(0, static_cast<int>((runtime.CloudMultiplier() - 1.0f) * 100.0f));
    spirit_realm_view.mode_line_prefix = runtime.Config().GetString(
        "spirit_realm_mode_prefix", spirit_realm_view.mode_line_prefix);
    spirit_realm_view.mode_options_text = runtime.Config().GetString(
        "spirit_realm_mode_options_text", spirit_realm_view.mode_options_text);
    spirit_realm_view.remaining_line_prefix = runtime.Config().GetString(
        "spirit_realm_remaining_prefix", spirit_realm_view.remaining_line_prefix);
    spirit_realm_view.drop_bonus_prefix = runtime.Config().GetString(
        "spirit_realm_drop_bonus_prefix", spirit_realm_view.drop_bonus_prefix);
    spirit_realm_view.regions_title = runtime.Config().GetString(
        "spirit_realm_regions_title", spirit_realm_view.regions_title);
    spirit_realm_view.default_region_line_1 = runtime.Config().GetString(
        "spirit_realm_default_region_line_1", spirit_realm_view.default_region_line_1);
    spirit_realm_view.default_region_line_2 = runtime.Config().GetString(
        "spirit_realm_default_region_line_2", spirit_realm_view.default_region_line_2);
    spirit_realm_view.default_region_line_3 = runtime.Config().GetString(
        "spirit_realm_default_region_line_3", spirit_realm_view.default_region_line_3);
    spirit_realm_view.active_state_in_realm_text = runtime.Config().GetString(
        "spirit_realm_active_state_in_realm_text", spirit_realm_view.active_state_in_realm_text);
    spirit_realm_view.active_state_unlocked_text = runtime.Config().GetString(
        "spirit_realm_active_state_unlocked_text", spirit_realm_view.active_state_unlocked_text);
    spirit_realm_view.active_state_suffix = runtime.Config().GetString(
        "spirit_realm_active_state_suffix", spirit_realm_view.active_state_suffix);
    spirit_realm_view.lock_hint_text = runtime.Config().GetString(
        "spirit_realm_lock_hint_text", spirit_realm_view.lock_hint_text);
    spirit_realm_view.actions_text = runtime.Config().GetString(
        "spirit_realm_actions_text", spirit_realm_view.actions_text);
    spirit_realm_view.region_lines = {
        runtime.Config().GetString("spirit_realm_region_line_1", "浅层云径  Lv8   掉落: 灵尘/雾草"),
        runtime.Config().GetString("spirit_realm_region_line_2", "潮汐裂谷  Lv15  状态: 🔒 需契约卷3"),
        runtime.Config().GetString("spirit_realm_region_line_3", "霜岚祭坛  Lv22  状态: 🔒 需山庄等级5"),
    };
    hud.UpdateSpiritRealmPanel(spirit_realm_view);
}

void HudPanelPresenters::UpdateBeastiaryPanel(PixelGameHud& hud, GameRuntime& runtime) {
    const auto& world_state = runtime.WorldState();
    BeastiaryPanelViewData beastiary_view;
    beastiary_view.filter_text = runtime.Config().GetString(
        "beastiary_filter_text", beastiary_view.filter_text);
    beastiary_view.progress_prefix = runtime.Config().GetString(
        "beastiary_progress_prefix", beastiary_view.progress_prefix);
    const std::array<std::string, 3> tracked_pet_types{"cat", "dog", "bird"};
    const auto& achievements_map = world_state.GetAchievements();
    beastiary_view.total_count = static_cast<int>(tracked_pet_types.size());
    beastiary_view.discovered_count = 0;
    for (const auto& type : tracked_pet_types) {
        const auto it = achievements_map.find("pet_collected_" + type);
        const bool discovered = (it != achievements_map.end() && it->second);
        if (discovered) {
            ++beastiary_view.discovered_count;
            beastiary_view.discovered_lines.push_back(
                PetDisplayName_(type) + "      已发现   区域: 云海山庄");
        } else {
            beastiary_view.undiscovered_lines.push_back(
                "???         未发现   条件: 解锁" + PetDisplayName_(type) + "认养");
        }
    }
    if (world_state.GetPetAdopted()) {
        beastiary_view.selected_detail =
            "选中详情: 已结缘灵兽 " + PetDisplayName_(world_state.GetPetType());
    } else {
        beastiary_view.selected_detail = "选中详情: 尚未结缘灵兽";
    }
    hud.UpdateBeastiaryPanel(beastiary_view);
}

void HudPanelPresenters::UpdateWorkshopPanel(PixelGameHud& hud, GameRuntime& runtime) {
    const auto& world_state = runtime.WorldState();
    WorkshopPanelViewData workshop_view;
    const auto& workshop = runtime.Systems().GetWorkshop();
    const auto& machine = world_state.GetTeaMachine();
    workshop_view.auto_craft = machine.running;
    workshop_view.workshop_level = workshop.Level();
    workshop_view.unlocked_slots = workshop.UnlockedSlots();
    workshop_view.tabs_text = runtime.Config().GetString("workshop_tabs_text", workshop_view.tabs_text);
    workshop_view.queue_title_text = runtime.Config().GetString("workshop_queue_title_text", workshop_view.queue_title_text);
    workshop_view.queue_primary_prefix = runtime.Config().GetString("workshop_queue_primary_prefix", workshop_view.queue_primary_prefix);
    workshop_view.queue_progress_suffix = runtime.Config().GetString("workshop_queue_progress_suffix", workshop_view.queue_progress_suffix);
    workshop_view.empty_slot_line_2 = runtime.Config().GetString("workshop_empty_slot_line_2", workshop_view.empty_slot_line_2);
    workshop_view.empty_slot_line_3 = runtime.Config().GetString("workshop_empty_slot_line_3", workshop_view.empty_slot_line_3);
    workshop_view.stock_prefix = runtime.Config().GetString("workshop_stock_prefix", workshop_view.stock_prefix);
    workshop_view.stock_tea_label = runtime.Config().GetString("workshop_stock_tea_label", workshop_view.stock_tea_label);
    workshop_view.stock_wood_label = runtime.Config().GetString("workshop_stock_wood_label", workshop_view.stock_wood_label);
    workshop_view.stock_crystal_label = runtime.Config().GetString("workshop_stock_crystal_label", workshop_view.stock_crystal_label);
    workshop_view.actions_text = runtime.Config().GetString("workshop_actions_text", workshop_view.actions_text);
    workshop_view.active_recipe = runtime.Config().GetString("workshop_idle_recipe_text", "待命");
    const auto& machines = workshop.GetMachines();
    workshop_view.queue_lines.clear();
    const std::size_t line_count = std::min<std::size_t>(3, machines.size());
    for (std::size_t i = 0; i < line_count; ++i) {
        const auto& m = machines[i];
        std::string recipe_name = runtime.Config().GetString("workshop_idle_recipe_text", "待命");
        if (!m.recipe_id.empty()) {
            if (const auto* recipe = workshop.GetRecipe(m.recipe_id)) {
                recipe_name = recipe->name;
            } else {
                recipe_name = m.recipe_id;
            }
        }
        const int machine_progress_pct = static_cast<int>(std::max(0.0f, std::min(100.0f, m.progress)));
        const std::string stage_text = CloudSeamanor::domain::TeaProcessStageText(m.stage);
        const int quality_est = static_cast<int>(std::max(0.0f, m.quality_score * 10.0f));
        workshop_view.queue_lines.push_back(
            std::to_string(i + 1) + ") " + recipe_name + "  " + stage_text
            + "  " + std::to_string(machine_progress_pct) + "%  品质预估+" + std::to_string(quality_est)
            + "  " + (m.is_processing ? "加工中" : "空闲"));
    }
    while (workshop_view.queue_lines.size() < 3) {
        workshop_view.queue_lines.push_back(
            std::to_string(workshop_view.queue_lines.size() + 1) + ") 空槽位");
    }
    if (!machines.empty()) {
        const auto& first_machine = machines.front();
        if (!first_machine.recipe_id.empty()) {
            if (const auto* recipe = workshop.GetRecipe(first_machine.recipe_id)) {
                workshop_view.active_recipe = recipe->name;
            } else {
                workshop_view.active_recipe = first_machine.recipe_id;
            }
        }
    }
    int unlocked_recipe_count = 0;
    int locked_recipe_count = 0;
    for (const auto& machine_state : machines) {
        const auto recipes = workshop.GetRecipesForMachine(machine_state.machine_id);
        for (const auto* recipe : recipes) {
            if (recipe == nullptr) {
                continue;
            }
            if (workshop.IsRecipeUnlocked(*recipe)) {
                ++unlocked_recipe_count;
            } else {
                ++locked_recipe_count;
            }
        }
    }
    workshop_view.actions_text +=
        "  | 解锁配方 " + std::to_string(unlocked_recipe_count)
        + " / 锁定 " + std::to_string(locked_recipe_count)
        + "  | 下一目标：主屋升级提升工坊阶梯";
    workshop_view.queue_progress = machine.progress;
    workshop_view.queued_output = machine.queued_output;
    workshop_view.tea_leaf_stock = world_state.GetInventory().CountOf("TeaLeaf");
    workshop_view.wood_stock = world_state.GetInventory().CountOf("Wood");
    workshop_view.crystal_stock = world_state.GetInventory().CountOf("spirit_dust");
    hud.UpdateWorkshopPanel(workshop_view);
}

}  // namespace CloudSeamanor::engine

