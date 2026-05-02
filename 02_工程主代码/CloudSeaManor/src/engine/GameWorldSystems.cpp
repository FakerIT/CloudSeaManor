#include "CloudSeamanor/engine/GameWorldSystems.hpp"

#include <unordered_map>

namespace CloudSeamanor::engine {

// ============================================================================
// 【GameWorldSystems】构造函数
// ============================================================================
GameWorldSystems::GameWorldSystems() {
}

// ============================================================================
// 【Initialize】初始化所有系统
// ============================================================================
void GameWorldSystems::Initialize() {
    skills_.Initialize();
    festivals_.Initialize();
    workshop_.Initialize();
    dynamic_life_.Initialize();
    npc_development_.LoadStageRules("assets/data/npc/npc_development_stages.csv");
    npc_development_.LoadBranchRules("assets/data/npc/npc_development_branches.csv");
}

// ============================================================================
// 【InitializeContracts】初始化契约系统
// ============================================================================
void GameWorldSystems::InitializeContracts() {
    contracts_.Initialize();
}

// ============================================================================
// 【UpdateDaily】每日更新
// ============================================================================
void GameWorldSystems::UpdateDaily(
    CloudSeamanor::domain::Season season,
    int day_in_season,
    float cloud_density
) {
    festivals_.Update(season, day_in_season);
    dynamic_life_.UpdateDaily(cloud_density);
}

// ============================================================================
// 【CheckDailyTransitions】检查每日状态转换
// ============================================================================
void GameWorldSystems::CheckDailyTransitions(
    int seeded_plots_count,
    bool main_house_repaired,
    bool spirit_beast_interacted
) {
    int daily_influence = 0;
    daily_influence += seeded_plots_count * 5;
    if (!main_house_repaired) daily_influence += 5;
    if (!spirit_beast_interacted) daily_influence -= 3;
    cloud_.ApplyPlayerInfluence(daily_influence);
}

// ============================================================================
// 【UpdateWorkshop】更新工坊
// ============================================================================
void GameWorldSystems::UpdateWorkshop(
    float delta_time,
    float cloud_density,
    int skill_level,
    int tool_level,
    std::unordered_map<std::string, int>& output_items
) {
    workshop_.Update(delta_time, cloud_density, skill_level, tool_level, output_items);
}

// ============================================================================
// 【UpdateWorkshopWithCompletion】更新工坊并返回完成信息
// ============================================================================
std::vector<CloudSeamanor::domain::MachineCompletionInfo> GameWorldSystems::UpdateWorkshopWithCompletion(
    float delta_time,
    float cloud_density,
    int skill_level,
    int tool_level,
    std::unordered_map<std::string, int>& output_items
) {
    return workshop_.Update(delta_time, cloud_density, skill_level, tool_level, output_items);
}

// ============================================================================
// 【CheckContractUnlocks】检查契约解锁
// ============================================================================
void GameWorldSystems::CheckContractUnlocks() {
    contracts_.CheckVolumeUnlocks();
}

// ============================================================================
// 【AddPlayerInfluence】添加玩家影响力
// ============================================================================
void GameWorldSystems::AddPlayerInfluence(int value) {
    cloud_.ApplyPlayerInfluence(value);
}

// ============================================================================
// 【SaveContractsState】保存契约状态
// ============================================================================
std::string GameWorldSystems::SaveContractsState() const {
    return contracts_.SaveState();
}

// ============================================================================
// 【LoadContractsState】加载契约状态
// ============================================================================
void GameWorldSystems::LoadContractsState(const std::string& state) {
    contracts_.LoadState(state);
}

// ============================================================================
// 【SaveEcologyState】保存生态状态
// ============================================================================
std::string GameWorldSystems::SaveEcologyState() const {
    return ecology_.Serialize();
}

// ============================================================================
// 【LoadEcologyState】加载生态状态
// ============================================================================
void GameWorldSystems::LoadEcologyState(const std::string& state) {
    ecology_.Deserialize(state);
}

// ============================================================================
// 【SaveMemoryState】保存记忆状态
// ============================================================================
std::string GameWorldSystems::SaveMemoryState() const {
    return memory_.Serialize();
}

// ============================================================================
// 【LoadMemoryState】加载记忆状态
// ============================================================================
void GameWorldSystems::LoadMemoryState(const std::string& state) {
    memory_.Deserialize(state);
}

// ============================================================================
// 【SaveTeaSpiritDexState】保存茶灵图鉴状态
// ============================================================================
std::string GameWorldSystems::SaveTeaSpiritDexState() const {
    return tea_spirit_dex_.Serialize();
}

// ============================================================================
// 【LoadTeaSpiritDexState】加载茶灵图鉴状态
// ============================================================================
void GameWorldSystems::LoadTeaSpiritDexState(const std::string& state) {
    tea_spirit_dex_.Deserialize(state);
}

} // namespace CloudSeamanor::engine
