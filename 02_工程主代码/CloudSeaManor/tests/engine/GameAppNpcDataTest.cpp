#include "TestFramework.hpp"

#include "CloudSeamanor/GameAppNpc.hpp"

#include <filesystem>

using namespace CloudSeamanor::engine;

namespace {

std::filesystem::path NpcTestSourceRoot() {
    return std::filesystem::weakly_canonical(
        std::filesystem::current_path().parent_path().parent_path());
}

}

TEST_CASE(GameAppNpc_BuildNpcsUsesNpcDataCsv) {
    NpcTextMappings mappings;
    const std::filesystem::path root = NpcTestSourceRoot();
    ASSERT_TRUE(LoadNpcTextMappings((root / "assets/data/NPC_Texts.json").string(), mappings));

    std::vector<NpcActor> npcs;
    BuildNpcs((root / "assets/data/Schedule_Data.csv").string(),
              (root / "assets/data/Gift_Preference.json").string(),
              (root / "assets/data/npc/npc_data.csv").string(),
              mappings,
              npcs);

    auto find_npc = [&](const std::string& id) -> const NpcActor* {
        for (const auto& npc : npcs) {
            if (npc.id == id) {
                return &npc;
            }
        }
        return nullptr;
    };

    const NpcActor* lin = find_npc("lin");
    ASSERT_TRUE(lin != nullptr);
    ASSERT_EQ(lin->display_name, std::string("林茶师"));
    ASSERT_FLOAT_EQ(lin->position.x, 420.0f, 0.001f);
    ASSERT_FLOAT_EQ(lin->position.y, 320.0f, 0.001f);
    ASSERT_FALSE(lin->schedule.empty());
    return true;
}
