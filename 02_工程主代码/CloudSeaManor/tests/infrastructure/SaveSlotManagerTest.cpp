#include "../TestFramework.hpp"
#include "CloudSeamanor/infrastructure/SaveSlotManager.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>

using CloudSeamanor::infrastructure::SaveSlotManager;
using CloudSeamanor::infrastructure::SaveSlotMetadata;

namespace {

std::filesystem::path MakeTempSaveDir() {
    const auto base = std::filesystem::temp_directory_path();
    const auto unique = "cloudseamanor_save_test_" + std::to_string(std::rand());
    return base / unique;
}

}  // namespace

TEST_CASE(SaveSlotManager_roundtrip_metadata_read_write) {
    const auto temp_dir = MakeTempSaveDir();
    SaveSlotManager manager(temp_dir);

    SaveSlotMetadata input;
    input.slot_index = 1;
    input.saved_at_text = "2026-04-24 10:00";
    input.day = 7;
    input.season_text = "春";
    input.thumbnail_path = "thumbs/slot1.png";
    input.summary_text = "主线阶段 2 | 关键成就 3 | 战斗 victory";
    input.main_plot_stage = 2;
    input.key_achievement_count = 3;
    input.last_battle_outcome = "victory";

    ASSERT_TRUE(manager.WriteMetadata(1, input));
    const SaveSlotMetadata output = manager.ReadMetadata(1);

    ASSERT_EQ(output.slot_index, 1);
    ASSERT_EQ(output.saved_at_text, input.saved_at_text);
    ASSERT_EQ(output.day, input.day);
    ASSERT_EQ(output.season_text, input.season_text);
    ASSERT_EQ(output.thumbnail_path, input.thumbnail_path);
    ASSERT_EQ(output.summary_text, input.summary_text);
    ASSERT_EQ(output.main_plot_stage, input.main_plot_stage);
    ASSERT_EQ(output.key_achievement_count, input.key_achievement_count);
    ASSERT_EQ(output.last_battle_outcome, input.last_battle_outcome);

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
    return true;
}

TEST_CASE(SaveSlotManager_rejects_invalid_slot_index) {
    const auto temp_dir = MakeTempSaveDir();
    SaveSlotManager manager(temp_dir);

    SaveSlotMetadata input;
    input.slot_index = 5;
    input.day = 1;

    ASSERT_TRUE(!manager.WriteMetadata(0, input));
    ASSERT_TRUE(!manager.WriteMetadata(4, input));
    ASSERT_TRUE(!manager.IsValidSlot(0));
    ASSERT_TRUE(!manager.IsValidSlot(4));

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
    return true;
}

TEST_CASE(SaveSlotManager_tolerates_malformed_day_field) {
    const auto temp_dir = MakeTempSaveDir();
    SaveSlotManager manager(temp_dir);
    manager.EnsureDirectories();

    {
        std::ofstream out(manager.BuildMetaPath(1), std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "saved_at=2026-04-24 10:00\n";
        out << "day=not_a_number\n";
        out << "season=夏\n";
        out << "thumbnail=thumbs/slot1.png\n";
        out << "main_plot_stage=invalid\n";
    }

    const SaveSlotMetadata output = manager.ReadMetadata(1);
    ASSERT_EQ(output.day, 0);
    ASSERT_TRUE(output.metadata_corrupted);
    ASSERT_EQ(output.season_text, "夏");
    ASSERT_EQ(output.thumbnail_path, "thumbs/slot1.png");
    ASSERT_EQ(output.main_plot_stage, 0);

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
    return true;
}

TEST_CASE(SaveSlotManager_marks_metadata_corruption_in_aggregate_read) {
    const auto temp_dir = MakeTempSaveDir();
    SaveSlotManager manager(temp_dir);
    manager.EnsureDirectories();

    {
        std::ofstream out(manager.BuildMetaPath(3), std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "saved_at=2026-04-24 10:00\n";
        out << "day=999999999999999999999\n";
        out << "season=冬\n";
    }

    const auto all = manager.ReadAllMetadata();
    ASSERT_EQ(all.size(), static_cast<std::size_t>(3));
    ASSERT_TRUE(all[2].metadata_corrupted);
    ASSERT_EQ(all[2].day, 0);

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
    return true;
}

TEST_CASE(SaveSlotManager_read_all_metadata_returns_three_slots) {
    const auto temp_dir = MakeTempSaveDir();
    SaveSlotManager manager(temp_dir);

    SaveSlotMetadata slot2;
    slot2.slot_index = 2;
    slot2.day = 3;
    slot2.saved_at_text = "2026-04-24";
    ASSERT_TRUE(manager.WriteMetadata(2, slot2));

    {
        std::ofstream save_file(manager.BuildSlotPath(2), std::ios::trunc);
        ASSERT_TRUE(save_file.is_open());
        save_file << "version|6\n";
    }

    const auto all = manager.ReadAllMetadata();
    ASSERT_EQ(all.size(), static_cast<std::size_t>(3));
    ASSERT_EQ(all[1].slot_index, 2);
    ASSERT_TRUE(all[1].exists);
    ASSERT_EQ(all[1].day, 3);

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
    return true;
}
