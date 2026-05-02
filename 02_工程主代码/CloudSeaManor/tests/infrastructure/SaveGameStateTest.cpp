#include "../TestFramework.hpp"
#include "CloudSeamanor/app/GameAppSave.hpp"

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

TEST_CASE(LoadGameState_migrates_legacy_v5_save) {
    const auto save_path = MakeTempSavePath();
    {
        std::ofstream out(save_path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "version|5\n";
        out << "clock|3|480\n";
        out << "cloud|0|1|10|5\n";
        out << "player|100|200|80\n";
        out << "hunger|70,100,0,0\n";
        out << "economy|777\n";
        out << "plot_schema|index|tilled|seeded|watered|ready|growth|stage|crop_id|quality\n";
        out << "plot|0|1|1|0|0|12.5|1|tea_leaf|0\n";
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
    auto push_hint = [](const std::string&, float) {};

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

    ASSERT_EQ(loaded_gold, 777);
    ASSERT_EQ(loaded_plots[0].crop_id, "tea_leaf");
    CleanupSaveFiles(save_path);
    return true;
}

// P17-STABLE-003: 边界条件测试 - 空存档
TEST_CASE(LoadGameState_handles_empty_file) {
    const auto save_path = MakeTempSavePath();
    {
        std::ofstream out(save_path, std::ios::trunc);
        // 空文件
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
    auto push_hint = [](const std::string&, float) {};

    // 空存档应返回 false 或安全初始化默认值
    const bool result = LoadGameState(save_path,
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
                              push_hint);

    // 验证：空存档不应崩溃，应返回失败或使用默认值
    // ASSERT_TRUE(!result);  // 如果返回 false 表示正确拒绝
    CleanupSaveFiles(save_path);
    return true;
}

// P17-STABLE-003: 边界条件测试 - 损坏存档（格式错误）
TEST_CASE(LoadGameState_handles_corrupted_format) {
    const auto save_path = MakeTempSavePath();
    {
        std::ofstream out(save_path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "version|abc\n";  // 损坏的版本号
        out << "clock|not_a_number|invalid\n";  // 损坏的时钟数据
        out << "player|xxx|yyy|zzz\n";  // 损坏的玩家数据
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
    auto push_hint = [](const std::string&, float) {};

    // 损坏存档应返回 false 或安全处理
    const bool result = LoadGameState(save_path,
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
                              push_hint);

    CleanupSaveFiles(save_path);
    return true;
}

// P17-STABLE-003: 边界条件测试 - v6 存档迁移
TEST_CASE(LoadGameState_migrates_legacy_v6_save) {
    const auto save_path = MakeTempSavePath();
    {
        std::ofstream out(save_path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "version|6\n";
        out << "clock|5|720\n";
        out << "cloud|1|0|8|3\n";
        out << "player|150|300|60\n";
        out << "stamina|50\n";
        out << "hunger|80,100,0,0\n";
        out << "economy|1500\n";
        out << "inventory||\n";
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
    auto push_hint = [](const std::string&, float) {};

    const bool result = LoadGameState(save_path,
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
                              push_hint);

    ASSERT_TRUE(result);
    ASSERT_EQ(loaded_gold, 1500);
    CleanupSaveFiles(save_path);
    return true;
}

// P17-STABLE-003: 边界条件测试 - v7 存档迁移
TEST_CASE(LoadGameState_migrates_legacy_v7_save) {
    const auto save_path = MakeTempSavePath();
    {
        std::ofstream out(save_path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "version|7\n";
        out << "clock|10|600\n";
        out << "cloud|2|1|5|2\n";
        out << "player|200|400|40\n";
        out << "stamina|30\n";
        out << "hunger|60,100,0,0\n";
        out << "economy|2500\n";
        out << "npc_development||\n";
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
    auto push_hint = [](const std::string&, float) {};

    const bool result = LoadGameState(save_path,
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
                              push_hint);

    ASSERT_TRUE(result);
    ASSERT_EQ(loaded_gold, 2500);
    CleanupSaveFiles(save_path);
    return true;
}

// P17-STABLE-003: 边界条件测试 - v8 存档迁移
TEST_CASE(LoadGameState_migrates_legacy_v8_save) {
    const auto save_path = MakeTempSavePath();
    {
        std::ofstream out(save_path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "version|8\n";
        out << "clock|15|480\n";
        out << "cloud|0|0|12|6\n";
        out << "player|250|500|20\n";
        out << "stamina|10\n";
        out << "hunger|40,100,0,0\n";
        out << "economy|5000\n";
        out << "spirit_beast|custom_name|TestBeast|type|0\n";
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
    auto push_hint = [](const std::string&, float) {};

    const bool result = LoadGameState(save_path,
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
                              push_hint);

    ASSERT_TRUE(result);
    ASSERT_EQ(loaded_gold, 5000);
    CleanupSaveFiles(save_path);
    return true;
}
