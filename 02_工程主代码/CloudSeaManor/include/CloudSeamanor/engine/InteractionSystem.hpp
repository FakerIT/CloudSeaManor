#pragma once

// ============================================================================
// 【InteractionSystem】交互系统
// ============================================================================
// 处理玩家与游戏中各种对象的交互逻辑。
//
// 主要职责：
// - 处理与 NPC 的对话和送礼
// - 处理与灵兽的互动
// - 处理与地块的种植/浇水/收获
// - 处理与可交互对象的交互（采集、工作台、储物）
//
// 设计原则：
// - 独立的交互逻辑管理
// - 通过依赖注入访问所需系统
// - 返回交互结果供 UI 层使用
// ============================================================================

#include "CloudSeamanor/engine/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/domain/DynamicLifeSystem.hpp"
#include "CloudSeamanor/engine/FarmingSystem.hpp"
#include "CloudSeamanor/domain/Inventory.hpp"
#include "CloudSeamanor/engine/Interactable.hpp"
#include "CloudSeamanor/domain/ManorEcologySystem.hpp"
#include "CloudSeamanor/engine/PickupDrop.hpp"
#include "CloudSeamanor/domain/SkillSystem.hpp"
#include "CloudSeamanor/domain/Stamina.hpp"
#include "CloudSeamanor/domain/WorkshopSystem.hpp"
#include "CloudSeamanor/engine/NpcDialogueManager.hpp"

#include <functional>
#include <string>
#include <vector>

namespace CloudSeamanor {

// Forward declaration for domain types used as pointers
namespace domain {
class ManorEcologySystem;
class PlayerMemorySystem;
}

namespace engine {

// Forward declarations
class FarmingSystem;
class DialogueEngine;

// ============================================================================
// 【InteractionResult】交互结果
// ============================================================================
struct InteractionResult {
    bool success = false;
    std::string message;
    std::string dialogue_text;
    bool show_dialogue = false;
    bool update_hud = false;
    std::vector<DialogueNode> dialogue_nodes;
    std::string dialogue_start_id;
};

// ============================================================================
// 【GiftResult】送礼结果
// ============================================================================
struct GiftResult {
    bool success = false;
    std::string message;
    int favor_change = 0;
};

// ============================================================================
// 【InteractionCallbacks】交互回调函数
// ============================================================================
struct InteractionCallbacks {
    std::function<void(const std::string&, float)> push_hint;
    std::function<void(const std::string&)> log_info;
    std::function<void()> update_hud;
    std::function<void()> refresh_window_title;
    std::function<void(const std::string&)> play_sfx;

    // 返回玩家名称和农场名称，用于构建对话上下文
    std::function<std::string()> get_player_name;
    std::function<std::string()> get_farm_name;
};

// ============================================================================
// 【InteractionContext】交互上下文
// ============================================================================
struct InteractionContext {
    int highlighted_plot_index = -1;
    int highlighted_npc_index = -1;
    int highlighted_interactable_index = -1;
    bool spirit_beast_highlighted = false;
};

// ============================================================================
// 【InteractionSystem】交互系统类
// ============================================================================
class InteractionSystem {
public:
    InteractionSystem();

    void Initialize(const InteractionCallbacks& callbacks);

    void SetFarmingSystem(FarmingSystem* farming);
    void SetWorkshopSystem(CloudSeamanor::domain::WorkshopSystem* workshop);
    void SetDynamicLifeSystem(CloudSeamanor::domain::DynamicLifeSystem* dynamic_life);
    void SetDialogueManager(CloudSeamanor::engine::NpcDialogueManager* manager);
    void SetManorEcologySystem(CloudSeamanor::domain::ManorEcologySystem* ecology);

    InteractionResult TalkToNpc(
        int npc_index,
        std::vector<NpcActor>& npcs,
        const NpcTextMappings& text_mappings,
        const CloudSeamanor::domain::GameClock& clock,
        CloudSeamanor::domain::CloudState cloud_state,
        const std::string& player_name,
        const std::string& farm_name
    );

    GiftResult GiveGiftToNpc(
        int npc_index,
        std::vector<NpcActor>& npcs,
        CloudSeamanor::domain::Inventory& inventory,
        CloudSeamanor::domain::DynamicLifeSystem* dynamic_life
    );

    InteractionResult InteractWithSpiritBeast(
        SpiritBeast& beast,
        int current_day,
        std::vector<HeartParticle>& heart_particles
    );

    InteractionResult InteractWithPlot(
        int plot_index,
        std::vector<TeaPlot>& plots,
        CloudSeamanor::domain::Inventory& inventory,
        CloudSeamanor::domain::SkillSystem& skills,
        float cloud_density,
        bool spirit_beast_interacted
    );

    InteractionResult InteractWithObject(
        int object_index,
        const std::vector<CloudSeamanor::domain::Interactable>& objects,
        std::vector<CloudSeamanor::domain::PickupDrop>& pickups,
        CloudSeamanor::domain::Inventory& inventory,
        CloudSeamanor::domain::WorkshopSystem* workshop,
        RepairProject& repair,
        std::vector<sf::RectangleShape>& obstacle_shapes,
        CloudSeamanor::domain::SkillSystem& skills,
        float cloud_density,
        bool spirit_beast_interacted
    );

    [[nodiscard]] std::string GetPlotActionText(const TeaPlot& plot) const;
    [[nodiscard]] std::string GetNpcActionText(const NpcActor& npc) const;
    [[nodiscard]] std::string GetSpiritBeastActionText(const SpiritBeast& beast, bool watered_today) const;
    [[nodiscard]] std::string GetObjectActionText(const CloudSeamanor::domain::Interactable& object, const RepairProject& repair, const TeaMachine& machine) const;

private:
    InteractionCallbacks callbacks_;
    FarmingSystem* farming_system_ = nullptr;
    CloudSeamanor::domain::WorkshopSystem* workshop_system_ = nullptr;
    CloudSeamanor::domain::DynamicLifeSystem* dynamic_life_system_ = nullptr;
    CloudSeamanor::engine::NpcDialogueManager* dialogue_manager_ = nullptr;
    domain::ManorEcologySystem* ecology_system_ = nullptr;
};

InteractionCallbacks CreateDefaultInteractionCallbacks(
    const std::function<void(const std::string&, float)>& push_hint,
    const std::function<void(const std::string&)>& log_info,
    const std::function<void()>& update_hud,
    const std::function<void()>& refresh_window_title,
    const std::function<void(const std::string&)>& play_sfx,
    const std::function<std::string()>& get_player_name,
    const std::function<std::string()>& get_farm_name
);

void RefreshPickupVisual(CloudSeamanor::domain::PickupDrop& pickup);

} // namespace engine
} // namespace CloudSeamanor
