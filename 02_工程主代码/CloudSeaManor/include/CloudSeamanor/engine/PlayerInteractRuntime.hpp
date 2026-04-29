#pragma once

// ============================================================================
// 【PlayerInteractRuntime.hpp】玩家交互运行时
// ============================================================================
// 提供 E/G 键交互处理所需的上下文结构体和函数声明。
//
// 主要职责：
// - 定义 PlayerInteractRuntimeContext 结构体，统一管理交互所需的所有状态
// - 提供 HandleGiftInteraction 函数（处理 G 键 NPC 送礼）
// - 提供 HandlePrimaryInteraction 函数（处理 E 键主交互）
//
// 设计原则：
// - 所有交互状态通过上下文结构体传递，避免全局变量
// - 使用构造函数确保所有引用成员都被正确初始化
// - 回调函数使用 std::function，支持灵活的事件处理
// ============================================================================

#include "CloudSeamanor/engine/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/domain/DynamicLifeSystem.hpp"
#include "CloudSeamanor/engine/GameWorldState.hpp"
#include "CloudSeamanor/engine/Interactable.hpp"
#include "CloudSeamanor/domain/Inventory.hpp"
#include "CloudSeamanor/engine/PickupDrop.hpp"
#include "CloudSeamanor/domain/SkillSystem.hpp"
#include "CloudSeamanor/domain/Stamina.hpp"
#include "CloudSeamanor/domain/HungerSystem.hpp"
#include "CloudSeamanor/domain/TeaBush.hpp"
#include "CloudSeamanor/domain/WorkshopSystem.hpp"
#include "CloudSeamanor/engine/NpcDialogueManager.hpp"
#include "CloudSeamanor/engine/FestivalRuntimeData.hpp"
#include "CloudSeamanor/domain/RelationshipSystem.hpp"
#include "CloudSeamanor/domain/FestivalSystem.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::domain {
class GameClock;
}

namespace CloudSeamanor::engine {

// ============================================================================
// 【PlayerInteractRuntimeContext】玩家交互运行时上下文
// ============================================================================
// 收敛 E/G 交互分发所需的可变状态与回调，避免模块内部依赖全局变量。
//
// 使用方式：
// - 通过构造函数创建实例，确保所有引用成员有效
// - 将实例传递给 HandleGiftInteraction 或 HandlePrimaryInteraction 函数
//
// 设计决策：
// - 使用引用而非指针，确保使用时必须提供有效对象
// - std::function 类型的回调支持灵活的依赖注入
// - 默认参数简化常见场景下的创建过程
// ============================================================================
struct PlayerInteractRuntimeContext {
    // ============================================================================
    // 【交互目标索引】
    // ============================================================================
    /** 当前高亮的 NPC 索引，-1 表示没有高亮任何 NPC */
    int highlighted_npc_index = -1;

    /** 当前高亮的茶田地块索引，-1 表示没有高亮任何地块 */
    int highlighted_plot_index = -1;

    /** 当前高亮的一般可交互对象索引，-1 表示没有高亮任何对象 */
    int highlighted_index = -1;

    // ============================================================================
    // 【游戏时间状态】
    // ============================================================================
    /** 当前游戏天数，用于判断每日限制（如每日送礼一次） */
    int current_day = 1;
    int current_hour = 8;

    // ============================================================================
    // 【交互目标状态】
    // ============================================================================
    /** 灵兽是否处于高亮状态 */
    bool spirit_beast_highlighted = false;

    // ============================================================================
    // 【核心系统引用】
    // ============================================================================
    /** 游戏时钟引用：用于构建对话上下文 */
    CloudSeamanor::domain::GameClock& game_clock;

    /** 云海系统引用：用于获取云海状态和灵气密度 */
    CloudSeamanor::domain::CloudSystem& cloud_system;

    /** 背包系统引用：用于检查物品数量、消耗物品、增加物品 */
    CloudSeamanor::domain::Inventory& inventory;

    /** 体力系统引用：用于检查体力、消耗体力、恢复体力 */
    CloudSeamanor::domain::StaminaSystem& stamina;

    /** 饱食系统引用：用于进食与早餐恢复 */
    CloudSeamanor::domain::HungerSystem& hunger;

    /** 茶田地块列表引用：用于修改地块状态（翻土、播种、浇水、收获） */
    std::vector<TeaPlot>& tea_plots;
    std::vector<CloudSeamanor::domain::TeaBush>& tea_bushes;

    /** NPC 列表引用：用于修改 NPC 状态（好感度、每日送礼标记） */
    std::vector<NpcActor>& npcs;

    /** 灵兽状态引用：用于修改灵兽交互状态 */
    SpiritBeast& spirit_beast;

    /** 灵兽今日是否已被浇水标记的引用 */
    bool& spirit_beast_watered_today;

