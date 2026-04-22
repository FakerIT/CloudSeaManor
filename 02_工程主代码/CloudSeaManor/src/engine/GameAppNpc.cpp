#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/GameAppNpc.hpp"

#include <SFML/Graphics/Color.hpp>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>
#include <unordered_map>

namespace CloudSeamanor::engine {

int ParseTimeToMinutes(const std::string& time_text);
std::vector<std::string> SplitCsvLine(const std::string& line);
NpcGiftPrefs ParseGiftPrefsForNpc(const std::string& json_text, const std::string& npc_id);
std::string NpcDisplayName(const NpcTextMappings& npc_text_mappings, const std::string& npc_id);

namespace {

void SeedCoreNpcs(const NpcTextMappings& npc_text_mappings,
                  std::unordered_map<std::string, NpcActor>& npc_map) {
    npc_map["lin"].id = "lin";
    npc_map["lin"].display_name = NpcDisplayName(npc_text_mappings, "lin");
    npc_map["lin"].shape.setSize({28.0f, 40.0f});
    npc_map["lin"].shape.setFillColor(sf::Color(220, 170, 200));
    npc_map["lin"].shape.setOutlineThickness(2.0f);
    npc_map["lin"].base_outline = sf::Color(100, 64, 88);
    npc_map["lin"].shape.setOutlineColor(npc_map["lin"].base_outline);
    npc_map["lin"].shape.setPosition({420.0f, 320.0f});

    npc_map["acha"].id = "acha";
    npc_map["acha"].display_name = NpcDisplayName(npc_text_mappings, "acha");
    npc_map["acha"].shape.setSize({26.0f, 44.0f});
    npc_map["acha"].shape.setFillColor(sf::Color(180, 240, 200));
    npc_map["acha"].shape.setOutlineThickness(2.0f);
    npc_map["acha"].base_outline = sf::Color(80, 140, 100);
    npc_map["acha"].shape.setOutlineColor(npc_map["acha"].base_outline);
    npc_map["acha"].shape.setPosition({430.0f, 330.0f});

    npc_map["xiaoman"].id = "xiaoman";
    npc_map["xiaoman"].display_name = NpcDisplayName(npc_text_mappings, "xiaoman");
    npc_map["xiaoman"].shape.setSize({28.0f, 38.0f});
    npc_map["xiaoman"].shape.setFillColor(sf::Color(255, 210, 100));
    npc_map["xiaoman"].shape.setOutlineThickness(2.0f);
    npc_map["xiaoman"].base_outline = sf::Color(180, 140, 40);
    npc_map["xiaoman"].shape.setOutlineColor(npc_map["xiaoman"].base_outline);
    npc_map["xiaoman"].shape.setPosition({320.0f, 450.0f});

    npc_map["wanxing"].id = "wanxing";
    npc_map["wanxing"].display_name = NpcDisplayName(npc_text_mappings, "wanxing");
    npc_map["wanxing"].shape.setSize({24.0f, 46.0f});
    npc_map["wanxing"].shape.setFillColor(sf::Color(140, 120, 200));
    npc_map["wanxing"].shape.setOutlineThickness(2.0f);
    npc_map["wanxing"].base_outline = sf::Color(80, 60, 140);
    npc_map["wanxing"].shape.setOutlineColor(npc_map["wanxing"].base_outline);
    npc_map["wanxing"].shape.setPosition({180.0f, 160.0f});
}

void SeedExtraNpcs(const NpcTextMappings& npc_text_mappings,
                   std::unordered_map<std::string, NpcActor>& npc_map) {
    const std::vector<std::string> extra_ids{
        "song", "yu", "mo", "qiao", "he", "ning", "an", "shu", "yan"
    };
    for (std::size_t i = 0; i < extra_ids.size(); ++i) {
        NpcActor actor;
        actor.id = extra_ids[i];
        actor.display_name = NpcDisplayName(npc_text_mappings, extra_ids[i]);
        actor.shape.setSize({24.0f, 38.0f});
        actor.shape.setFillColor(sf::Color(
            static_cast<std::uint8_t>(120 + (i * 13) % 110),
            static_cast<std::uint8_t>(130 + (i * 17) % 100),
            static_cast<std::uint8_t>(140 + (i * 19) % 90)));
        actor.shape.setOutlineThickness(2.0f);
        actor.base_outline = sf::Color(72, 72, 88);
        actor.shape.setOutlineColor(actor.base_outline);
        actor.shape.setPosition({540.0f + static_cast<float>(i % 3) * 36.0f,
                                 250.0f + static_cast<float>(i / 3) * 44.0f});
        npc_map[extra_ids[i]] = actor;
    }
}

void ParseNpcSchedules(const std::string& schedule_text,
                       std::unordered_map<std::string, NpcActor>& npc_map) {
    std::istringstream schedule_lines(schedule_text);
    std::string line;
    bool is_header = true;
    while (std::getline(schedule_lines, line)) {
        if (line.empty()) {
            continue;
        }

        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        if (!trimmed.empty() && trimmed[0] == '#') {
            continue;
        }
        if (is_header) {
            is_header = false;
            continue;
        }

        const auto cells = SplitCsvLine(line);
        if (cells.size() < 5) {
            continue;
        }
        auto it = npc_map.find(cells[0]);
        if (it == npc_map.end()) {
            continue;
        }
        const auto dash = cells[2].find('-');
        if (dash == std::string::npos) {
            continue;
        }

        NpcScheduleEntry entry;
        entry.start_minutes = ParseTimeToMinutes(cells[2].substr(0, dash));
        entry.end_minutes = ParseTimeToMinutes(cells[2].substr(dash + 1));
        entry.location = cells[3];
        entry.activity = cells[4];
        it->second.schedule.push_back(entry);
    }
}

void FinalizeNpcs(const std::string& gift_text,
                  std::unordered_map<std::string, NpcActor>& npc_map,
                  std::vector<NpcActor>& npcs) {
    for (auto& [id, npc] : npc_map) {
        npc.prefs = ParseGiftPrefsForNpc(gift_text, id);
        if (npc.schedule.empty()) {
            npc.schedule.push_back({360, 720, "VillageCenter", "Trading"});
            npc.schedule.push_back({720, 1080, "Market", "Trading"});
            npc.schedule.push_back({1080, 1320, "MainHouse", "BreakfastAndChores"});
        }
        npcs.push_back(npc);
    }
}

} // namespace

int ParseTimeToMinutes(const std::string& time_text) {
    const auto colon = time_text.find(':');
    if (colon == std::string::npos) {
        return 0;
    }
    const int hour = std::stoi(time_text.substr(0, colon));
    const int minute = std::stoi(time_text.substr(colon + 1));
    return hour * 60 + minute;
}

std::vector<std::string> SplitCsvLine(const std::string& line) {
    std::vector<std::string> cells;
    std::string current;
    for (char ch : line) {
        if (ch == ',') {
            cells.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    cells.push_back(current);
    return cells;
}

sf::Vector2f AnchorForLocation(const std::string& location) {
    if (location == "TeaField")       return {430.0f, 340.0f};
    if (location == "MainHouse")      return {220.0f, 300.0f};
    if (location == "Market")         return {980.0f, 180.0f};
    if (location == "Dock")           return {980.0f, 520.0f};
    if (location == "Workshop")      return {540.0f, 330.0f};
    if (location == "VillageCenter")  return {620.0f, 300.0f};
    if (location == "SpiritGarden")  return {320.0f, 450.0f};
    if (location == "Observatory")   return {180.0f, 160.0f};
    return {620.0f, 300.0f};
}

std::vector<std::string> ParseJsonArray(const std::string& object_text, const std::string& key) {
    std::vector<std::string> result;
    const std::string pattern = "\"" + key + "\"";
    const auto key_pos = object_text.find(pattern);
    if (key_pos == std::string::npos) {
        return result;
    }
    const auto open = object_text.find('[', key_pos);
    const auto close = object_text.find(']', open);
    if (open == std::string::npos || close == std::string::npos) {
        return result;
    }

    std::string item;
    bool in_string = false;
    for (std::size_t i = open + 1; i < close; ++i) {
        const char ch = object_text[i];
        if (ch == '"') {
            if (in_string) {
                result.push_back(item);
                item.clear();
            }
            in_string = !in_string;
        } else if (in_string) {
            item.push_back(ch);
        }
    }
    return result;
}

NpcGiftPrefs ParseGiftPrefsForNpc(const std::string& json_text, const std::string& npc_id) {
    NpcGiftPrefs prefs;
    const std::string pattern = "\"" + npc_id + "\"";
    const auto id_pos = json_text.find(pattern);
    if (id_pos == std::string::npos) {
        return prefs;
    }
    const auto open = json_text.find('{', id_pos);
    const auto close = json_text.find('}', open);
    if (open == std::string::npos || close == std::string::npos) {
        return prefs;
    }

    const std::string object_text = json_text.substr(open, close - open + 1);
    prefs.loved = ParseJsonArray(object_text, "loved");
    prefs.liked = ParseJsonArray(object_text, "liked");
    prefs.disliked = ParseJsonArray(object_text, "disliked");
    return prefs;
}

std::unordered_map<std::string, std::string> ParseJsonObjectMap(const std::string& json_text, const std::string& key) {
    std::unordered_map<std::string, std::string> result;
    const std::string pattern = "\"" + key + "\"";
    const auto key_pos = json_text.find(pattern);
    if (key_pos == std::string::npos) {
        return result;
    }
    const auto open = json_text.find('{', key_pos);
    const auto close = json_text.find('}', open);
    if (open == std::string::npos || close == std::string::npos) {
        return result;
    }

    std::string current_key;
    std::string current_value;
    bool reading_key = true;
    bool in_string = false;
    for (std::size_t i = open + 1; i < close; ++i) {
        const char ch = json_text[i];
        if (ch == '"') {
            in_string = !in_string;
            if (!in_string && !current_key.empty() && !current_value.empty()) {
                result[current_key] = current_value;
                current_key.clear();
                current_value.clear();
                reading_key = true;
            }
            continue;
        }
        if (!in_string) {
            if (ch == ':') {
                reading_key = false;
            }
            continue;
        }
        if (reading_key) {
            current_key.push_back(ch);
        } else {
            current_value.push_back(ch);
        }
    }
    return result;
}

bool LoadNpcTextMappings(const std::string& path, NpcTextMappings& npc_text_mappings) {
    std::ifstream mapping_stream(path);
    if (!mapping_stream.is_open()) {
        npc_text_mappings = {};
        return false;
    }

    const std::string mapping_text((std::istreambuf_iterator<char>(mapping_stream)), std::istreambuf_iterator<char>());
    npc_text_mappings.names = ParseJsonObjectMap(mapping_text, "names");
    npc_text_mappings.locations = ParseJsonObjectMap(mapping_text, "locations");
    npc_text_mappings.activities = ParseJsonObjectMap(mapping_text, "activities");
    return true;
}

std::string NpcDisplayName(const NpcTextMappings& npc_text_mappings, const std::string& npc_id) {
    const auto it = npc_text_mappings.names.find(npc_id);
    return it != npc_text_mappings.names.end() ? it->second : npc_id;
}

std::string LocationDisplayName(const NpcTextMappings& npc_text_mappings, const std::string& location) {
    const auto it = npc_text_mappings.locations.find(location);
    return it != npc_text_mappings.locations.end() ? it->second : location;
}

std::string ActivityDisplayName(const NpcTextMappings& npc_text_mappings, const std::string& activity) {
    const auto it = npc_text_mappings.activities.find(activity);
    return it != npc_text_mappings.activities.end() ? it->second : activity;
}

bool ContainsItem(const std::vector<std::string>& items, const std::string& item) {
    return std::find(items.begin(), items.end(), item) != items.end();
}

void BuildNpcs(const std::string& schedule_path,
               const std::string& gift_path,
               const NpcTextMappings& npc_text_mappings,
               std::vector<NpcActor>& npcs) {
    npcs.clear();
    npcs.reserve(13);

    std::ifstream schedule_stream(schedule_path);
    const std::string schedule_text((std::istreambuf_iterator<char>(schedule_stream)), std::istreambuf_iterator<char>());

    std::ifstream gift_stream(gift_path);
    const std::string gift_text((std::istreambuf_iterator<char>(gift_stream)), std::istreambuf_iterator<char>());

    std::unordered_map<std::string, NpcActor> npc_map;
    SeedCoreNpcs(npc_text_mappings, npc_map);
    SeedExtraNpcs(npc_text_mappings, npc_map);
    ParseNpcSchedules(schedule_text, npc_map);
    FinalizeNpcs(gift_text, npc_map, npcs);
}

} // namespace CloudSeamanor::engine
