// ============================================================================
// 【AtmosphereDataLoader.cpp】大气数据加载器实现
// ============================================================================

#include "CloudSeamanor/infrastructure/AtmosphereDataLoader.hpp"

#include "CloudSeamanor/Logger.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace CloudSeamanor::infrastructure {

namespace {

[[nodiscard]] std::string Trim(const std::string& s) {
    auto start = std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    });
    auto end = std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base();
    return (start < end) ? std::string(start, end) : std::string();
}

[[nodiscard]] std::vector<std::string> Split(const std::string& line, char delim) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    while (std::getline(iss, token, delim)) {
        tokens.push_back(Trim(token));
    }
    return tokens;
}

bool ParseHexColor(const std::string& hex, int& out_r, int& out_g, int& out_b, int& out_a) {
    std::string h = hex;
    if (!h.empty() && h[0] == '#') {
        h = h.substr(1);
    }
    if (h.length() == 6) {
        try {
            out_r = std::stoi(h.substr(0, 2), nullptr, 16);
            out_g = std::stoi(h.substr(2, 2), nullptr, 16);
            out_b = std::stoi(h.substr(4, 2), nullptr, 16);
            out_a = 255;
            return true;
        } catch (...) {
            return false;
        }
    }
    if (h.length() == 8) {
        try {
            out_r = std::stoi(h.substr(0, 2), nullptr, 16);
            out_g = std::stoi(h.substr(2, 2), nullptr, 16);
            out_b = std::stoi(h.substr(4, 2), nullptr, 16);
            out_a = std::stoi(h.substr(6, 2), nullptr, 16);
            return true;
        } catch (...) {
            return false;
        }
    }
    return false;
}

}  // namespace

bool AtmosphereDataLoader::LoadSkyProfiles(
    const std::filesystem::path& file_path,
    std::vector<domain::SkyProfile>& out_profiles
) const {
    out_profiles.clear();
    std::ifstream file(file_path);
    if (!file.is_open()) {
        last_error_ = "无法打开文件: " + file_path.string();
        return false;
    }

    std::string line;
    std::getline(file, line);

    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const auto tokens = Split(line, ',');
        if (tokens.size() < 12) {
            continue;
        }

        domain::SkyProfile profile;
        int col = 0;
        profile.id = tokens[col++];
        profile.cloud_state = tokens[col++];
        profile.day_phase = tokens[col++];

        int r = 0, g = 0, b = 0, a = 0;

        // sky_top_color (hex)
        if (ParseHexColor(tokens[col++], r, g, b, a)) {
            profile.sky_top_r = r; profile.sky_top_g = g;
            profile.sky_top_b = b; profile.sky_top_a = a;
        }

        // sky_bottom_color (hex)
        if (ParseHexColor(tokens[col++], r, g, b, a)) {
            profile.sky_bottom_r = r; profile.sky_bottom_g = g;
            profile.sky_bottom_b = b; profile.sky_bottom_a = a;
        }

        // horizon_color (hex)
        if (ParseHexColor(tokens[col++], r, g, b, a)) {
            profile.horizon_r = r; profile.horizon_g = g;
            profile.horizon_b = b; profile.horizon_a = a;
        }

        // sun_color (hex)
        if (ParseHexColor(tokens[col++], r, g, b, a)) {
            profile.sun_r = r; profile.sun_g = g;
            profile.sun_b = b; profile.sun_a = a;
        }

        // ambient_color (hex)
        if (ParseHexColor(tokens[col++], r, g, b, a)) {
            profile.ambient_r = r; profile.ambient_g = g;
            profile.ambient_b = b; profile.ambient_a = a;
        }

        auto safe_float = [&](const std::string& s, float def) -> float {
            try { return std::stof(s); } catch (...) { return def; }
        };
        auto safe_int = [&](const std::string& s, int def) -> int {
            try { return std::stoi(s); } catch (...) { return def; }
        };

        profile.fog_density = safe_float(tokens[col++], 0.0f);
        profile.atmosphere_intensity = safe_float(tokens[col++], 1.0f);
        profile.particle_density = safe_float(tokens[col++], 0.0f);
        profile.sort_order = safe_int(tokens[col++], 0);

        out_profiles.push_back(profile);
    }

    file.close();
    return !out_profiles.empty();
}

