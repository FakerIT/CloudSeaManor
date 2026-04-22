#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace yhsz::save {

using json = nlohmann::json;

// Generic wrapper for entries like: { "__type": "...", "value": ... }
template <typename T>
struct TypedValue {
    std::string type_name;
    T value{};
};

template <typename T>
inline void to_json(json& j, const TypedValue<T>& v) {
    j = json{{"__type", v.type_name}, {"value", v.value}};
}

template <typename T>
inline void from_json(const json& j, TypedValue<T>& v) {
    v.type_name = j.value("__type", "");
    if (j.contains("value")) {
        v.value = j.at("value").get<T>();
    }
}

struct SkillData {
    std::string skill_name;   // "技能名"
    int32_t skill_level = 1;  // "技能等级"
    bool is_passive = false;  // "是被动"
};

inline void to_json(json& j, const SkillData& v) {
    j = json{
        {"技能名", v.skill_name},
        {"技能等级", v.skill_level},
        {"是被动", v.is_passive},
    };
}

inline void from_json(const json& j, SkillData& v) {
    v.skill_name = j.value("技能名", "");
    v.skill_level = j.value("技能等级", 1);
    v.is_passive = j.value("是被动", false);
}

struct AffixData {
    std::string affix_name;                // "词条名称"
    std::string affix_desc;                // "词条描述"
    int32_t type = 0;                      // "类型"
    int32_t quality = 0;                   // "品质"
    float bonus_value = 0.0f;              // "属性增伤"
    float bonus_percent = 0.0f;            // "属性增伤百分比"
    std::string keyword;                   // "属性关键词"
    float lifesteal_decay = 0.0f;          // "吸血衰减率"
    int32_t target_buff = 0;               // "目标BUFF"
    int32_t extra_gain = 0;                // "额外获得"
    int32_t extra_gain_count = 0;          // "额外获得数量"
    float custom_float_1 = 0.0f;           // "自定义浮点一"
    float custom_float_2 = 0.0f;           // "自定义浮点二"
    bool positive = true;                  // "正面词条"
    bool target_hp = true;                 // "目标生命"
    bool custom_bool = false;              // "自定义布尔"
    int32_t rarity = 0;                    // "稀有度"
};

inline void to_json(json& j, const AffixData& v) {
    j = json{
        {"词条名称", v.affix_name},
        {"词条描述", v.affix_desc},
        {"类型", v.type},
        {"品质", v.quality},
        {"属性增伤", v.bonus_value},
        {"属性增伤百分比", v.bonus_percent},
        {"属性关键词", v.keyword},
        {"吸血衰减率", v.lifesteal_decay},
        {"目标BUFF", v.target_buff},
        {"额外获得", v.extra_gain},
        {"额外获得数量", v.extra_gain_count},
        {"自定义浮点一", v.custom_float_1},
        {"自定义浮点二", v.custom_float_2},
        {"正面词条", v.positive},
        {"目标生命", v.target_hp},
        {"自定义布尔", v.custom_bool},
        {"稀有度", v.rarity},
    };
}

inline void from_json(const json& j, AffixData& v) {
    v.affix_name = j.value("词条名称", "");
    v.affix_desc = j.value("词条描述", "");
    v.type = j.value("类型", 0);
    v.quality = j.value("品质", 0);
    v.bonus_value = j.value("属性增伤", 0.0f);
    v.bonus_percent = j.value("属性增伤百分比", 0.0f);
    v.keyword = j.value("属性关键词", "");
    v.lifesteal_decay = j.value("吸血衰减率", 0.0f);
    v.target_buff = j.value("目标BUFF", 0);
    v.extra_gain = j.value("额外获得", 0);
    v.extra_gain_count = j.value("额外获得数量", 0);
    v.custom_float_1 = j.value("自定义浮点一", 0.0f);
    v.custom_float_2 = j.value("自定义浮点二", 0.0f);
    v.positive = j.value("正面词条", true);
    v.target_hp = j.value("目标生命", true);
    v.custom_bool = j.value("自定义布尔", false);
    v.rarity = j.value("稀有度", 0);
}

