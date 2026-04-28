#include "../TestFramework.hpp"
#include "CloudSeamanor/GameAppSave.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>

using CloudSeamanor::domain::CloudState;
using CloudSeamanor::domain::GameClock;
using CloudSeamanor::domain::HungerSystem;
using CloudSeamanor::domain::Inventory;
using CloudSeamanor::domain::Player;
using CloudSeamanor::domain::StaminaSystem;
using CloudSeamanor::engine::LoadGameState;
using CloudSeamanor::engine::MailOrderEntry;
using CloudSeamanor::engine::NpcActor;
using CloudSeamanor::engine::PriceTableEntry;
using CloudSeamanor::engine::RepairProject;
using CloudSeamanor::engine::SaveGameState;
using CloudSeamanor::engine::SpiritBeast;
using CloudSeamanor::engine::TeaMachine;
using CloudSeamanor::engine::TeaPlot;

namespace {

std::filesystem::path MakeTempSavePath() {
    const auto base = std::filesystem::temp_directory_path();
    const auto name = "cloudseamanor_save_state_test_" + std::to_string(std::rand()) + ".sav";
    return base / name;
}

TeaPlot BuildSamplePlot() {
    TeaPlot plot;
    plot.crop_id = "tea_leaf";
    plot.crop_name = "云雾茶";
    plot.seed_item_id = "TeaSeed";
    plot.harvest_item_id = "TeaLeaf";
    plot.harvest_amount = 3;
    plot.tilled = true;
    plot.seeded = true;
    plot.watered = true;
    plot.ready = false;
    plot.growth = 42.5f;
    plot.stage = 2;
    plot.sprinkler_installed = true;
    plot.sprinkler_days_left = 11;
    plot.fertilized = true;
    plot.fertilizer_type = "basic";
    plot.in_greenhouse = true;
    plot.cleared = true;
    plot.disease = true;
    plot.pest = false;
    plot.disease_days = 2;
    return plot;
}

void CleanupSaveFiles(const std::filesystem::path& save_path) {
    std::error_code ec;
    std::filesystem::remove(save_path, ec);
    std::filesystem::remove(save_path.string() + ".bak", ec);
    std::filesystem::remove(save_path.string() + ".tmp", ec);
}

}  // namespace

TEST_CASE(SaveGameState_LoadGameState_roundtrip_plot_fields) {
    const auto save_path = MakeTempSavePath();

    GameClock clock;
    clock.SetState(9, 540.0f);
    CloudSeamanor::domain::CloudSystem cloud_system;
    Player player;
    player.SetPosition({321.0f, 123.0f});
    StaminaSystem stamina;
    stamina.SetCurrent(77.0f);
    HungerSystem hunger;
    hunger.Initialize(66, 100);
    RepairProject repair;
    TeaMachine machine;
    SpiritBeast beast;
    beast.custom_name = "测试灵兽";
    bool spirit_beast_watered_today = true;
    std::vector<TeaPlot> tea_plots{BuildSamplePlot()};
    int gold = 1234;
    std::vector<PriceTableEntry> prices;
    std::vector<MailOrderEntry> mails;
    Inventory inventory;
    inventory.AddItem("TeaLeaf", 5);
    std::vector<NpcActor> npcs;

    auto push_hint = [](const std::string&, float) {};
    ASSERT_TRUE(SaveGameState(save_path,
                              clock,
                              cloud_system,
                              player,
                              stamina,
                              hunger,
                              repair,
                              machine,
                              beast,
                              spirit_beast_watered_today,
                              tea_plots,
                              gold,
                              prices,
                              mails,
                              inventory,
                              npcs,
                              push_hint));

    GameClock loaded_clock;
    CloudSeamanor::domain::CloudSystem loaded_cloud;
    Player loaded_player;
    StaminaSystem loaded_stamina;
    HungerSystem loaded_hunger;
    RepairProject loaded_repair;
    TeaMachine loaded_machine;
    SpiritBeast loaded_beast;
    bool loaded_spirit_watered = false;
    std::vector<TeaPlot> loaded_plots{TeaPlot{}};
    int loaded_gold = 0;
    std::vector<PriceTableEntry> loaded_prices;
    std::vector<MailOrderEntry> loaded_mails;
    Inventory loaded_inventory;
    std::vector<NpcActor> loaded_npcs;
    std::vector<sf::RectangleShape> obstacle_shapes;
    CloudState last_cloud_state = CloudState::Clear;

    ASSERT_TRUE(LoadGameState(save_path,
                              loaded_clock,
                              loaded_cloud,
                              loaded_player,
                              loaded_stamina,
                              loaded_hunger,
                              loaded_repair,
                              loaded_machine,
                              loaded_beast,
                              loaded_spirit_watered,
                              loaded_plots,
                              loaded_gold,
                              loaded_prices,
                              loaded_mails,
                              loaded_inventory,
                              loaded_npcs,
                              obstacle_shapes,
                              last_cloud_state,
                              []() {},
                              [](TeaPlot&, bool) {},
                              []() {},
                              []() {},
                              push_hint));

    ASSERT_EQ(loaded_plots.size(), static_cast<std::size_t>(1));
    ASSERT_EQ(loaded_plots[0].crop_id, "tea_leaf");
    ASSERT_TRUE(loaded_plots[0].sprinkler_installed);
    ASSERT_EQ(loaded_plots[0].sprinkler_days_left, 11);
    ASSERT_TRUE(loaded_plots[0].fertilized);
    ASSERT_EQ(loaded_plots[0].fertilizer_type, "basic");
    ASSERT_TRUE(loaded_plots[0].in_greenhouse);
    ASSERT_TRUE(loaded_plots[0].disease);
    ASSERT_EQ(loaded_plots[0].disease_days, 2);
    ASSERT_EQ(loaded_gold, 1234);

    CleanupSaveFiles(save_path);
    return true;
}

