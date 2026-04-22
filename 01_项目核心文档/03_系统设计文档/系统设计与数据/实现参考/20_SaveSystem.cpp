#include "SaveSystem.hpp"

#include <fstream>
#include <filesystem>

using nlohmann::json;
namespace fs = std::filesystem;

static void to_json(json& j, const GameTime& v) {
    j = json{{"year", v.year}, {"season", v.season}, {"day", v.day}, {"minute", v.minute}};
}

static void from_json(const json& j, GameTime& v) {
    v.year = j.value("year", 1);
    v.season = j.value("season", 0);
    v.day = j.value("day", 1);
    v.minute = j.value("minute", 360);
}

static void to_json(json& j, const GlobalData& v) {
    j = json{{"money", v.money}, {"aura", v.aura}, {"weather_state", v.weather_state}, {"time", v.time}};
}

static void from_json(const json& j, GlobalData& v) {
    v.money = j.value("money", 0);
    v.aura = j.value("aura", 0.0f);
    v.weather_state = j.value("weather_state", 0);
    v.time = j.value("time", GameTime{});
}

static void to_json(json& j, const PersistentEntity& v) {
    j = json{{"entity_id", v.entity_id}, {"prefab_id", v.prefab_id}, {"tile_x", v.tile_x}, {"tile_y", v.tile_y}, {"state", v.state}};
}

static void from_json(const json& j, PersistentEntity& v) {
    v.entity_id = j.value("entity_id", 0);
    v.prefab_id = j.value("prefab_id", 0);
    v.tile_x = j.value("tile_x", 0);
    v.tile_y = j.value("tile_y", 0);
    v.state = j.value("state", 0);
}

static void to_json(json& j, const NpcSocialData& v) {
    j = json{{"npc_id", v.npc_id}, {"heart_level", v.heart_level}, {"affection", v.affection}, {"talked_today", v.talked_today}, {"gifted_today", v.gifted_today}};
}

static void from_json(const json& j, NpcSocialData& v) {
    v.npc_id = j.value("npc_id", "");
    v.heart_level = j.value("heart_level", 0);
    v.affection = j.value("affection", 0);
    v.talked_today = j.value("talked_today", false);
    v.gifted_today = j.value("gifted_today", false);
}

SaveSystem::SaveSystem(std::string save_dir) : save_dir_(std::move(save_dir)) {
    fs::create_directories(save_dir_);
}

bool SaveSystem::SaveSync(const SaveData& data, const std::string& slot_name) {
    json j;
    j["version"] = data.version;
    j["global"] = data.global;
    j["entities"] = data.entities;
    j["socials"] = data.socials;
    return WriteAtomically(j, slot_name);
}

std::future<bool> SaveSystem::SaveAsync(SaveData data, std::string slot_name) {
    return std::async(std::launch::async, [this, data = std::move(data), slot_name = std::move(slot_name)]() {
        return SaveSync(data, slot_name);
    });
}

std::optional<SaveData> SaveSystem::Load(const std::string& slot_name) {
    auto j_opt = ReadJson(slot_name);
    if (!j_opt.has_value()) {
        return std::nullopt;
    }
    SaveData migrated = MigrateIfNeeded(*j_opt);
    return migrated;
}

bool SaveSystem::WriteAtomically(const json& j, const std::string& slot_name) {
    const fs::path final_path = fs::path(save_dir_) / (slot_name + ".json");
    const fs::path tmp_path = fs::path(save_dir_) / (slot_name + "_tmp.json");
    const fs::path backup_path = fs::path(save_dir_) / (slot_name + "_backup.json");

    {
        std::ofstream ofs(tmp_path, std::ios::binary);
        if (!ofs) return false;
        ofs << j.dump(2);
    }

    if (fs::exists(final_path)) {
        std::error_code ec;
        fs::rename(final_path, backup_path, ec);
        if (ec) {
            fs::copy_file(final_path, backup_path, fs::copy_options::overwrite_existing, ec);
            if (ec) return false;
            fs::remove(final_path, ec);
        }
    }

    std::error_code ec2;
    fs::rename(tmp_path, final_path, ec2);
    if (ec2) {
        return false;
    }
    return true;
}

std::optional<json> SaveSystem::ReadJson(const std::string& slot_name) {
    const fs::path final_path = fs::path(save_dir_) / (slot_name + ".json");
    const fs::path backup_path = fs::path(save_dir_) / (slot_name + "_backup.json");

    auto try_read = [](const fs::path& p) -> std::optional<json> {
        std::ifstream ifs(p, std::ios::binary);
        if (!ifs) return std::nullopt;
        try {
            json j;
            ifs >> j;
            return j;
        } catch (...) {
            return std::nullopt;
        }
    };

    if (auto j = try_read(final_path); j.has_value()) return j;
    if (auto j = try_read(backup_path); j.has_value()) return j;
    return std::nullopt;
}

SaveData SaveSystem::MigrateIfNeeded(const json& j) {
    SaveData out;

    const std::string version = j.value("version", "1.0.0");
    out.version = "1.1.0";

    if (j.contains("global")) out.global = j["global"].get<GlobalData>();
    if (j.contains("entities")) out.entities = j["entities"].get<std::vector<PersistentEntity>>();
    if (j.contains("socials")) out.socials = j["socials"].get<std::vector<NpcSocialData>>();

    if (version == "1.0.0") {
        for (auto& s : out.socials) {
            if (s.npc_id.empty()) {
                s.npc_id = "unknown_npc";
            }
        }
    }

    return out;
}