struct ItemData {
    bool is_equip = false;                    // "是装备"
    std::string equip_name;                   // "装备名字"
    std::string prefab_name;                  // "预制体名字"
    std::string main_desc;                    // "装备主描述"
    int32_t level = 1;                        // "等级"
    int32_t quality = 0;                      // "品质"
    int32_t min_phy = 0;                      // "最小物理伤害"
    int32_t max_phy = 0;                      // "最大物理伤害"
    int32_t min_mag = 0;                      // "最小魔法伤害"
    int32_t max_mag = 0;                      // "最大魔法伤害"
    int32_t defense = 0;                      // "防御"
    float move_speed = 0.0f;                  // "移动速度"
    float crit_rate = 0.0f;                   // "暴击概率"
    float crit_damage = 0.0f;                 // "暴击伤害"
    float dodge_rate = 0.0f;                  // "闪避率"
    int32_t count = 1;                        // "numeber"
    int32_t price = 0;                        // "priece"
    int32_t slot_index = -1;                  // "格子索引"
    std::vector<AffixData> affixes;           // "词条列表"
};

inline void to_json(json& j, const ItemData& v) {
    j = json{
        {"是装备", v.is_equip},
        {"装备名字", v.equip_name},
        {"预制体名字", v.prefab_name},
        {"装备主描述", v.main_desc},
        {"等级", v.level},
        {"品质", v.quality},
        {"最小物理伤害", v.min_phy},
        {"最大物理伤害", v.max_phy},
        {"最小魔法伤害", v.min_mag},
        {"最大魔法伤害", v.max_mag},
        {"防御", v.defense},
        {"移动速度", v.move_speed},
        {"暴击概率", v.crit_rate},
        {"暴击伤害", v.crit_damage},
        {"闪避率", v.dodge_rate},
        {"numeber", v.count},
        {"priece", v.price},
        {"格子索引", v.slot_index},
        {"词条列表", v.affixes},
    };
}

inline void from_json(const json& j, ItemData& v) {
    v.is_equip = j.value("是装备", false);
    v.equip_name = j.value("装备名字", "");
    v.prefab_name = j.value("预制体名字", "");
    v.main_desc = j.value("装备主描述", "");
    v.level = j.value("等级", 1);
    v.quality = j.value("品质", 0);
    v.min_phy = j.value("最小物理伤害", 0);
    v.max_phy = j.value("最大物理伤害", 0);
    v.min_mag = j.value("最小魔法伤害", 0);
    v.max_mag = j.value("最大魔法伤害", 0);
    v.defense = j.value("防御", 0);
    v.move_speed = j.value("移动速度", 0.0f);
    v.crit_rate = j.value("暴击概率", 0.0f);
    v.crit_damage = j.value("暴击伤害", 0.0f);
    v.dodge_rate = j.value("闪避率", 0.0f);
    v.count = j.value("numeber", 1);
    v.price = j.value("priece", 0);
    v.slot_index = j.value("格子索引", -1);
    if (j.contains("词条列表")) {
        v.affixes = j.at("词条列表").get<std::vector<AffixData>>();
    }
}

struct CharacterData {
    std::string name;               // "Cname"
    bool is_boy = true;             // "isboy"
    int32_t level = 1;
    int32_t min_phy = 0;            // "最小物理伤害"
    int32_t max_phy = 0;            // "最大物理伤害"
    int32_t min_mag = 0;            // "最小魔法伤害"
    int32_t max_mag = 0;            // "最大魔法伤害"
    int32_t defense = 0;
    float move_speed = 0.0f;        // "移动速度"
    float crit_rate = 0.0f;         // "暴击概率"
    float crit_damage = 0.0f;       // "暴击伤害"
    float dodge_rate = 0.0f;        // "闪避率"
    int32_t hp = 0;
    int32_t max_hp = 0;
    int32_t mp = 0;
    int32_t max_mp = 0;
    int32_t exp = 0;
    int32_t max_exp = 0;
    float growth = 1.0f;            // "成长"
    int32_t role_job = 0;           // "角色职业"
    int32_t portrait_index = 0;     // "立绘索引"
    int32_t role_id = 0;            // "角色ID"
    std::optional<SkillData> normal_skill;   // "普攻"
    std::optional<SkillData> battle_skill;   // "战技"
    std::optional<SkillData> finisher_skill; // "终结技"
    std::optional<SkillData> passive_skill;  // "被动技能"
};