bool AtmosphereDataLoader::LoadWeatherProfiles(
    const std::filesystem::path& file_path,
    std::vector<domain::WeatherProfile>& out_profiles
) const {
    out_profiles.clear();

    std::ifstream file(file_path);
    if (!file.is_open()) {
        last_error_ = "无法打开文件: " + file_path.string();
        return false;
    }

    std::ostringstream content;
    content << file.rdbuf();
    const std::string json_str = content.str();
    file.close();

    const char* ptr = json_str.c_str();
    const char* end = ptr + json_str.length();

    const std::vector<std::string> states = {"clear", "mist", "dense", "tide"};
    for (const std::string& state : states) {
        const std::string key_marker = "\"" + state + "\"";
        auto key_pos = json_str.find(key_marker);
        if (key_pos == std::string::npos) {
            continue;
        }

        const char* obj_start = json_str.c_str() + json_str.find('{', key_pos);
        const char* obj_end = obj_start;
        int brace_depth = 0;
        bool in_string = false;
        while (obj_end < end) {
            if (*obj_end == '"' && (obj_end == obj_start || *(obj_end - 1) != '\\')) {
                in_string = !in_string;
            }
            if (!in_string) {
                if (*obj_end == '{') ++brace_depth;
                else if (*obj_end == '}') {
                    --brace_depth;
                    if (brace_depth == 0) {
                        ++obj_end;
                        break;
                    }
                }
            }
            ++obj_end;
        }

        std::string obj_str(obj_start, obj_end - obj_start);

        domain::WeatherProfile profile;
        profile.id = state;
        profile.cloud_state = state;

        auto get_int = [&](const std::string& field, int def) -> int {
            const std::string f = "\"" + field + "\"";
            auto pos = obj_str.find(f);
            if (pos == std::string::npos) return def;
            auto colon = obj_str.find(':', pos);
            if (colon == std::string::npos) return def;
            auto start = colon + 1;
            while (start < obj_str.size() && std::isspace(static_cast<unsigned char>(obj_str[start]))) ++start;
            auto num_end = start;
            while (num_end < obj_str.size() && (std::isdigit(static_cast<unsigned char>(obj_str[num_end])) || obj_str[num_end] == '.')) ++num_end;
            try { return std::stoi(obj_str.substr(start, num_end - start)); } catch (...) { return def; }
        };

        auto get_float = [&](const std::string& field, float def) -> float {
            const std::string f = "\"" + field + "\"";
            auto pos = obj_str.find(f);
            if (pos == std::string::npos) return def;
            auto colon = obj_str.find(':', pos);
            if (colon == std::string::npos) return def;
            auto start = colon + 1;
            while (start < obj_str.size() && std::isspace(static_cast<unsigned char>(obj_str[start]))) ++start;
            auto num_end = start;
            while (num_end < obj_str.size() && (std::isdigit(static_cast<unsigned char>(obj_str[num_end])) || obj_str[num_end] == '.' || obj_str[num_end] == '-')) ++num_end;
            try { return std::stof(obj_str.substr(start, num_end - start)); } catch (...) { return def; }
        };

        auto get_bool = [&](const std::string& field, bool def) -> bool {
            const std::string f = "\"" + field + "\"";
            auto pos = obj_str.find(f);
            if (pos == std::string::npos) return def;
            auto colon = obj_str.find(':', pos);
            if (colon == std::string::npos) return def;
            auto start = colon + 1;
            while (start < obj_str.size() && std::isspace(static_cast<unsigned char>(obj_str[start]))) ++start;
            return obj_str.compare(start, 4, "true") == 0;
        };

        auto get_color = [&](const std::string& field, int& r, int& g, int& b, int& a) -> bool {
            const std::string f = "\"" + field + "\"";
            auto pos = obj_str.find(f);
            if (pos == std::string::npos) return false;
            auto colon = obj_str.find(':', pos);
            if (colon == std::string::npos) return false;
            auto start = obj_str.find('"', colon);
            if (start == std::string::npos) return false;
            auto end = obj_str.find('"', start + 1);
            if (end == std::string::npos) return false;
            return ParseHexColor(obj_str.substr(start + 1, end - start - 1), r, g, b, a);
        };

        profile.max_particles = get_int("max_particles", 0);
        profile.particle_speed = get_float("particle_speed", 30.0f);
        profile.particle_size_min = get_float("particle_size_min", 2.0f);
        profile.particle_size_max = get_float("particle_size_max", 8.0f);
        profile.particle_lifetime_min = get_float("particle_lifetime_min", 2.0f);
        profile.particle_lifetime_max = get_float("particle_lifetime_max", 5.0f);
        profile.particle_spawn_rate = get_float("particle_spawn_rate", 30.0f);
        profile.follows_camera = get_bool("follows_camera", true);

        int r = 255, g = 255, b = 255, a = 200;
        if (get_color("particle_color_start", r, g, b, a)) {
            profile.color_start_r = r; profile.color_start_g = g;
            profile.color_start_b = b; profile.color_start_a = a;
        }
        if (get_color("particle_color_end", r, g, b, a)) {
            profile.color_end_r = r; profile.color_end_g = g;
            profile.color_end_b = b; profile.color_end_a = a;
        }

        out_profiles.push_back(profile);
    }

    return !out_profiles.empty();
}

