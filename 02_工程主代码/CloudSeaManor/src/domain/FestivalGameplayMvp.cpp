#include "CloudSeamanor/FestivalGameplayMvp.hpp"

#include <unordered_map>

namespace CloudSeamanor::domain {

namespace {

int HashMix_(int a, int b, int c) noexcept {
    return (a * 92837111) ^ (b * 689287499) ^ (c * 100663319);
}

void PushUniqueStrings_(std::vector<std::string>& out, const std::vector<std::string>& add) {
    out.insert(out.end(), add.begin(), add.end());
}

int LookupGold_(const std::unordered_map<std::string, int>& table, const char* key, int fallback) {
    const auto it = table.find(key);
    return it == table.end() ? fallback : it->second;
}

}  // namespace

FestivalAutoRewardSpec BuildFestivalAutoRewardSpec(
    const std::string& festival_id,
    int game_day,
    int year) {
    FestivalAutoRewardSpec spec;
    if (festival_id.empty()) {
        spec.hint_lines.push_back("【节日】未识别节日 ID。");
        return spec;
    }
    const int h = HashMix_(static_cast<int>(festival_id.size()), game_day, year);

    static const std::unordered_map<std::string, int> kGoldTable{
        {"lantern_festival", 80},
        {"flower_festival", 60},
        {"qingming_festival", 90},
        {"dragon_boat", 100},
        {"qixi_festival", 70},
        {"mid_autumn", 120},
        {"double_ninth", 110},
        {"winter_solstice", 130},
        {"tea_culture_day", 85},
        {"harvest_festival", 140},
    };

    if (festival_id == "spring_festival" || festival_id == "spring_awakening") {
        spec.gold = 120 + (game_day % 3) * 40;
        PushUniqueStrings_(spec.hint_lines, {
            "【春节】开春红包已入袋：金币 +" + std::to_string(spec.gold) + "。",
            "今日可在节日摊位与工坊体验年节小玩法。"
        });
        return spec;
    }

    if (festival_id == "cloud_tide_ritual" || festival_id == "cloud_tide") {
        PushUniqueStrings_(spec.hint_lines, {
            "【大潮祭】云海共鸣达到顶点：前往灵界灵兽区域可触发祭典决战。",
            "潮祭期间战斗掉落与经验获得小幅提升（已在战场结算中生效）。"
        });
        return spec;
    }

    if (festival_id == "lantern_festival" || festival_id == "summer_lantern") {
        spec.gold = LookupGold_(kGoldTable, "lantern_festival", 80) + (h & 31);
        spec.stamina = 25 + (h % 15);
        spec.grant_items.emplace_back("TeaPack", 1);
        PushUniqueStrings_(spec.hint_lines, {
            "【元宵】灯谜会开张：到「节日摊位」猜谜可得汤圆礼包。",
            "已领取节日补给：金币 +" + std::to_string(spec.gold) + "，体力 +" + std::to_string(spec.stamina) + "，茶包 x1。"
        });
        return spec;
    }

    if (festival_id == "flower_festival") {
        spec.gold = LookupGold_(kGoldTable, "flower_festival", 60) + (h % 20);
        spec.favor_all_npcs = 3;
        spec.set_flower_bloom_visual = true;
        spec.grant_items.emplace_back("TeaSeed", 2);
        PushUniqueStrings_(spec.hint_lines, {
            "【花朝】云海染上花意：今日采集与社交都更轻松。",
            "花神小礼：金币 +" + std::to_string(spec.gold) + "，种子 x2，全员好感微升。"
        });
        return spec;
    }

    if (festival_id == "qingming_festival") {
        spec.gold = LookupGold_(kGoldTable, "qingming_festival", 90);
        spec.set_qingming_double_spirit = true;
        PushUniqueStrings_(spec.hint_lines, {
            "【清明】踏青双倍：今日灵界「灵草」采集量翻倍。",
            "到「节日摊位」可完成祭祖小仪，获得额外谢礼。"
        });
        return spec;
    }

    if (festival_id == "dragon_boat") {
        spec.gold = LookupGold_(kGoldTable, "dragon_boat", 100) + (h % 25);
        spec.stamina = 20;
        spec.grant_items.emplace_back("Feed", 2);
        PushUniqueStrings_(spec.hint_lines, {
            "【端午】香囊与龙舟赛：摊位可包粽子，体力 +" + std::to_string(spec.stamina) + "。",
            "（占位）香囊已放入背包，前往工坊可配合茶叶制作节庆茶点。"
        });
        return spec;
    }

    if (festival_id == "qixi_festival") {
        spec.gold = LookupGold_(kGoldTable, "qixi_festival", 70) + (h % 40);
        spec.favor_all_npcs = 5;
        PushUniqueStrings_(spec.hint_lines, {
            "【七夕】星河入梦：摊位「许愿」可获得随机缘分礼。",
            "结缘礼：金币 +" + std::to_string(spec.gold) + "，好感微升。"
        });
        return spec;
    }

    if (festival_id == "mid_autumn") {
        spec.gold = LookupGold_(kGoldTable, "mid_autumn", 120);
        spec.grant_items.emplace_back("TeaPack", 2);
        spec.mid_autumn_regen_bonus_until_day = game_day + 2;
        PushUniqueStrings_(spec.hint_lines, {
            "【中秋】赏月食饼：获得月饼（茶包 x2）与 3 日「月相」体力滋养。",
            "三日内每日起床额外恢复体力（在换日时结算）。"
        });
        return spec;
    }

    if (festival_id == "double_ninth") {
        spec.gold = LookupGold_(kGoldTable, "double_ninth", 110);
        spec.set_double_ninth_chrysanthemum = true;
        spec.stamina = 35;
        PushUniqueStrings_(spec.hint_lines, {
            "【重阳】登高饮菊：今日体力恢复获得菊花酒加成。",
            "到「节日摊位」可触发登高小事件。"
        });
        return spec;
    }

    if (festival_id == "winter_solstice") {
        spec.gold = LookupGold_(kGoldTable, "winter_solstice", 130);
        spec.stamina = 45;
        spec.set_winter_polar_night = true;
        spec.grant_items.emplace_back("TeaPack", 1);
        spec.grant_items.emplace_back("FertilizerItem", 1);
        PushUniqueStrings_(spec.hint_lines, {
            "【冬至】极夜团圆：暖食补给已发放，体力 +" + std::to_string(spec.stamina) + "。",
            "山庄进入短暂极夜视觉（仅今日）。"
        });
        return spec;
    }

    if (festival_id == "tea_culture_day") {
        spec.gold = LookupGold_(kGoldTable, "tea_culture_day", 85);
        spec.set_tea_culture_contest = true;
        spec.grant_items.emplace_back("TeaPack", 1);
        PushUniqueStrings_(spec.hint_lines, {
            "【茶文化节】斗茶评分开放：摊位可进行一轮品质评定。",
            "茶艺补给：金币 +" + std::to_string(spec.gold) + "，茶包 x1。"
        });
        return spec;
    }

    if (festival_id == "harvest_festival" || festival_id == "autumn_harvest") {
        spec.gold = LookupGold_(kGoldTable, "harvest_festival", 140) + (h % 30);
        spec.set_harvest_sell_bonus = true;
        PushUniqueStrings_(spec.hint_lines, {
            "【丰收祭】收购价提升：今日向收购商出售作物额外 +15% 金币。",
            "到「节日摊位」可登记今日收成评选。"
        });
        return spec;
    }

    // 默认：山庄内置节日等
    spec.gold = 40 + (h % 30);
    PushUniqueStrings_(spec.hint_lines, {
        "【节日活动】" + festival_id + " 已激活，金币 +" + std::to_string(spec.gold) + "。",
        "今日可前往节日商店与摊位体验玩法。"
    });
    return spec;
}

}  // namespace CloudSeamanor::domain