inline void to_json(json& j, const CharacterData& v) {
    j = json{
        {"Cname", v.name},
        {"isboy", v.is_boy},
        {"level", v.level},
        {"最小物理伤害", v.min_phy},
        {"最大物理伤害", v.max_phy},
        {"最小魔法伤害", v.min_mag},
        {"最大魔法伤害", v.max_mag},
        {"防御", v.defense},
        {"移动速度", v.move_speed},
        {"暴击概率", v.crit_rate},
        {"暴击伤害", v.crit_damage},
        {"闪避率", v.dodge_rate},
        {"hp", v.hp},
        {"maxhp", v.max_hp},
        {"mp", v.mp},
        {"maxmp", v.max_mp},
        {"exp", v.exp},
        {"maxexp", v.max_exp},
        {"成长", v.growth},
        {"角色职业", v.role_job},
        {"立绘索引", v.portrait_index},
        {"角色ID", v.role_id},
        {"普攻", v.normal_skill},
        {"战技", v.battle_skill},
        {"终结技", v.finisher_skill},
        {"被动技能", v.passive_skill},
    };
}

inline void from_json(const json& j, CharacterData& v) {
    v.name = j.value("Cname", "");
    v.is_boy = j.value("isboy", true);
    v.level = j.value("level", 1);
    v.min_phy = j.value("最小物理伤害", 0);
    v.max_phy = j.value("最大物理伤害", 0);
    v.min_mag = j.value("最小魔法伤害", 0);
    v.max_mag = j.value("最大魔法伤害", 0);
    v.defense = j.value("防御", 0);
    v.move_speed = j.value("移动速度", 0.0f);
    v.crit_rate = j.value("暴击概率", 0.0f);
    v.crit_damage = j.value("暴击伤害", 0.0f);
    v.dodge_rate = j.value("闪避率", 0.0f);
    v.hp = j.value("hp", 0);
    v.max_hp = j.value("maxhp", 0);
    v.mp = j.value("mp", 0);
    v.max_mp = j.value("maxmp", 0);
    v.exp = j.value("exp", 0);
    v.max_exp = j.value("maxexp", 0);
    v.growth = j.value("成长", 1.0f);
    v.role_job = j.value("角色职业", 0);
    v.portrait_index = j.value("立绘索引", 0);
    v.role_id = j.value("角色ID", 0);
    if (j.contains("普攻") && !j.at("普攻").is_null()) v.normal_skill = j.at("普攻").get<SkillData>();
    if (j.contains("战技") && !j.at("战技").is_null()) v.battle_skill = j.at("战技").get<SkillData>();
    if (j.contains("终结技") && !j.at("终结技").is_null()) v.finisher_skill = j.at("终结技").get<SkillData>();
    if (j.contains("被动技能") && !j.at("被动技能").is_null()) v.passive_skill = j.at("被动技能").get<SkillData>();
}

struct SystemSaveData {
    float bgm_volume = 0.5f;       // "BGM音量"
    float sfx_volume = 0.5f;       // "音效音量"
    bool mute_on_hide = true;      // "隐藏后静音"
    int32_t display_setting = 0;   // "显示设置"
    bool always_on_top = false;    // "始终置顶"
    bool show_damage_num = true;   // "显示伤害数字"
    bool window_mode = false;      // "是窗口模式"
    int32_t next_role_id = 0;      // "下一个角色ID"
};

inline void to_json(json& j, const SystemSaveData& v) {
    j = json{
        {"BGM音量", v.bgm_volume},
        {"音效音量", v.sfx_volume},
        {"隐藏后静音", v.mute_on_hide},
        {"显示设置", v.display_setting},
        {"始终置顶", v.always_on_top},
        {"显示伤害数字", v.show_damage_num},
        {"是窗口模式", v.window_mode},
        {"下一个角色ID", v.next_role_id},
    };
}

