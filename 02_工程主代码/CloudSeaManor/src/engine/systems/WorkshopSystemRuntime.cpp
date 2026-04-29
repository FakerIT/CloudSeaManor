#include "CloudSeamanor/engine/systems/WorkshopSystemRuntime.hpp"

#include "CloudSeamanor/app/GameAppText.hpp"
#include "CloudSeamanor/domain/PlayerMemorySystem.hpp"
#include "CloudSeamanor/domain/SkillSystem.hpp"
#include "CloudSeamanor/domain/TeaProductData.hpp"
#include "CloudSeamanor/domain/TeaSpiritDexSystem.hpp"
#include "CloudSeamanor/engine/GameWorldState.hpp"
#include "CloudSeamanor/engine/TextRenderUtils.hpp"

#include <unordered_map>

namespace CloudSeamanor::engine {

namespace {

int DetectWorkshopToolLevel_(const CloudSeamanor::domain::Inventory& inventory) {
    if (inventory.CountOf("hoe_spirit") > 0 || inventory.CountOf("watering_can_spirit") > 0) {
        return 4;
    }
    if (inventory.CountOf("hoe_gold") > 0 || inventory.CountOf("watering_can_gold") > 0) {
        return 3;
    }
    if (inventory.CountOf("hoe_silver") > 0 || inventory.CountOf("watering_can_silver") > 0) {
        return 2;
    }
    return 1;
}

// ============================================================================
// 【TeaQualityFromScore】根据品质分数计算品质等级
// ============================================================================
[[nodiscard]] std::string TeaQualityFromScore(float quality_score) {
    if (quality_score >= 15.0f) {
        return "Holy";
    }
    if (quality_score >= 10.0f) {
        return "Rare";
    }
    if (quality_score >= 5.0f) {
        return "Fine";
    }
    return "Normal";
}

// ============================================================================
// 【TryUnlockTeaSpirit】尝试解锁茶灵
// ============================================================================
bool TryUnlockTeaSpirit(
    CloudSeamanor::domain::TeaSpiritDexSystem& dex_system,
    const std::string& output_item_id,
    const std::string& quality,
    int current_day,
    int current_hour,
    const std::string& season_name,
    const std::string& weather,
    float ecology_balance,
    bool is_first_brew,
    bool is_festival_day,
    HintCallback hint_callback
) {
    // 查找是否有对应的茶灵
    bool unlocked = dex_system.TryUnlock(
        output_item_id,
        quality,
        season_name,
        current_hour,
        weather,
        "tea_room",
        "",  // extra_condition 后续扩展
        current_day
    );
    
    if (unlocked) {
        // 获取已解锁的茶灵信息
        const int total = dex_system.GetTotalCount();
        const int unlocked_count = dex_system.GetUnlockedCount();
        
        // 查找刚解锁的茶灵名称
        std::string spirit_name = "神秘茶灵";
        for (const auto* spirit : dex_system.GetUnlockedSpirits()) {
            // 检查这个茶灵是否在今天刚解锁
            if (spirit && spirit->first_unlock_day == current_day) {
                spirit_name = spirit->name;
                break;
            }
        }
        
        std::string hint = "【茶灵降临】" + spirit_name + " 被你的灵茶吸引而来！";
        hint += "（" + std::to_string(unlocked_count) + "/" + std::to_string(total) + " 已解锁）";
        
        if (hint_callback) {
            hint_callback(hint, 4.0f);
        }
        return true;
    }
    return false;
}

}  // namespace

// ============================================================================
// 【WorkshopSystemRuntime::WorkshopSystemRuntime】构造函数
// ============================================================================
WorkshopSystemRuntime::WorkshopSystemRuntime(
    GameWorldSystems& systems,
    GameWorldState& world_state,
    HintCallback hint_callback
)
    : systems_(systems)
    , world_state_(world_state)
    , hint_callback_(std::move(hint_callback))
{
}

// ============================================================================
// 【WorkshopSystemRuntime::Update】每帧更新工坊
// ============================================================================
void WorkshopSystemRuntime::Update(float delta_seconds) {
    static const bool tea_table_loaded =
        CloudSeamanor::domain::GetGlobalTeaProductTable().LoadFromFile("assets/data/tea/tea_products.csv");
    (void)tea_table_loaded;

    const float cloud_density = systems_.GetCloud().CurrentSpiritDensity();
    const int skill_level =
        systems_.GetSkills().GetLevel(CloudSeamanor::domain::SkillType::SpiritFarm);
    const int tool_level = DetectWorkshopToolLevel_(world_state_.GetInventory());
    std::unordered_map<std::string, int> outputs;
    
    // 使用 UpdateWorkshopWithCompletion 获取完成信息
    auto completions = systems_.UpdateWorkshopWithCompletion(
        delta_seconds, cloud_density, skill_level, tool_level, outputs);

    // 获取当前环境信息（用于茶灵解锁判定）
    const auto& clock = world_state_.GetClock();
    const std::string season_name = CloudSeamanor::domain::GameClock::SeasonName(clock.Season());
    const std::string weather = systems_.GetCloud().CurrentStateText();
    const int current_day = clock.Day();
    const int current_hour = clock.Hour();
    const float ecology_balance = systems_.GetEcology().GetBalanceScore();
    const bool is_festival_day = !world_state_.GetActiveFestivalId().empty();
    
    // 追踪是否为本局第一次制茶（用于 first_brew 条件）
    static bool is_first_brew = true;

    // 处理完成信息
    for (const auto& completion : completions) {
        // 计算品质等级
        const std::string quality = TeaQualityFromScore(completion.quality_score);
        
        // 检查是否是茶灵相关配方
        const bool is_tea_related = completion.recipe_id.find("tea") != std::string::npos
            || completion.output_item.find("tea") != std::string::npos
            || completion.output_item == "TeaPack"
            || completion.output_item.find("spirit_tea") != std::string::npos;
        
        // 尝试解锁茶灵（仅对茶相关产出）
        if (is_tea_related && systems_.GetTeaSpiritDex().GetTotalCount() > 0) {
            const bool unlocked = TryUnlockTeaSpirit(
                systems_.MutableTeaSpiritDex(),
                completion.output_item,
                quality,
                current_day,
                current_hour,
                season_name,
                weather,
                ecology_balance,
                is_first_brew,
                is_festival_day,
                hint_callback_
            );
            is_first_brew = false;  // 首次制茶后标记
            
            // P8-MEM-002: 茶灵解锁后写入记忆
            if (unlocked) {
                systems_.MutableMemory().AddMemory(
                    CloudSeamanor::domain::PlayerMemoryType::TeaSpiritUnlocked,
                    completion.output_item,
                    current_day,
                    5,  // 茶灵解锁权重高
                    30   // 茶灵记忆持久
                );
            }
        }
        
        // P8-MEM-002: 制茶完成后写入记忆（不区分是否解锁茶灵）
        if (is_tea_related) {
            if (quality == "Holy") {
                // 圣品茶：强记忆
                systems_.MutableMemory().AddMemory(
                    CloudSeamanor::domain::PlayerMemoryType::BrewHolyTea,
                    completion.output_item,
                    current_day,
                    3,
                    14
                );
            } else {
                // 普通制茶：弱记忆
                systems_.MutableMemory().AddMemory(
                    CloudSeamanor::domain::PlayerMemoryType::BrewNewTea,
                    completion.output_item,
                    current_day,
                    1,
                    7
                );
            }
        }
    }

    // 处理产出物品
    for (const auto& [item_id, count] : outputs) {
        if (item_id == "TeaPack") {
            world_state_.MutableTeaMachine().queued_output += count;
            if (hint_callback_) {
                hint_callback_("灵茶加工完成！按 E 领取", 3.0f);
            }
            continue;
        }
        world_state_.MutableInventory().AddItem(item_id, count);
        if (hint_callback_) {
            if (const auto* tea = CloudSeamanor::domain::GetGlobalTeaProductTable().Get(item_id)) {
                hint_callback_(
                    "工坊产出 " + ItemDisplayName(item_id) + " x" + std::to_string(count)
                    + "（品质:" + tea->quality
                    + "，售价:" + std::to_string(tea->sell_price)
                    + "，自用:" + tea->buff_effect_id
                    + "，赠礼:" + tea->gift_preference + "）", 3.2f);
            } else {
                hint_callback_(
                    "工坊产出 " + ItemDisplayName(item_id) + " x" +
                    std::to_string(count) + "。", 3.0f);
            }
        }
    }
}

// ============================================================================
// 【WorkshopSystemRuntime::UpdateProgressBar】更新工坊进度条 UI
// ============================================================================
void WorkshopSystemRuntime::UpdateProgressBar() const {
    ::CloudSeamanor::engine::UpdateWorkshopProgressBar(
        world_state_, systems_.GetWorkshop());
}

// ============================================================================
// 【WorkshopSystemRuntime::DomainSystem】获取领域系统引用
// ============================================================================
CloudSeamanor::domain::WorkshopSystem& WorkshopSystemRuntime::DomainSystem() {
    return systems_.GetWorkshop();
}

const CloudSeamanor::domain::WorkshopSystem& WorkshopSystemRuntime::DomainSystem() const {
    return systems_.GetWorkshop();
}

// ============================================================================
// 【WorkshopSystemRuntime::GetProgress】获取机器进度
// ============================================================================
float WorkshopSystemRuntime::GetProgress(const std::string& machine_id) const {
    return systems_.GetWorkshop().GetMachineProgress(machine_id);
}

// ============================================================================
// 【WorkshopSystemRuntime::GetMachine】获取机器状态
// ============================================================================
const CloudSeamanor::domain::MachineState* WorkshopSystemRuntime::GetMachine(
    const std::string& machine_id
) const {
    return systems_.GetWorkshop().GetMachine(machine_id);
}

}  // namespace CloudSeamanor::engine