    /** 爱心粒子效果列表引用：用于添加/移除粒子效果 */
    std::vector<HeartParticle>& heart_particles;

    /** 可拾取物列表引用：用于添加新的掉落物 */
    std::vector<CloudSeamanor::domain::PickupDrop>& pickups;

    /** 场景可交互对象列表引用：用于查询交互对象信息 */
    const std::vector<CloudSeamanor::domain::Interactable>& interactables;

    /** 主屋修缮项目引用：用于检查和完成修缮 */
    RepairProject& main_house_repair;

    /** 制茶机运行时状态：用于加工完成后的领取逻辑 */
    TeaMachine& tea_machine;

    /** 障碍物形状列表引用：用于修改障碍物外观 */
    std::vector<sf::RectangleShape>& obstacle_shapes;

    /** 技能系统引用：用于添加经验、检查升级、增加技能加成 */
    CloudSeamanor::domain::SkillSystem& skills;

    /** 动态人生系统引用：用于 NPC 阶段变化和好感度通知 */
    CloudSeamanor::domain::DynamicLifeSystem& dynamic_life;

    /** 工坊系统引用：用于制茶机操作 */
    CloudSeamanor::domain::WorkshopSystem& workshop;

    /** 关系系统引用：用于告白/婚礼状态机推进 */
    CloudSeamanor::domain::RelationshipSystem& relationship_system;

    /** 关系状态引用：用于持久化与 UI 展示 */
    CloudSeamanor::domain::RelationshipState& relationship_state;

    /** 节日系统只读引用：用于婚礼日期冲突处理 */
    const CloudSeamanor::domain::FestivalSystem& festivals;

    /** NPC 文本映射表引用：用于获取 NPC 位置和活动的显示名称 */
    NpcTextMappings& npc_text_mappings;

    /** NPC 对话管理器指针：用于生成动态对话内容 */
    NpcDialogueManager* dialogue_manager = nullptr;

    /** 对话引擎引用：用于启动和管理分支对话 */
    DialogueEngine& dialogue_engine;

    /** 对话数据根目录：用于 StartDialogueById 加载 JSON 文件 */
    std::string dialogue_data_root;

    /** 玩家名称：用于对话 Token 替换 */
    std::string player_name = "云海旅人";

    /** 农场名称：用于对话 Token 替换 */
    std::string farm_name = "云海山庄";

    /** 对话节点列表指针：用于填充分支对话节点 */
    std::vector<DialogueNode>* dialogue_nodes = nullptr;

    /** 对话起始节点 ID 指针：用于设置对话起始位置 */
    std::string* dialogue_start_id = nullptr;

    /** 对话文本引用：用于设置当前对话内容 */
    std::string& dialogue_text;

    /** 玩家金币引用。 */
    int& gold;
    std::vector<PriceTableEntry>& price_table;
    std::vector<MailOrderEntry>& mail_orders;
    CloudSeamanor::domain::CropQuality& last_trade_quality;
    bool& in_spirit_realm;
    const std::string& active_festival_id;
    FestivalRuntimeData* festival_runtime_state = nullptr;
    std::unordered_map<std::string, int>& spirit_plant_last_harvest_hour;
    std::unordered_map<std::string, int>& weekly_buy_count;
    std::unordered_map<std::string, int>& weekly_sell_count;
    std::vector<std::string>& daily_general_store_stock;
    int& inn_gold_reserve;
    int& coop_fed_today;
    int& decoration_score;
    std::vector<DiaryEntryState>& diary_entries;
    std::unordered_map<std::string, std::string>& skill_branches;
    std::vector<std::string>& pending_skill_branches;
    std::vector<PlacedObject>& placed_objects;
    int& fishing_attempts;
    std::string& last_fish_catch;
    std::string& pet_type;
    bool& pet_adopted;
    std::unordered_map<std::string, bool>& achievements;
    std::vector<std::string>& mod_hooks;
    bool& greenhouse_unlocked;
    bool& greenhouse_tag_next_planting;
    int current_game_hour;

    /**
     * @brief 与 NPC 对话时尝试领取委托奖励（BE-028）。
     * @return true 表示成功领取过至少一个奖励
     */
    std::function<bool(const std::string& npc_id)> try_claim_npc_delivery_rewards;

    /**
     * @brief 请求灵界传送（BE-029）。由 Runtime 决定何时切图/播放过场。
     * @note 传入 true 表示去灵界，false 表示回主世界。
     */
    std::function<void(bool to_spirit_realm)> request_spirit_realm_travel;
    std::function<void(const std::string& source_label)> start_fishing_qte;

    // ============================================================================
    // 【消耗参数】
    // ============================================================================
    /** 单次交互消耗的体力值 */
    float stamina_interact_cost = 0.0f;