inline void from_json(const json& j, SystemSaveData& v) {
    v.bgm_volume = j.value("BGM音量", 0.5f);
    v.sfx_volume = j.value("音效音量", 0.5f);
    v.mute_on_hide = j.value("隐藏后静音", true);
    v.display_setting = j.value("显示设置", 0);
    v.always_on_top = j.value("始终置顶", false);
    v.show_damage_num = j.value("显示伤害数字", true);
    v.window_mode = j.value("是窗口模式", false);
    v.next_role_id = j.value("下一个角色ID", 0);
}

struct MainSaveData {
    std::vector<CharacterData> player_zone;         // "玩家区"
    std::vector<CharacterData> recruit_zone;        // "招募区"
    std::vector<ItemData> inventory_1;              // "玩家库存1"
};

inline void to_json(json& j, const MainSaveData& v) {
    j = json{
        {"玩家区", v.player_zone},
        {"招募区", v.recruit_zone},
        {"玩家库存1", v.inventory_1},
    };
}

inline void from_json(const json& j, MainSaveData& v) {
    if (j.contains("玩家区")) v.player_zone = j.at("玩家区").get<std::vector<CharacterData>>();
    if (j.contains("招募区")) v.recruit_zone = j.at("招募区").get<std::vector<CharacterData>>();
    if (j.contains("玩家库存1")) v.inventory_1 = j.at("玩家库存1").get<std::vector<ItemData>>();
}

// Top-level save root. Unknown keys are stored in extras for forward compatibility.
struct SaveRoot {
    TypedValue<double> offline_last_ts;     // "离线收益_上次时间戳"
    TypedValue<bool> have_save0;            // "HaveSave0"
    TypedValue<bool> deleted_save0;         // "DeletedSave0"
    TypedValue<SystemSaveData> system_save; // "SaveS"
    TypedValue<int32_t> language;           // "语言"
    TypedValue<int32_t> selected_slot;      // "SelectSave"
    TypedValue<MainSaveData> save0;         // "Save0"

    std::unordered_map<std::string, json> extras;
};

inline void to_json(json& j, const SaveRoot& v) {
    j = json{
        {"离线收益_上次时间戳", v.offline_last_ts},
        {"HaveSave0", v.have_save0},
        {"DeletedSave0", v.deleted_save0},
        {"SaveS", v.system_save},
        {"语言", v.language},
        {"SelectSave", v.selected_slot},
        {"Save0", v.save0},
    };
    for (const auto& [k, val] : v.extras) {
        j[k] = val;
    }
}

inline void from_json(const json& j, SaveRoot& v) {
    if (j.contains("离线收益_上次时间戳")) v.offline_last_ts = j.at("离线收益_上次时间戳").get<TypedValue<double>>();
    if (j.contains("HaveSave0")) v.have_save0 = j.at("HaveSave0").get<TypedValue<bool>>();
    if (j.contains("DeletedSave0")) v.deleted_save0 = j.at("DeletedSave0").get<TypedValue<bool>>();
    if (j.contains("SaveS")) v.system_save = j.at("SaveS").get<TypedValue<SystemSaveData>>();
    if (j.contains("语言")) v.language = j.at("语言").get<TypedValue<int32_t>>();
    if (j.contains("SelectSave")) v.selected_slot = j.at("SelectSave").get<TypedValue<int32_t>>();
    if (j.contains("Save0")) v.save0 = j.at("Save0").get<TypedValue<MainSaveData>>();

    static const std::unordered_map<std::string, bool> known = {
        {"离线收益_上次时间戳", true},
        {"HaveSave0", true},
        {"DeletedSave0", true},
        {"SaveS", true},
        {"语言", true},
        {"SelectSave", true},
        {"Save0", true},
    };
    for (auto it = j.begin(); it != j.end(); ++it) {
        if (!known.count(it.key())) {
            v.extras[it.key()] = it.value();
        }
    }
}

}  // namespace yhsz::save
