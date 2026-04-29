#include "../TestFramework.hpp"
#include "CloudSeamanor/infrastructure/GameConfig.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>

using CloudSeamanor::infrastructure::GameConfig;

namespace {

std::filesystem::path MakeTempConfigPath() {
    const auto base = std::filesystem::temp_directory_path();
    const auto unique = "cloudseamanor_game_config_test_" + std::to_string(std::rand()) + ".cfg";
    return base / unique;
}

}  // namespace

TEST_CASE(GameConfig_trim_handles_blank_lines_without_crash) {
    const auto temp_path = MakeTempConfigPath();
    {
        std::ofstream out(temp_path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "   \n";
        out << "\t\t\r\n";
        out << "movement_speed = 3.5\n";
        out << "player_name=\"Cloud\"\n";
    }

    GameConfig config;
    ASSERT_TRUE(config.LoadFromFile(temp_path.string()));
    ASSERT_FLOAT_EQ(config.GetFloat("movement_speed", 0.0f), 3.5f, 0.0001f);
    ASSERT_EQ(config.GetString("player_name", ""), "Cloud");

    std::error_code ec;
    std::filesystem::remove(temp_path, ec);
    return true;
}

