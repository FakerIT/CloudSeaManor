#pragma once

#include <string>
#include <vector>
#include <optional>
#include <future>
#include <nlohmann/json.hpp>

struct GameTime {
    int year = 1;
    int season = 0;
    int day = 1;
    int minute = 360;
};

struct GlobalData {
    int money = 0;
    float aura = 0.0f;
    int weather_state = 0;
    GameTime time;
};

struct PersistentEntity {
    int entity_id = 0;
    int prefab_id = 0;
    int tile_x = 0;
    int tile_y = 0;
    int state = 0;
};

struct NpcSocialData {
    std::string npc_id;
    int heart_level = 0;
    int affection = 0;
    bool talked_today = false;
    bool gifted_today = false;
};

struct SaveData {
    std::string version = "1.0.0";
    GlobalData global;
    std::vector<PersistentEntity> entities;
    std::vector<NpcSocialData> socials;
};

class SaveSystem {
public:
    explicit SaveSystem(std::string save_dir);

    bool SaveSync(const SaveData& data, const std::string& slot_name);
    std::future<bool> SaveAsync(SaveData data, std::string slot_name);

    std::optional<SaveData> Load(const std::string& slot_name);

private:
    std::string save_dir_;

    bool WriteAtomically(const nlohmann::json& j, const std::string& slot_name);
    std::optional<nlohmann::json> ReadJson(const std::string& slot_name);

    SaveData MigrateIfNeeded(const nlohmann::json& j);
};
