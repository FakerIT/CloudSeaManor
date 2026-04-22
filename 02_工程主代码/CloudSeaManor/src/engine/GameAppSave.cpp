#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/DynamicLifeSystem.hpp"
#include "CloudSeamanor/GameAppSave.hpp"

#include "CloudSeamanor/CloudSystem.hpp"
#include "CloudSeamanor/CropData.hpp"
#include "CloudSeamanor/FestivalSystem.hpp"
#include "CloudSeamanor/GameAppText.hpp"
#include "CloudSeamanor/GameClock.hpp"
#include "CloudSeamanor/Inventory.hpp"
#include "CloudSeamanor/Player.hpp"
#include "CloudSeamanor/Result.hpp"
#include "CloudSeamanor/SkillSystem.hpp"
#include "CloudSeamanor/Stamina.hpp"
#include "CloudSeamanor/WorkshopSystem.hpp"
#include "CloudSeamanor/GameAppFarming.hpp"
#include "CloudSeamanor/GameAppSpiritBeast.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <functional>
#include <sstream>

namespace CloudSeamanor::engine {

namespace {

std::vector<std::string> SplitSaveFields(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    for (char ch : line) {
        if (ch == '|') {
            fields.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    fields.push_back(current);
    return fields;
}

std::uint32_t ComputeChecksum(const std::string& text) {
    // FNV-1a 32-bit checksum (fast and deterministic).
    std::uint32_t hash = 2166136261u;
    for (unsigned char ch : text) {
        hash ^= static_cast<std::uint32_t>(ch);
        hash *= 16777619u;
    }
    return hash;
}

std::string BuildPayload(const std::vector<std::string>& lines) {
    std::ostringstream oss;
    for (const auto& l : lines) {
        oss << l << '\n';
    }
    return oss.str();
}

constexpr int kSaveVersion = 6;

std::filesystem::path BackupPathFor(const std::filesystem::path& save_path) {
    return save_path.string() + ".bak";
}

std::filesystem::path TempPathFor(const std::filesystem::path& save_path) {
    return save_path.string() + ".tmp";
}

bool WriteAtomically(
    const std::filesystem::path& save_path,
    const std::vector<std::string>& lines,
    const std::function<void(const std::string&, float)>& push_hint) {
    const auto tmp_path = TempPathFor(save_path);
    const auto bak_path = BackupPathFor(save_path);

    {
        std::ofstream tmp_out(tmp_path, std::ios::trunc);
        if (!tmp_out.is_open()) {
            push_hint("无法写入临时存档文件。", 2.8f);
            return false;
        }
        for (const auto& line : lines) {
            tmp_out << line << '\n';
        }
        tmp_out.flush();
        if (!tmp_out.good()) {
            push_hint("临时存档写入失败。", 2.8f);
            return false;
        }
    }

    std::error_code ec;
    if (std::filesystem::exists(save_path, ec)) {
        std::filesystem::copy_file(
            save_path,
            bak_path,
            std::filesystem::copy_options::overwrite_existing,
            ec);
    }

    std::filesystem::remove(save_path, ec);
    std::filesystem::rename(tmp_path, save_path, ec);
    if (ec) {
        push_hint("存档提交失败，已保留临时文件。", 3.0f);
        return false;
    }

    return true;
}

Result<int> ParseInt(const std::string& text, const std::string& field_name) {
    try {
        return std::stoi(text);
    } catch (...) {
        return Result<int>("字段解析失败（int）: " + field_name + "='" + text + "'");
    }
}

Result<float> ParseFloat(const std::string& text, const std::string& field_name) {
    try {
        return std::stof(text);
    } catch (...) {
        return Result<float>("字段解析失败（float）: " + field_name + "='" + text + "'");
    }
}

Result<bool> ParseBool01(const std::string& text, const std::string& field_name) {
    auto parsed = ParseInt(text, field_name);
    if (!parsed) {
        return Result<bool>(parsed.Error());
    }
    if (parsed.Value() != 0 && parsed.Value() != 1) {
        return Result<bool>("字段取值非法（bool期望0或1）: " + field_name + "='" + text + "'");
    }
    return parsed.Value() != 0;
}

int SpiritBeastPersonalityToInt(SpiritBeastPersonality personality) {
    return static_cast<int>(personality);
}

SpiritBeastPersonality SpiritBeastPersonalityFromInt(int value) {
    switch (value) {
    case 1: return SpiritBeastPersonality::Lazy;
    case 2: return SpiritBeastPersonality::Curious;
    case 0:
    default:
        return SpiritBeastPersonality::Lively;
    }
}

void ResetLoadTargets(CloudSeamanor::domain::Inventory& inventory,
                      std::vector<PriceTableEntry>& price_table,
                      std::vector<MailOrderEntry>& mail_orders,
                      std::unordered_map<std::string, int>* weekly_buy_count,
                      std::unordered_map<std::string, int>* weekly_sell_count,
                      std::unordered_map<std::string, bool>* achievements) {
    inventory.Clear();
    price_table.clear();
    mail_orders.clear();
    if (weekly_buy_count) {
        weekly_buy_count->clear();
    }
    if (weekly_sell_count) {
        weekly_sell_count->clear();
    }
    if (achievements) {
        achievements->clear();
    }
}

std::vector<std::string> ReadNonEmptyLines(const std::filesystem::path& path) {
    std::vector<std::string> result;
    std::ifstream in(path);
    if (!in.is_open()) {
        return result;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) {
            result.push_back(line);
        }
    }
    return result;
}

std::vector<std::string> MigrateLegacyLinesToV5(
    const std::vector<std::string>& legacy_lines,
    int save_version) {
    if (save_version >= 5) {
        return legacy_lines;
    }
    std::vector<std::string> migrated;
    migrated.reserve(legacy_lines.size());
    for (const auto& line : legacy_lines) {
        const auto fields = SplitSaveFields(line);
        if (fields.empty()) {
            continue;
        }
        // v1~v3 历史格式兼容：mail_order|item|count|day -> mail|item|day|count
        if (fields[0] == "mail_order" && fields.size() >= 4) {
            migrated.push_back("mail|" + fields[1] + "|" + fields[3] + "|" + fields[2]);
            continue;
        }
        migrated.push_back(line);
    }
    return migrated;
}

bool PrepareLoadLines(const std::filesystem::path& save_path,
                      std::vector<std::string>& all_lines,
                      std::size_t& data_start,
                      int& save_version,
                      const std::function<void(const std::string&, float)>& push_hint) {
    all_lines = ReadNonEmptyLines(save_path);
    if (all_lines.empty()) {
        const auto bak_lines = ReadNonEmptyLines(BackupPathFor(save_path));
        if (!bak_lines.empty()) {
            all_lines = bak_lines;
            push_hint("主存档无效，已自动回退到备份。", 3.0f);
        }
    }
    if (all_lines.empty()) {
        push_hint("存档文件为空。", 2.6f);
        return false;
    }

    data_start = 0;
    save_version = 1;
    {
        const auto version_fields = SplitSaveFields(all_lines.front());
        if (version_fields.size() >= 2 && version_fields[0] == "version") {
            auto parsed = ParseInt(version_fields[1], "header.version");
            if (!parsed) {
                push_hint(parsed.Error(), 3.0f);
                return false;
            }
            save_version = parsed.Value();
            data_start = 1;
        }
    }
    if (save_version > kSaveVersion) {
        push_hint(
            "存档版本过高（v" + std::to_string(save_version)
            + "），当前版本仅支持到 v" + std::to_string(kSaveVersion) + "。",
            3.6f);
        return false;
    }
    {
        const auto header = SplitSaveFields(all_lines[data_start]);
        if (header.size() >= 2 && header[0] == "checksum") {
            std::uint32_t expected = 0;
            try {
                expected = static_cast<std::uint32_t>(std::stoul(header[1]));
            } catch (...) {
                push_hint("存档校验头损坏。", 3.0f);
                return false;
            }
            std::vector<std::string> payload_lines(all_lines.begin() + static_cast<long long>(data_start + 1), all_lines.end());
            const std::uint32_t actual = ComputeChecksum(BuildPayload(payload_lines));
            if (actual != expected) {
                const auto bak_lines = ReadNonEmptyLines(BackupPathFor(save_path));
                if (!bak_lines.empty()) {
                    push_hint("主存档校验失败，已尝试回退到备份。", 3.2f);
                } else {
                    push_hint("存档校验失败，文件可能已损坏或被篡改。", 3.6f);
                    return false;
                }
                all_lines = bak_lines;
                data_start = 0;
                const auto bak_v = SplitSaveFields(all_lines.front());
                if (bak_v.size() >= 2 && bak_v[0] == "version") {
                    auto parsed = ParseInt(bak_v[1], "backup.version");
                    if (!parsed) {
                        push_hint(parsed.Error(), 3.0f);
                        return false;
                    }
                    save_version = parsed.Value();
                    data_start = 1;
                }
                const auto bak_header = SplitSaveFields(all_lines[data_start]);
                if (bak_header.size() >= 2 && bak_header[0] == "checksum") {
                    std::uint32_t bak_expected = 0;
                    try {
                        bak_expected = static_cast<std::uint32_t>(std::stoul(bak_header[1]));
                    } catch (...) {
                        push_hint("备份存档校验头损坏。", 3.0f);
                        return false;
                    }
                    std::vector<std::string> bak_payload(all_lines.begin() + static_cast<long long>(data_start + 1), all_lines.end());
                    const std::uint32_t bak_actual = ComputeChecksum(BuildPayload(bak_payload));
                    if (bak_actual != bak_expected) {
                        push_hint("备份存档校验失败。", 3.2f);
                        return false;
                    }
                    data_start += 1;
                }
            } else {
                data_start += 1;
            }
        }
    }
    if (save_version < 5) {
        std::vector<std::string> payload_lines(all_lines.begin() + static_cast<long long>(data_start), all_lines.end());
        payload_lines = MigrateLegacyLinesToV5(payload_lines, save_version);
        all_lines.resize(data_start);
        all_lines.insert(all_lines.end(), payload_lines.begin(), payload_lines.end());
    }
    return true;
}

} // namespace

// ============================================================================
// 【SaveGameState】保存游戏状态
// ============================================================================
bool SaveGameState(const std::filesystem::path& save_path,
                   const CloudSeamanor::domain::GameClock& clock,
                   const CloudSeamanor::domain::CloudSystem& cloud_system,
                   const CloudSeamanor::domain::Player& player,
                   const CloudSeamanor::domain::StaminaSystem& stamina,
                   const RepairProject& main_house_repair,
                   const TeaMachine& tea_machine,
                   const SpiritBeast& spirit_beast,
                   bool spirit_beast_watered_today,
                   const std::vector<TeaPlot>& tea_plots,
                   int gold,
                   const std::vector<PriceTableEntry>& price_table,
                   const std::vector<MailOrderEntry>& mail_orders,
                   const CloudSeamanor::domain::Inventory& inventory,
                   const std::vector<NpcActor>& npcs,
                   const std::function<void(const std::string&, float)>& push_hint,
                   const CloudSeamanor::domain::SkillSystem* skills,
                   const CloudSeamanor::domain::FestivalSystem* festivals,
                   const CloudSeamanor::domain::DynamicLifeSystem* dynamic_life,
                   const CloudSeamanor::domain::WorkshopSystem* workshop,
                   const int* decoration_score,
                   const std::string* pet_type,
                   const bool* pet_adopted,
                   const std::unordered_map<std::string, bool>* achievements,
                   const std::unordered_map<std::string, int>* weekly_buy_count,
                   const std::unordered_map<std::string, int>* weekly_sell_count,
                   const int* spirit_realm_daily_max,
                   const int* spirit_realm_daily_remaining,
                   const bool* in_battle_mode,
                   const int* battle_state,
                   const TutorialState* tutorial) {
    std::filesystem::create_directories(save_path.parent_path());

    std::vector<std::string> lines;
    lines.push_back("version|" + std::to_string(kSaveVersion));
    lines.push_back("clock|" + std::to_string(clock.Day()) + "|" + std::to_string(clock.TimeOfDayMinutes()));
    lines.push_back(
        "cloud|" + std::to_string(static_cast<int>(cloud_system.CurrentState())) + "|"
        + std::to_string(static_cast<int>(cloud_system.ForecastState())) + "|"
        + std::to_string(cloud_system.SpiritEnergy()) + "|"
        + std::to_string(cloud_system.TotalPlayerInfluence()));
    const auto player_pos = player.GetPosition();
    lines.push_back(
        "player|" + std::to_string(player_pos.x) + "|"
        + std::to_string(player_pos.y) + "|"
        + std::to_string(stamina.Current()));
    lines.push_back(
        "repair|" + std::to_string(main_house_repair.completed ? 1 : 0) + "|"
        + std::to_string(main_house_repair.level) + "|"
        + std::to_string(main_house_repair.build_days_left) + "|"
        + std::to_string(main_house_repair.wood_cost) + "|"
        + std::to_string(main_house_repair.turnip_cost) + "|"
        + std::to_string(main_house_repair.gold_cost));
    lines.push_back(
        "machine|" + std::to_string(tea_machine.running ? 1 : 0) + "|"
        + std::to_string(tea_machine.progress) + "|"
        + std::to_string(tea_machine.duration) + "|"
        + std::to_string(tea_machine.queued_output));
    lines.push_back(
        "spirit|" + std::to_string(spirit_beast.shape.getPosition().x) + "|"
        + std::to_string(spirit_beast.shape.getPosition().y) + "|"
        + std::to_string(SpiritBeastStateToInt(spirit_beast.state)) + "|"
        + std::to_string(static_cast<int>(spirit_beast.patrol_index)) + "|"
        + std::to_string(spirit_beast.idle_timer) + "|"
        + std::to_string(spirit_beast.interact_timer) + "|"
        + std::to_string(spirit_beast.daily_interacted ? 1 : 0) + "|"
        + std::to_string(spirit_beast.last_interaction_day) + "|"
        + std::to_string(spirit_beast_watered_today ? 1 : 0) + "|"
        + std::to_string(spirit_beast.favor) + "|"
        + std::to_string(spirit_beast.dispatched_for_pest_control ? 1 : 0) + "|"
        + spirit_beast.custom_name + "|"
        + std::to_string(SpiritBeastPersonalityToInt(spirit_beast.personality)));
    lines.push_back("economy|" + std::to_string(gold));
    if (decoration_score) {
        lines.push_back("decor|" + std::to_string(*decoration_score));
    }
    if (pet_adopted && pet_type) {
        lines.push_back("pet|" + std::to_string(*pet_adopted ? 1 : 0) + "|" + *pet_type);
    }
    if (achievements) {
        for (const auto& [id, unlocked] : *achievements) {
            lines.push_back("ach|" + id + "|" + std::to_string(unlocked ? 1 : 0));
        }
    }
    if (tutorial) {
        lines.push_back(
            "tutorial|" + std::to_string(static_cast<unsigned int>(tutorial->tutorial_bubble_completed_mask)) + "|"
            + std::to_string(tutorial->tutorial_bubble_step) + "|"
            + std::to_string(tutorial->daily_cloud_report_day_shown));
    }
    if (in_battle_mode && battle_state) {
        lines.push_back(
            "battle|" + std::to_string(*in_battle_mode ? 1 : 0) + "|"
            + std::to_string(*battle_state));
    }
    if (spirit_realm_daily_max && spirit_realm_daily_remaining) {
        lines.push_back(
            "spirit_realm|" + std::to_string(std::max(1, *spirit_realm_daily_max)) + "|"
            + std::to_string(std::max(0, *spirit_realm_daily_remaining)));
    }

    // v4 存档格式：
    // plot|index|tilled|seeded|watered|ready|growth|stage|crop_id|quality|
    // sprinkler_installed|sprinkler_days_left|fertilized|fertilizer_type|in_greenhouse|
    // cleared|disease|pest|disease_days
    for (std::size_t i = 0; i < tea_plots.size(); ++i) {
        const auto& plot = tea_plots[i];
        lines.push_back(
            "plot|" + std::to_string(i) + "|"
            + std::to_string(plot.tilled ? 1 : 0) + "|"
            + std::to_string(plot.seeded ? 1 : 0) + "|"
            + std::to_string(plot.watered ? 1 : 0) + "|"
            + std::to_string(plot.ready ? 1 : 0) + "|"
            + std::to_string(plot.growth) + "|"
            + std::to_string(plot.stage) + "|"
            + plot.crop_id + "|"
            + std::to_string(static_cast<int>(plot.quality)) + "|"
            + std::to_string(plot.sprinkler_installed ? 1 : 0) + "|"
            + std::to_string(plot.sprinkler_days_left) + "|"
            + std::to_string(plot.fertilized ? 1 : 0) + "|"
            + plot.fertilizer_type + "|"
            + std::to_string(plot.in_greenhouse ? 1 : 0) + "|"
            + std::to_string(plot.cleared ? 1 : 0) + "|"
            + std::to_string(plot.disease ? 1 : 0) + "|"
            + std::to_string(plot.pest ? 1 : 0) + "|"
            + std::to_string(plot.disease_days));
    }

    for (const auto& p : price_table) {
        lines.push_back(
            "price|" + p.item_id + "|"
            + std::to_string(p.buy_price) + "|"
            + std::to_string(p.sell_price) + "|"
            + p.buy_from + "|"
            + p.category);
    }
    for (const auto& m : mail_orders) {
        // 采用补充任务定义中的 mail 标签格式。
        lines.push_back(
            "mail|" + m.item_id + "|"
            + std::to_string(m.deliver_day) + "|"
            + std::to_string(m.count));
    }
    if (weekly_buy_count) {
        for (const auto& [item_id, count] : *weekly_buy_count) {
            lines.push_back("weekly_buy|" + item_id + "|" + std::to_string(count));
        }
    }
    if (weekly_sell_count) {
        for (const auto& [item_id, count] : *weekly_sell_count) {
            lines.push_back("weekly_sell|" + item_id + "|" + std::to_string(count));
        }
    }

    for (const auto& slot : inventory.Slots()) {
        lines.push_back("inventory|" + slot.item_id + "|" + std::to_string(slot.count));
    }

    for (const auto& npc : npcs) {
        lines.push_back(
            "npc|" + npc.id + "|"
            + std::to_string(npc.shape.getPosition().x) + "|"
            + std::to_string(npc.shape.getPosition().y) + "|"
            + std::to_string(npc.favor) + "|"
            + std::to_string(npc.daily_gifted ? 1 : 0) + "|"
            + npc.current_location + "|" + npc.current_activity + "|"
            + std::to_string(npc.heart_level) + "|"
            + std::to_string(npc.daily_favor_gain) + "|"
            + std::to_string(npc.last_gift_day) + "|"
            + std::to_string(npc.daily_talked ? 1 : 0) + "|"
            + std::to_string(npc.last_talk_day));
    }

    if (skills) {
        lines.push_back("skills|" + skills->SaveState());
    }

    if (festivals) {
        for (const auto& fest : festivals->GetAllFestivals()) {
            lines.push_back(
                "festival|" + fest.id + "|"
                + std::to_string(fest.participated ? 1 : 0));
        }
    }

    if (dynamic_life) {
        for (const auto& npc : npcs) {
            if (const auto* state = dynamic_life->GetNpcState(npc.id)) {
                lines.push_back(
                    "dynamic_life|" + npc.id + "|"
                    + std::to_string(static_cast<int>(state->stage)) + "|"
                    + std::to_string(state->progress_points));
            }
        }
    }

    if (workshop) {
        for (const auto& machine : workshop->GetMachines()) {
            lines.push_back(
                "workshop_machine|" + machine.machine_id + "|"
                + machine.recipe_id + "|"
                + std::to_string(machine.progress) + "|"
                + std::to_string(machine.is_processing ? 1 : 0));
        }
    }

    const std::string payload = BuildPayload(lines);
    std::vector<std::string> final_lines;
    final_lines.reserve(lines.size() + 1);
    final_lines.push_back("checksum|" + std::to_string(ComputeChecksum(payload)));
    final_lines.insert(final_lines.end(), lines.begin(), lines.end());
    if (!WriteAtomically(save_path, final_lines, push_hint)) {
        return false;
    }

    push_hint("游戏已保存到 save_slot_01。", 2.4f);
    return true;
}

// ============================================================================
// 【LoadGameState】加载游戏状态
// ============================================================================
bool LoadGameState(const std::filesystem::path& save_path,
                   CloudSeamanor::domain::GameClock& clock,
                   CloudSeamanor::domain::CloudSystem& cloud_system,
                   CloudSeamanor::domain::Player& player,
                   CloudSeamanor::domain::StaminaSystem& stamina,
                   RepairProject& main_house_repair,
                   TeaMachine& tea_machine,
                   SpiritBeast& spirit_beast,
                   bool& spirit_beast_watered_today,
                   std::vector<TeaPlot>& tea_plots,
                   int& gold,
                   std::vector<PriceTableEntry>& price_table,
                   std::vector<MailOrderEntry>& mail_orders,
                   CloudSeamanor::domain::Inventory& inventory,
                   std::vector<NpcActor>& npcs,
                   std::vector<sf::RectangleShape>& obstacle_shapes,
                   CloudSeamanor::domain::CloudState& last_cloud_state,
                   const std::function<void()>& refresh_spirit_beast_visual,
                   const std::function<void(TeaPlot&, bool)>& refresh_tea_plot_visual,
                   const std::function<void()>& update_highlighted_interactable,
                   const std::function<void()>& update_hud_text,
                   const std::function<void(const std::string&, float)>& push_hint,
                   CloudSeamanor::domain::SkillSystem* skills,
                   CloudSeamanor::domain::FestivalSystem* festivals,
                   CloudSeamanor::domain::DynamicLifeSystem* dynamic_life,
                   CloudSeamanor::domain::WorkshopSystem* workshop,
                   NpcDialogueManager* dialogue_manager,
                   int* decoration_score,
                   std::string* pet_type,
                   bool* pet_adopted,
                   std::unordered_map<std::string, bool>* achievements,
                   std::unordered_map<std::string, int>* weekly_buy_count,
                   std::unordered_map<std::string, int>* weekly_sell_count,
                   int* spirit_realm_daily_max,
                   int* spirit_realm_daily_remaining,
                   bool* in_battle_mode,
                   int* battle_state,
                   TutorialState* tutorial) {
    ResetLoadTargets(
        inventory,
        price_table,
        mail_orders,
        weekly_buy_count,
        weekly_sell_count,
        achievements);

    std::vector<std::string> all_lines;
    std::size_t data_start = 0;
    int save_version = 1;
    if (!PrepareLoadLines(save_path, all_lines, data_start, save_version, push_hint)) {
        return false;
    }
    if (save_version < kSaveVersion) {
        push_hint(
            "检测到旧版存档 v" + std::to_string(save_version)
            + "，已执行兼容迁移到 v" + std::to_string(kSaveVersion) + " 读取流程。",
            2.8f);
    }

    for (std::size_t i = data_start; i < all_lines.size(); ++i) {
        const auto fields = SplitSaveFields(all_lines[i]);
        if (fields.empty()) {
            continue;
        }

        const auto& tag = fields[0];
        if (tag == "clock" && fields.size() >= 3) {
            auto day = ParseInt(fields[1], "clock.day");
            auto minutes = ParseFloat(fields[2], "clock.minutes");
            if (!day || !minutes) {
                push_hint(!day ? day.Error() : minutes.Error(), 3.2f);
                return false;
            }
            clock.SetState(day.Value(), minutes.Value());
        } else if (tag == "tutorial" && fields.size() >= 3 && tutorial) {
            auto mask = ParseInt(fields[1], "tutorial.mask");
            auto step = ParseInt(fields[2], "tutorial.step");
            if (!mask || !step) {
                push_hint(!mask ? mask.Error() : step.Error(), 3.2f);
                return false;
            }
            tutorial->tutorial_bubble_completed_mask = static_cast<std::uint16_t>(std::clamp(mask.Value(), 0, 0xFFFF));
            tutorial->tutorial_bubble_step = std::max(1, std::min(11, step.Value()));
            if (fields.size() >= 4) {
                auto shown = ParseInt(fields[3], "tutorial.daily_cloud_report_day_shown");
                if (!shown) {
                    push_hint(shown.Error(), 3.2f);
                    return false;
                }
                tutorial->daily_cloud_report_day_shown = shown.Value();
            }
        } else if (tag == "battle" && fields.size() >= 3) {
            if (in_battle_mode) {
                auto in_battle = ParseBool01(fields[1], "battle.in_battle_mode");
                if (in_battle) {
                    *in_battle_mode = in_battle.Value();
                }
            }
            if (battle_state) {
                auto state = ParseInt(fields[2], "battle.state");
                if (state) {
                    *battle_state = state.Value();
                }
            }
        } else if (tag == "spirit_realm" && fields.size() >= 3) {
            auto daily_max = ParseInt(fields[1], "spirit_realm.daily_max");
            auto daily_remaining = ParseInt(fields[2], "spirit_realm.daily_remaining");
            if (!daily_max || !daily_remaining) {
                push_hint(!daily_max ? daily_max.Error() : daily_remaining.Error(), 3.2f);
                return false;
            }
            if (spirit_realm_daily_max) {
                *spirit_realm_daily_max = std::max(1, daily_max.Value());
            }
            if (spirit_realm_daily_remaining) {
                const int clamped_max = spirit_realm_daily_max ? *spirit_realm_daily_max : std::max(1, daily_max.Value());
                *spirit_realm_daily_remaining = std::clamp(daily_remaining.Value(), 0, clamped_max);
            }
        } else if (tag == "cloud" && fields.size() >= 3) {
            auto current_state = ParseInt(fields[1], "cloud.current_state");
            auto forecast_state = ParseInt(fields[2], "cloud.forecast_state");
            if (!current_state || !forecast_state) {
                push_hint(!current_state ? current_state.Error() : forecast_state.Error(), 3.2f);
                return false;
            }
            cloud_system.SetStates(
                static_cast<CloudSeamanor::domain::CloudState>(current_state.Value()),
                static_cast<CloudSeamanor::domain::CloudState>(forecast_state.Value()));
            if (fields.size() >= 5) {
                auto spirit = ParseInt(fields[3], "cloud.spirit_energy");
                auto influence = ParseInt(fields[4], "cloud.total_player_influence");
                if (!spirit || !influence) {
                    push_hint(!spirit ? spirit.Error() : influence.Error(), 3.2f);
                    return false;
                }
                cloud_system.SetSpiritEnergy(spirit.Value());
                cloud_system.ApplyPlayerInfluence(
                    influence.Value() - cloud_system.TotalPlayerInfluence());
            }
            cloud_system.UpdateForecastVisibility(clock.Day(), clock.Hour());
            last_cloud_state = cloud_system.CurrentState();
        } else if (tag == "player" && fields.size() >= 4) {
            auto pos_x = ParseFloat(fields[1], "player.x");
            auto pos_y = ParseFloat(fields[2], "player.y");
            auto stamina_current = ParseFloat(fields[3], "player.stamina");
            if (!pos_x || !pos_y || !stamina_current) {
                push_hint(!pos_x ? pos_x.Error() : (!pos_y ? pos_y.Error() : stamina_current.Error()), 3.2f);
                return false;
            }
            player.SetPosition({pos_x.Value(), pos_y.Value()});
            stamina.SetCurrent(stamina_current.Value());
        } else if (tag == "repair" && fields.size() >= 2) {
            auto completed = ParseBool01(fields[1], "repair.completed");
            if (!completed) {
                push_hint(completed.Error(), 3.2f);
                return false;
            }
            main_house_repair.completed = completed.Value();
            if (fields.size() >= 7) {
                auto level = ParseInt(fields[2], "repair.level");
                auto build_days_left = ParseInt(fields[3], "repair.build_days_left");
                auto wood_cost = ParseInt(fields[4], "repair.wood_cost");
                auto turnip_cost = ParseInt(fields[5], "repair.turnip_cost");
                auto gold_cost = ParseInt(fields[6], "repair.gold_cost");
                if (!level || !build_days_left || !wood_cost || !turnip_cost || !gold_cost) {
                    push_hint("建筑存档字段解析失败。", 3.2f);
                    return false;
                }
                main_house_repair.level = level.Value();
                main_house_repair.build_days_left = build_days_left.Value();
                main_house_repair.wood_cost = wood_cost.Value();
                main_house_repair.turnip_cost = turnip_cost.Value();
                main_house_repair.gold_cost = gold_cost.Value();
            }
            if (!obstacle_shapes.empty()) {
                if (main_house_repair.completed) {
                    obstacle_shapes.front().setFillColor(sf::Color(148, 122, 92));
                    obstacle_shapes.front().setOutlineColor(sf::Color(86, 62, 38));
                } else {
                    obstacle_shapes.front().setFillColor(sf::Color(112, 88, 72));
                    obstacle_shapes.front().setOutlineColor(sf::Color(64, 47, 35));
                }
            }
        } else if (tag == "machine" && fields.size() >= 5) {
            auto running = ParseBool01(fields[1], "machine.running");
            auto progress = ParseFloat(fields[2], "machine.progress");
            auto duration = ParseFloat(fields[3], "machine.duration");
            auto queued_output = ParseInt(fields[4], "machine.queued_output");
            if (!running || !progress || !duration || !queued_output) {
                push_hint(!running ? running.Error()
                                   : (!progress ? progress.Error()
                                                : (!duration ? duration.Error() : queued_output.Error())), 3.2f);
                return false;
            }
            tea_machine.running = running.Value();
            tea_machine.progress = progress.Value();
            tea_machine.duration = duration.Value();
            tea_machine.queued_output = queued_output.Value();
        } else if (tag == "spirit" && fields.size() >= 10) {
            auto pos_x = ParseFloat(fields[1], "spirit.x");
            auto pos_y = ParseFloat(fields[2], "spirit.y");
            auto state = ParseInt(fields[3], "spirit.state");
            auto patrol_index = ParseInt(fields[4], "spirit.patrol_index");
            auto idle_timer = ParseFloat(fields[5], "spirit.idle_timer");
            auto interact_timer = ParseFloat(fields[6], "spirit.interact_timer");
            auto daily_interacted = ParseBool01(fields[7], "spirit.daily_interacted");
            auto last_day = ParseInt(fields[8], "spirit.last_interaction_day");
            auto watered_today = ParseBool01(fields[9], "spirit.watered_today");
            if (!pos_x || !pos_y || !state || !patrol_index || !idle_timer || !interact_timer
                || !daily_interacted || !last_day || !watered_today) {
                push_hint("灵兽存档字段解析失败。", 3.2f);
                return false;
            }
            spirit_beast.shape.setPosition({pos_x.Value(), pos_y.Value()});
            spirit_beast.state = SpiritBeastStateFromInt(state.Value());
            spirit_beast.patrol_index = static_cast<std::size_t>(std::max(0, patrol_index.Value()));
            spirit_beast.idle_timer = idle_timer.Value();
            spirit_beast.interact_timer = interact_timer.Value();
            spirit_beast.daily_interacted = daily_interacted.Value();
            spirit_beast.last_interaction_day = last_day.Value();
            spirit_beast_watered_today = watered_today.Value();
            if (fields.size() >= 11) {
                auto favor = ParseInt(fields[10], "spirit.favor");
                if (favor) {
                    spirit_beast.favor = std::max(0, favor.Value());
                }
            }
            if (fields.size() >= 12) {
                auto dispatched = ParseBool01(fields[11], "spirit.dispatched");
                if (dispatched) {
                    spirit_beast.dispatched_for_pest_control = dispatched.Value();
                }
            }
            if (fields.size() >= 13) {
                spirit_beast.custom_name = fields[12];
            }
            if (fields.size() >= 14) {
                auto personality = ParseInt(fields[13], "spirit.personality");
                if (personality) {
                    spirit_beast.personality = SpiritBeastPersonalityFromInt(personality.Value());
                }
            }
            refresh_spirit_beast_visual();
        } else if (tag == "economy" && fields.size() >= 2) {
            auto parsed_gold = ParseInt(fields[1], "economy.gold");
            if (!parsed_gold) {
                push_hint(parsed_gold.Error(), 3.2f);
                return false;
            }
            gold = parsed_gold.Value();
        } else if (tag == "decor" && fields.size() >= 2 && decoration_score) {
            auto d = ParseInt(fields[1], "decor.score");
            if (!d) {
                push_hint(d.Error(), 3.2f);
                return false;
            }
            *decoration_score = d.Value();
        } else if (tag == "pet" && fields.size() >= 3 && pet_adopted && pet_type) {
            auto adopted = ParseBool01(fields[1], "pet.adopted");
            if (!adopted) {
                push_hint(adopted.Error(), 3.2f);
                return false;
            }
            *pet_adopted = adopted.Value();
            *pet_type = fields[2];
        } else if (tag == "ach" && fields.size() >= 3 && achievements) {
            auto unlocked = ParseBool01(fields[2], "ach.unlocked");
            if (!unlocked) {
                push_hint(unlocked.Error(), 3.2f);
                return false;
            }
            (*achievements)[fields[1]] = unlocked.Value();
        } else if (tag == "plot") {
            // v2 格式：至少 8 个字段（index + 6 个状态 + stage）
            if (fields.size() < 8) continue;
            auto index = ParseInt(fields[1], "plot.index");
            if (!index) {
                push_hint(index.Error(), 3.2f);
                return false;
            }
            if (index.Value() < 0 || index.Value() >= static_cast<int>(tea_plots.size())) continue;
            auto& plot = tea_plots[static_cast<std::size_t>(index.Value())];
            auto tilled = ParseBool01(fields[2], "plot.tilled");
            auto seeded = ParseBool01(fields[3], "plot.seeded");
            auto watered = ParseBool01(fields[4], "plot.watered");
            auto ready = ParseBool01(fields[5], "plot.ready");
            auto growth = ParseFloat(fields[6], "plot.growth");
            auto stage = ParseInt(fields[7], "plot.stage");
            if (!tilled || !seeded || !watered || !ready || !growth || !stage) {
                push_hint("地块存档字段解析失败。", 3.2f);
                return false;
            }
            plot.tilled = tilled.Value();
            plot.seeded = seeded.Value();
            plot.watered = watered.Value();
            plot.ready = ready.Value();
            plot.growth = growth.Value();
            plot.stage = stage.Value();
            // v2 新字段：crop_id 和 quality（前向兼容：旧存档没有这些字段）
            if (fields.size() >= 10) {
                plot.crop_id = fields[8];
                auto quality = ParseInt(fields[9], "plot.quality");
                if (!quality) {
                    push_hint(quality.Error(), 3.2f);
                    return false;
                }
                plot.quality = static_cast<CloudSeamanor::domain::CropQuality>(quality.Value());
            }
            if (fields.size() >= 15) {
                auto sprinkler = ParseBool01(fields[10], "plot.sprinkler");
                auto sprinkler_days = ParseInt(fields[11], "plot.sprinkler_days");
                auto fertilized = ParseBool01(fields[12], "plot.fertilized");
                if (!sprinkler || !sprinkler_days || !fertilized) {
                    push_hint("地块扩展字段解析失败。", 3.2f);
                    return false;
                }
                plot.sprinkler_installed = sprinkler.Value();
                plot.sprinkler_days_left = sprinkler_days.Value();
                plot.fertilized = fertilized.Value();
                plot.fertilizer_type = fields[13];
                auto greenhouse = ParseBool01(fields[14], "plot.greenhouse");
                plot.in_greenhouse = greenhouse ? greenhouse.Value() : false;
            }
            if (fields.size() >= 19) {
                auto cleared = ParseBool01(fields[15], "plot.cleared");
                auto disease = ParseBool01(fields[16], "plot.disease");
                auto pest = ParseBool01(fields[17], "plot.pest");
                auto disease_days = ParseInt(fields[18], "plot.disease_days");
                if (!cleared || !disease || !pest || !disease_days) {
                    push_hint("地块 v4 字段解析失败。", 3.2f);
                    return false;
                }
                plot.cleared = cleared.Value();
                plot.disease = disease.Value();
                plot.pest = pest.Value();
                plot.disease_days = disease_days.Value();
            }
            refresh_tea_plot_visual(plot, false);
        } else if (tag == "price" && fields.size() >= 6) {
            PriceTableEntry p;
            p.item_id = fields[1];
            auto buy_price = ParseInt(fields[2], "price.buy_price");
            auto sell_price = ParseInt(fields[3], "price.sell_price");
            if (!buy_price || !sell_price) {
                push_hint(!buy_price ? buy_price.Error() : sell_price.Error(), 3.0f);
                return false;
            }
            p.buy_price = buy_price.Value();
            p.sell_price = sell_price.Value();
            p.buy_from = fields[4];
            p.category = fields[5];
            price_table.push_back(std::move(p));
        } else if (tag == "mail_order" && fields.size() >= 4) {
            MailOrderEntry m;
            m.item_id = fields[1];
            auto count = ParseInt(fields[2], "mail_order.count");
            auto deliver_day = ParseInt(fields[3], "mail_order.deliver_day");
            if (!count || !deliver_day) {
                push_hint(!count ? count.Error() : deliver_day.Error(), 3.0f);
                return false;
            }
            m.count = count.Value();
            m.deliver_day = deliver_day.Value();
            mail_orders.push_back(std::move(m));
        } else if (tag == "mail" && fields.size() >= 4) {
            MailOrderEntry m;
            m.item_id = fields[1];
            auto deliver_day = ParseInt(fields[2], "mail.deliver_day");
            auto count = ParseInt(fields[3], "mail.count");
            if (!deliver_day || !count) {
                push_hint(!deliver_day ? deliver_day.Error() : count.Error(), 3.0f);
                return false;
            }
            m.count = count.Value();
            m.deliver_day = deliver_day.Value();
            mail_orders.push_back(std::move(m));
        } else if (tag == "weekly_buy" && fields.size() >= 3 && weekly_buy_count) {
            auto count = ParseInt(fields[2], "weekly_buy.count");
            if (!count) {
                push_hint(count.Error(), 3.0f);
                return false;
            }
            (*weekly_buy_count)[fields[1]] = count.Value();
        } else if (tag == "weekly_sell" && fields.size() >= 3 && weekly_sell_count) {
            auto count = ParseInt(fields[2], "weekly_sell.count");
            if (!count) {
                push_hint(count.Error(), 3.0f);
                return false;
            }
            (*weekly_sell_count)[fields[1]] = count.Value();
        } else if (tag == "inventory" && fields.size() >= 3) {
            auto count = ParseInt(fields[2], "inventory.count");
            if (!count) {
                push_hint(count.Error(), 3.2f);
                return false;
            }
            inventory.AddItem(fields[1], count.Value());
        } else if (tag == "npc" && fields.size() >= 8) {
            for (auto& npc : npcs) {
                if (npc.id == fields[1]) {
                    auto pos_x = ParseFloat(fields[2], "npc.x");
                    auto pos_y = ParseFloat(fields[3], "npc.y");
                    auto favor = ParseInt(fields[4], "npc.favor");
                    auto gifted = ParseBool01(fields[5], "npc.daily_gifted");
                    if (!pos_x || !pos_y || !favor || !gifted) {
                        push_hint("NPC存档字段解析失败。", 3.2f);
                        return false;
                    }
                    npc.shape.setPosition({pos_x.Value(), pos_y.Value()});
                    npc.favor = favor.Value();
                    npc.daily_gifted = gifted.Value();
                    npc.current_location = fields[6];
                    npc.current_activity = fields[7];
                    npc.heart_level = NpcHeartLevelFromFavor(npc.favor);
                    npc.daily_favor_gain = 0;
                    npc.last_gift_day = gifted.Value() ? clock.Day() : (clock.Day() - 1);
                    if (fields.size() >= 11) {
                        auto heart_level = ParseInt(fields[8], "npc.heart_level");
                        auto daily_gain = ParseInt(fields[9], "npc.daily_favor_gain");
                        auto last_day = ParseInt(fields[10], "npc.last_gift_day");
                        if (!heart_level || !daily_gain || !last_day) {
                            push_hint("NPC扩展存档字段解析失败。", 3.2f);
                            return false;
                        }
                        npc.heart_level = heart_level.Value();
                        npc.daily_favor_gain = daily_gain.Value();
                        npc.last_gift_day = last_day.Value();
                    }
                    if (fields.size() >= 13) {
                        auto talked = ParseBool01(fields[11], "npc.daily_talked");
                        auto last_talk = ParseInt(fields[12], "npc.last_talk_day");
                        if (talked && last_talk) {
                            npc.daily_talked = talked.Value();
                            npc.last_talk_day = last_talk.Value();
                        }
                    }
                    break;
                }
            }
        } else if (tag == "skills" && fields.size() >= 2 && skills) {
            skills->LoadState(fields[1]);
        } else if (tag == "festival" && fields.size() >= 3 && festivals) {
            auto participated = ParseBool01(fields[2], "festival.participated");
            if (!participated) {
                push_hint(participated.Error(), 3.2f);
                return false;
            }
            festivals->SetParticipated(fields[1], participated.Value());
        } else if (tag == "dynamic_life" && fields.size() >= 4 && dynamic_life) {
            if (auto* state = dynamic_life->GetNpcState(fields[1])) {
                auto life_stage = ParseInt(fields[2], "dynamic_life.stage");
                auto points = ParseFloat(fields[3], "dynamic_life.points");
                if (!life_stage || !points) {
                    push_hint(!life_stage ? life_stage.Error() : points.Error(), 3.2f);
                    return false;
                }
                state->stage = static_cast<CloudSeamanor::domain::LifeStage>(life_stage.Value());
                state->progress_points = points.Value();
            }
        } else if (tag == "workshop_machine" && fields.size() >= 5 && workshop) {
            auto progress = ParseFloat(fields[3], "workshop.progress");
            auto processing = ParseBool01(fields[4], "workshop.is_processing");
            if (!progress || !processing) {
                push_hint(!progress ? progress.Error() : processing.Error(), 3.2f);
                return false;
            }
            workshop->SetMachineState(fields[1], fields[2], progress.Value(), processing.Value());
        }
    }

    update_highlighted_interactable();
    update_hud_text();

    // 加载心事件完成状态
    if (dialogue_manager) {
        dialogue_manager->LoadState(all_lines);
    }

    push_hint(
        "已从 save_slot_01 读取存档（版本 v" + std::to_string(save_version) + "）。",
        2.4f);
    return true;
}

} // namespace CloudSeamanor::engine