    // ============================================================================
    // 【回调函数】
    // ============================================================================
    /** 推送提示消息回调 */
    std::function<void(const std::string&, float)> push_hint;

    /** 记录信息日志回调 */
    std::function<void(const std::string&)> log_info;

    /** 播放音效回调 */
    std::function<void(const std::string&)> play_sfx;

    /** 刷新 HUD 文本回调 */
    std::function<void()> update_hud_text;

    /** 刷新全部 UI 回调（用于对话打字机效果同步 HUD 状态） */
    std::function<void()> update_ui;

    /** 刷新窗口标题回调 */
    std::function<void()> refresh_window_title;

    /** 技能升级回调 */
    std::function<void(CloudSeamanor::domain::SkillType, int)> on_skill_level_up;

    /** 心事件完成回调：触发奖励结算 */
    std::function<void(const std::string& npc_id, int reward_favor, const std::string& reward_flag)> on_heart_event_complete;

    /** 刷新地块视觉回调 */
    std::function<void(TeaPlot&, bool)> refresh_plot_visual;

    /** 生成爱心粒子回调 */
    std::function<void(const CloudSeamanor::domain::Vec2f&, std::vector<HeartParticle>&)> spawn_heart_particles;

    /** 刷新拾取物视觉回调 */
    std::function<void(CloudSeamanor::domain::PickupDrop&)> refresh_pickup_visual;

    /** 世界交互状态引用：用于追踪心事件完成奖励 */
    CloudSeamanor::engine::InteractionState& interaction_state;

    // ============================================================================
    // 【心事件追踪】
    // ============================================================================
    std::string current_heart_event_id;
    int current_heart_event_reward = 0;
    std::string current_heart_event_flag;