bool AtmosphereDataLoader::LoadAll(
    const std::filesystem::path& data_dir,
    std::vector<domain::SkyProfile>& out_skies,
    std::vector<domain::WeatherProfile>& out_weathers
) const {
    const auto sky_path = data_dir / "atmosphere_profiles.csv";
    const auto weather_path = data_dir / "weather_effects.json";

    bool sky_ok = LoadSkyProfiles(sky_path, out_skies);
    bool weather_ok = LoadWeatherProfiles(weather_path, out_weathers);

    return sky_ok || weather_ok;
}

bool AtmosphereDataLoader::LoadWithDefaults(
    const std::filesystem::path& data_dir,
    std::vector<domain::SkyProfile>& out_skies,
    std::vector<domain::WeatherProfile>& out_weathers
) const {
    const auto sky_path = data_dir / "atmosphere_profiles.csv";
    const auto weather_path = data_dir / "weather_effects.json";

    const bool sky_ok = LoadSkyProfiles(sky_path, out_skies);
    const bool weather_ok = LoadWeatherProfiles(weather_path, out_weathers);

    if (!out_skies.empty() && !out_weathers.empty()) {
        return true;
    }

    // 填充默认值
    if (out_skies.empty()) {
        domain::SkyProfile default_sky;
        default_sky.id = "default_clear_morning";
        default_sky.cloud_state = "clear";
        default_sky.day_phase = "morning";
        default_sky.sky_top_r = 135; default_sky.sky_top_g = 206; default_sky.sky_top_b = 235; default_sky.sky_top_a = 255;
        default_sky.sky_bottom_r = 240; default_sky.sky_bottom_g = 248; default_sky.sky_bottom_b = 255; default_sky.sky_bottom_a = 255;
        default_sky.horizon_r = 255; default_sky.horizon_g = 255; default_sky.horizon_b = 255; default_sky.horizon_a = 200;
        default_sky.sun_r = 255; default_sky.sun_g = 255; default_sky.sun_b = 200; default_sky.sun_a = 100;
        default_sky.fog_density = 0.02f;
        default_sky.atmosphere_intensity = 0.8f;
        default_sky.particle_density = 0.0f;
        out_skies.push_back(default_sky);
        Logger::Warning("AtmosphereDataLoader: 使用默认天空配置。");
    }

    if (out_weathers.empty()) {
        const std::vector<std::string> states = {"clear", "mist", "dense", "tide"};
        for (const std::string& s : states) {
            domain::WeatherProfile wp;
            wp.id = s;
            wp.cloud_state = s;
            wp.max_particles = (s == "clear") ? 0 : (s == "mist") ? 120 : (s == "dense") ? 180 : 200;
            wp.particle_speed = (s == "mist") ? 15.0f : (s == "dense") ? 25.0f : (s == "tide") ? 40.0f : 30.0f;
            wp.particle_spawn_rate = static_cast<float>(wp.max_particles) * 0.3f;
            out_weathers.push_back(wp);
        }
        Logger::Warning("AtmosphereDataLoader: 使用默认天气配置。");
    }

    return sky_ok || weather_ok;
}

}  // namespace CloudSeamanor::infrastructure