TEST_CASE(LoadGameState_fails_on_tampered_checksum) {
    const auto save_path = MakeTempSavePath();

    GameClock clock;
    CloudSeamanor::domain::CloudSystem cloud_system;
    Player player;
    StaminaSystem stamina;
    HungerSystem hunger;
    hunger.Initialize(70, 100);
    RepairProject repair;
    TeaMachine machine;
    SpiritBeast beast;
    bool spirit_beast_watered_today = false;
    std::vector<TeaPlot> tea_plots{BuildSamplePlot()};
    int gold = 50;
    std::vector<PriceTableEntry> prices;
    std::vector<MailOrderEntry> mails;
    Inventory inventory;
    std::vector<NpcActor> npcs;
    auto push_hint = [](const std::string&, float) {};

    ASSERT_TRUE(SaveGameState(save_path,
                              clock,
                              cloud_system,
                              player,
                              stamina,
                              hunger,
                              repair,
                              machine,
                              beast,
                              spirit_beast_watered_today,
                              tea_plots,
                              gold,
                              prices,
                              mails,
                              inventory,
                              npcs,
                              push_hint));

    {
        std::ofstream out(save_path, std::ios::app);
        ASSERT_TRUE(out.is_open());
        out << "tampered|1\n";
    }

    GameClock loaded_clock;
    CloudSeamanor::domain::CloudSystem loaded_cloud;
    Player loaded_player;
    StaminaSystem loaded_stamina;
    HungerSystem loaded_hunger;
    RepairProject loaded_repair;
    TeaMachine loaded_machine;
    SpiritBeast loaded_beast;
    bool loaded_spirit_watered = false;
    std::vector<TeaPlot> loaded_plots{TeaPlot{}};
    int loaded_gold = 0;
    std::vector<PriceTableEntry> loaded_prices;
    std::vector<MailOrderEntry> loaded_mails;
    Inventory loaded_inventory;
    std::vector<NpcActor> loaded_npcs;
    std::vector<sf::RectangleShape> obstacle_shapes;
    CloudState last_cloud_state = CloudState::Clear;

    ASSERT_TRUE(!LoadGameState(save_path,
                               loaded_clock,
                               loaded_cloud,
                               loaded_player,
                               loaded_stamina,
                               loaded_hunger,
                               loaded_repair,
                               loaded_machine,
                               loaded_beast,
                               loaded_spirit_watered,
                               loaded_plots,
                               loaded_gold,
                               loaded_prices,
                               loaded_mails,
                               loaded_inventory,
                               loaded_npcs,
                               obstacle_shapes,
                               last_cloud_state,
                               []() {},
                               [](TeaPlot&, bool) {},
                               []() {},
                               []() {},
                               push_hint));

    CleanupSaveFiles(save_path);
    return true;
}