    // ============================================================================
    // 【构造函数】
    // ============================================================================
    PlayerInteractRuntimeContext(
        CloudSeamanor::domain::GameClock& clock,
        CloudSeamanor::domain::CloudSystem& cloud,
        CloudSeamanor::domain::Inventory& inv,
        CloudSeamanor::domain::StaminaSystem& sta,
        CloudSeamanor::domain::HungerSystem& hunger_system,
        float stamina_cost,
        std::vector<TeaPlot>& plots,
        std::vector<CloudSeamanor::domain::TeaBush>& bushes,
        std::vector<NpcActor>& npcs_list,
        SpiritBeast& beast,
        bool& beast_watered,
        std::vector<HeartParticle>& hearts,
        std::vector<CloudSeamanor::domain::PickupDrop>& pickups_list,
        const std::vector<CloudSeamanor::domain::Interactable>& interactables_list,
        RepairProject& repair,
        TeaMachine& tea_machine_state,
        std::vector<sf::RectangleShape>& obstacles,
        CloudSeamanor::domain::SkillSystem& sk,
        CloudSeamanor::domain::DynamicLifeSystem& dl,
        CloudSeamanor::domain::WorkshopSystem& ws,
        CloudSeamanor::domain::RelationshipSystem& rel_system,
        CloudSeamanor::domain::RelationshipState& rel_state,
        const CloudSeamanor::domain::FestivalSystem& festivals_system,
        NpcTextMappings& mappings,
        DialogueEngine& dialogue_eng,
        std::string dialogue_root,
        std::string& dialogue,
        int& player_gold,
        std::vector<PriceTableEntry>& price_entries,
        std::vector<MailOrderEntry>& order_entries,
        CloudSeamanor::domain::CropQuality& trade_quality,
        bool& spirit_realm_flag,
        const std::string& active_festival_id_ref,
        FestivalRuntimeData* festival_runtime_state_ptr,
        std::unordered_map<std::string, int>& spirit_plant_hours,
        std::unordered_map<std::string, int>& weekly_buy_counter,
        std::unordered_map<std::string, int>& weekly_sell_counter,
        std::vector<std::string>& daily_store_stock,
        int& inn_reserve,
        int& coop_fed,
        int& deco_score,
        std::vector<DiaryEntryState>& diary_entry_list,
        std::unordered_map<std::string, std::string>& skill_branch_map,
        std::vector<std::string>& pending_skill_branch_list,
        std::vector<PlacedObject>& placed_object_list,
        int& fishing_attempt_count,
        std::string& last_fish_catch_id,
        std::string& pet_type_ref,
        bool& pet_adopted_ref,
        std::unordered_map<std::string, bool>& achievements_ref,
        std::vector<std::string>& mod_hooks_ref,
        bool& greenhouse_unlocked_ref,
        bool& greenhouse_tag_next_planting_ref,
        int game_hour,
        std::function<bool(const std::string& npc_id)> claim_delivery_fn,
        std::function<void(bool to_spirit_realm)> spirit_realm_travel_fn,
        std::function<void(const std::string& source_label)> start_fishing_qte_fn,
        std::function<void(const std::string&, float)> hint_fn,
        std::function<void(const std::string&)> log_fn,
        std::function<void(const std::string&)> sfx_fn,
        std::function<void()> hud_fn,
        std::function<void()> ui_fn,
        std::function<void()> title_fn,
        std::function<void(CloudSeamanor::domain::SkillType, int)> level_fn,
        std::function<void(TeaPlot&, bool)> plot_fn,
        std::function<void(const CloudSeamanor::domain::Vec2f&, std::vector<HeartParticle>&)> hearts_fn,
        std::function<void(CloudSeamanor::domain::PickupDrop&)> pickup_fn,
        CloudSeamanor::engine::InteractionState& interaction_state,
        int npc_idx = -1,
        int plot_idx = -1,
        int idx = -1,
        int day = 1,
        int hour = 8,
        bool beast_highlighted = false
    ) : game_clock(clock),
        cloud_system(cloud),
        inventory(inv),
        stamina(sta),
        hunger(hunger_system),
        stamina_interact_cost(stamina_cost),
        tea_plots(plots),
        tea_bushes(bushes),
        npcs(npcs_list),
        spirit_beast(beast),
        spirit_beast_watered_today(beast_watered),
        heart_particles(hearts),
        pickups(pickups_list),
        interactables(interactables_list),
        main_house_repair(repair),
        tea_machine(tea_machine_state),
        obstacle_shapes(obstacles),
        skills(sk),
        dynamic_life(dl),
        workshop(ws),
        relationship_system(rel_system),
        relationship_state(rel_state),
        festivals(festivals_system),
        npc_text_mappings(mappings),
        dialogue_engine(dialogue_eng),
        dialogue_data_root(std::move(dialogue_root)),
        dialogue_text(dialogue),
        gold(player_gold),
        price_table(price_entries),
        mail_orders(order_entries),
        last_trade_quality(trade_quality),
        in_spirit_realm(spirit_realm_flag),
        active_festival_id(active_festival_id_ref),
        festival_runtime_state(festival_runtime_state_ptr),
        spirit_plant_last_harvest_hour(spirit_plant_hours),
        weekly_buy_count(weekly_buy_counter),
        weekly_sell_count(weekly_sell_counter),
        daily_general_store_stock(daily_store_stock),
        inn_gold_reserve(inn_reserve),
        coop_fed_today(coop_fed),
        decoration_score(deco_score),
        diary_entries(diary_entry_list),
        skill_branches(skill_branch_map),
        pending_skill_branches(pending_skill_branch_list),
        placed_objects(placed_object_list),
        fishing_attempts(fishing_attempt_count),
        last_fish_catch(last_fish_catch_id),
        pet_type(pet_type_ref),
        pet_adopted(pet_adopted_ref),
        achievements(achievements_ref),
        mod_hooks(mod_hooks_ref),
        greenhouse_unlocked(greenhouse_unlocked_ref),
        greenhouse_tag_next_planting(greenhouse_tag_next_planting_ref),
        current_game_hour(game_hour),
        try_claim_npc_delivery_rewards(std::move(claim_delivery_fn)),
        request_spirit_realm_travel(std::move(spirit_realm_travel_fn)),
        start_fishing_qte(std::move(start_fishing_qte_fn)),
        push_hint(hint_fn),
        log_info(log_fn),
        play_sfx(sfx_fn),
        update_hud_text(hud_fn),
        update_ui(ui_fn),
        refresh_window_title(title_fn),
        on_skill_level_up(level_fn),
        refresh_plot_visual(plot_fn),
        spawn_heart_particles(hearts_fn),
        refresh_pickup_visual(pickup_fn),
        interaction_state(interaction_state),
        highlighted_npc_index(npc_idx),
        highlighted_plot_index(plot_idx),
        highlighted_index(idx),
        current_day(day),
        current_hour(hour),
        spirit_beast_highlighted(beast_highlighted)
    {}
};

// ============================================================================
// 【HandleGiftInteraction】处理 NPC 送礼交互（G 键）
// ============================================================================
bool HandleGiftInteraction(PlayerInteractRuntimeContext& context);

// ============================================================================
// 【HandlePrimaryInteraction】处理主交互（E 键）
// ============================================================================
/**
 * @brief 处理 E 键的主交互
 *
 * 交互优先级：
 * 1. NPC 对话（使用 NpcDialogueManager 生成动态对话）
 * 2. 灵兽互动
 * 3. 茶田地块操作（翻土/播种/浇水/收获）
 * 4. 一般可交互对象（采集点、工作台、储物点）
 *
 * @param context 交互运行时上下文，包含所有必要状态和回调
 * @return true 表示输入被消费，false 表示无有效目标
 */
bool HandlePrimaryInteraction(PlayerInteractRuntimeContext& context);

} // namespace CloudSeamanor::engine
