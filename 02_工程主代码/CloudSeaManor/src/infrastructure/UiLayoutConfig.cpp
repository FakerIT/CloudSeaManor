#include "CloudSeamanor/UiLayoutConfig.hpp"

#include "CloudSeamanor/JsonValue.hpp"
#include "CloudSeamanor/Logger.hpp"

#include <exception>
#include <fstream>
#include <sstream>

namespace CloudSeamanor::infrastructure {

namespace {

std::array<float, 2> ParseVec2(const JsonValue& value, const std::array<float, 2>& fallback) {
    if (!value.IsArray() || value.Size() < 2) {
        return fallback;
    }
    return {
        static_cast<float>(value[0].AsFloatOrDefault(fallback[0])),
        static_cast<float>(value[1].AsFloatOrDefault(fallback[1]))
    };
}

PanelLayout ParsePanelLayout(const JsonValue& obj, const PanelLayout& fallback) {
    if (!obj.IsObject()) {
        return fallback;
    }

    PanelLayout out = fallback;
    if (const auto* v = obj.Get("position")) out.position = ParseVec2(*v, fallback.position);
    if (const auto* v = obj.Get("size")) out.size = ParseVec2(*v, fallback.size);
    if (const auto* v = obj.Get("fill_color"); v && v->IsString()) out.fill_color = ParseHexColor(v->AsString());
    if (const auto* v = obj.Get("outline_color"); v && v->IsString()) out.outline_color = ParseHexColor(v->AsString());
    if (const auto* v = obj.Get("outline_thickness")) out.outline_thickness = static_cast<float>(v->AsFloatOrDefault(fallback.outline_thickness));
    if (const auto* v = obj.Get("alpha")) out.alpha = static_cast<float>(v->AsFloatOrDefault(fallback.alpha));
    return out;
}

TextStyleLayout ParseTextLayout(const JsonValue& obj, const TextStyleLayout& fallback) {
    if (!obj.IsObject()) {
        return fallback;
    }

    TextStyleLayout out = fallback;
    if (const auto* v = obj.Get("position")) out.position = ParseVec2(*v, fallback.position);
    if (const auto* v = obj.Get("font_size")) out.font_size = static_cast<unsigned int>(v->AsIntOrDefault(fallback.font_size));
    if (const auto* v = obj.Get("color"); v && v->IsString()) out.color = ParseHexColor(v->AsString());
    if (const auto* v = obj.Get("outline_thickness")) out.outline_thickness = static_cast<float>(v->AsFloatOrDefault(fallback.outline_thickness));
    if (const auto* v = obj.Get("outline_color"); v && v->IsString()) out.outline_color = ParseHexColor(v->AsString());
    return out;
}

CloudColorSet ParseCloudColor(const JsonValue& obj, const CloudColorSet& fallback) {
    if (!obj.IsObject()) {
        return fallback;
    }

    CloudColorSet out = fallback;
    if (const auto* v = obj.Get("background"); v && v->IsString()) out.background = ParseHexColor(v->AsString());
    if (const auto* v = obj.Get("aura")) {
        if (v->IsString()) {
            const std::string& aura = v->AsString();
            out.aura = (aura == "transparent") ? 0x00000000 : ParseHexColor(aura);
        }
    }
    return out;
}

void ParseSemanticColorIfExists(
    const JsonValue& colors,
    UiLayoutData& data,
    const std::string& key,
    std::uint32_t fallback) {
    if (const auto* v = colors.Get(key); v && v->IsString()) {
        data.semantic_colors[key] = ParseHexColor(v->AsString());
        return;
    }
    data.semantic_colors[key] = fallback;
}

void ParseSemanticNumberIfExists(
    const JsonValue& numbers,
    UiLayoutData& data,
    const std::string& key,
    float fallback) {
    if (const auto* v = numbers.Get(key)) {
        data.semantic_numbers[key] = static_cast<float>(v->AsFloatOrDefault(fallback));
        return;
    }
    data.semantic_numbers[key] = fallback;
}

}  // namespace

std::uint32_t ParseHexColor(const std::string& hex) {
    if (hex.empty() || hex[0] != '#') {
        return 0x000000FF;
    }

    // #RRGGBB or #RRGGBBAA
    if (hex.size() != 7 && hex.size() != 9) {
        return 0x000000FF;
    }

    try {
        auto parse_byte = [&](std::size_t offset) -> std::uint32_t {
            return static_cast<std::uint32_t>(std::stoul(hex.substr(offset, 2), nullptr, 16));
        };

        const std::uint32_t r = parse_byte(1);
        const std::uint32_t g = parse_byte(3);
        const std::uint32_t b = parse_byte(5);
        const std::uint32_t a = (hex.size() == 9) ? parse_byte(7) : 0xFF;
        return (r << 24) | (g << 16) | (b << 8) | a;
    } catch (const std::invalid_argument&) {
        return 0x000000FF;
    } catch (const std::out_of_range&) {
        return 0x000000FF;
    } catch (const std::exception&) {
        return 0x000000FF;
    }
}

bool UiLayoutConfig::LoadFromFile(const std::string& path) {
    data_ = GetDefaults();
    loaded_ = false;

    std::ifstream in(path);
    if (!in.is_open()) {
        Logger::Warning("UiLayoutConfig: 配置文件不存在，使用默认 UI 布局。");
        return false;
    }

    std::stringstream buffer;
    buffer << in.rdbuf();
    const JsonValue root = JsonValue::Parse(buffer.str());
    if (!root.IsObject()) {
        Logger::Warning("UiLayoutConfig: JSON 根对象无效，使用默认 UI 布局。");
        return false;
    }

    if (const auto* panels = root.Get("panels"); panels && panels->IsObject()) {
        for (const auto& [name, fallback] : GetDefaults().panels) {
            data_.panels[name] = ParsePanelLayout((*panels)[name], fallback);
        }

        // 特定字段：进度填充色
        if (const auto* stamina = panels->Get("stamina_bar"); stamina && stamina->IsObject()) {
            if (const auto* fill = stamina->Get("fill_color_stamina"); fill && fill->IsString()) {
                data_.panels["stamina_fill"].fill_color = ParseHexColor(fill->AsString());
            }
        }
        if (const auto* workshop = panels->Get("workshop_progress"); workshop && workshop->IsObject()) {
            if (const auto* fill = workshop->Get("fill_color_progress"); fill && fill->IsString()) {
                data_.panels["workshop_fill"].fill_color = ParseHexColor(fill->AsString());
            }
        }
    }

    if (const auto* texts = root.Get("texts"); texts && texts->IsObject()) {
        for (const auto& [name, fallback] : GetDefaults().texts) {
            data_.texts[name] = ParseTextLayout((*texts)[name], fallback);
        }
    }

    if (const auto* colors = root.Get("colors"); colors && colors->IsObject()) {
        for (const auto& [name, fallback] : GetDefaults().cloud_colors) {
            data_.cloud_colors[name] = ParseCloudColor((*colors)[name], fallback);
        }
        ParseSemanticColorIfExists(*colors, data_, "button_default_text", GetDefaults().semantic_colors["button_default_text"]);
        ParseSemanticColorIfExists(*colors, data_, "button_hover_text", GetDefaults().semantic_colors["button_hover_text"]);
        ParseSemanticColorIfExists(*colors, data_, "button_selected_text", GetDefaults().semantic_colors["button_selected_text"]);
        ParseSemanticColorIfExists(*colors, data_, "button_disabled_text", GetDefaults().semantic_colors["button_disabled_text"]);
        ParseSemanticColorIfExists(*colors, data_, "button_default_bg", GetDefaults().semantic_colors["button_default_bg"]);
        ParseSemanticColorIfExists(*colors, data_, "button_hover_bg", GetDefaults().semantic_colors["button_hover_bg"]);
        ParseSemanticColorIfExists(*colors, data_, "button_selected_bg", GetDefaults().semantic_colors["button_selected_bg"]);
        ParseSemanticColorIfExists(*colors, data_, "button_disabled_bg", GetDefaults().semantic_colors["button_disabled_bg"]);
        ParseSemanticColorIfExists(*colors, data_, "main_menu_item_normal", GetDefaults().semantic_colors["main_menu_item_normal"]);
        ParseSemanticColorIfExists(*colors, data_, "main_menu_item_selected", GetDefaults().semantic_colors["main_menu_item_selected"]);
    }

    if (const auto* numbers = root.Get("numbers"); numbers && numbers->IsObject()) {
        ParseSemanticNumberIfExists(*numbers, data_, "world_tip_wave_amplitude", GetDefaults().semantic_numbers["world_tip_wave_amplitude"]);
        ParseSemanticNumberIfExists(*numbers, data_, "world_tip_wave_period", GetDefaults().semantic_numbers["world_tip_wave_period"]);
    }

    if (const auto* fonts = root.Get("fonts"); fonts && fonts->IsObject()) {
        if (const auto* primary = fonts->Get("primary"); primary && primary->IsString()) {
            data_.primary_font = primary->AsString();
        }
        if (const auto* fallback = fonts->Get("fallback"); fallback && fallback->IsString()) {
            data_.fallback_font = fallback->AsString();
        }
    }

    loaded_ = true;
    Logger::Info("UiLayoutConfig: 成功加载 UI 布局配置。");
    return true;
}

PanelLayout UiLayoutConfig::GetPanel(const std::string& name) const {
    const auto it = data_.panels.find(name);
    return it != data_.panels.end() ? it->second : PanelLayout{};
}

TextStyleLayout UiLayoutConfig::GetText(const std::string& name) const {
    const auto it = data_.texts.find(name);
    return it != data_.texts.end() ? it->second : TextStyleLayout{};
}

CloudColorSet UiLayoutConfig::GetCloudColor(const std::string& name) const {
    const auto it = data_.cloud_colors.find(name);
    return it != data_.cloud_colors.end() ? it->second : CloudColorSet{};
}

std::uint32_t UiLayoutConfig::GetSemanticColor(
    const std::string& name,
    std::uint32_t fallback) const {
    const auto it = data_.semantic_colors.find(name);
    return it != data_.semantic_colors.end() ? it->second : fallback;
}

float UiLayoutConfig::GetSemanticNumber(
    const std::string& name,
    float fallback) const {
    const auto it = data_.semantic_numbers.find(name);
    return it != data_.semantic_numbers.end() ? it->second : fallback;
}

UiLayoutData UiLayoutConfig::GetDefaults() {
    UiLayoutData d;

    d.panels["main_panel"] = {{824.0f, 18.0f}, {430.0f, 178.0f}, ParseHexColor("#0A0E18E6"), ParseHexColor("#5F92C6"), 2.0f, 230.0f};
    d.panels["inventory_panel"] = {{824.0f, 208.0f}, {430.0f, 172.0f}, ParseHexColor("#101824E4"), ParseHexColor("#709CB8"), 2.0f, 228.0f};
    d.panels["dialogue_panel"] = {{824.0f, 392.0f}, {430.0f, 126.0f}, ParseHexColor("#18121EE0"), ParseHexColor("#BC7AB4"), 2.0f, 224.0f};
    d.panels["hint_panel"] = {{40.0f, 648.0f}, {1214.0f, 48.0f}, ParseHexColor("#090E14DA"), ParseHexColor("#7698B0"), 2.0f, 218.0f};
    d.panels["stamina_bar"] = {{850.0f, 156.0f}, {360.0f, 18.0f}, ParseHexColor("#2A3440"), ParseHexColor("#788C9C"), 1.0f, 255.0f};
    d.panels["workshop_progress"] = {{850.0f, 356.0f}, {360.0f, 12.0f}, ParseHexColor("#2E3640"), ParseHexColor("#8C94A6"), 1.0f, 255.0f};
    d.panels["aura_overlay"] = {{0.0f, 0.0f}, {1280.0f, 720.0f}, 0x00000000, 0x00000000, 0.0f, 0.0f};
    d.panels["festival_notice"] = {{40.0f, 16.0f}, {760.0f, 36.0f}, ParseHexColor("#241812DC"), 0x00000000, 0.0f, 220.0f};
    d.panels["stamina_fill"] = {{850.0f, 156.0f}, {360.0f, 18.0f}, ParseHexColor("#78D68A"), 0x00000000, 0.0f, 255.0f};
    d.panels["workshop_fill"] = {{850.0f, 356.0f}, {360.0f, 12.0f}, ParseHexColor("#F2BC68"), 0x00000000, 0.0f, 255.0f};
    d.panels["main_menu_panel"] = {{360.0f, 180.0f}, {560.0f, 360.0f}, ParseHexColor("#FBF4E0F0"), ParseHexColor("#5C3A1E"), 2.0f, 240.0f};

    // Pixel HUD panels (PixelGameHud)
    d.panels["pixel_top_right_info"] = {{998.0f, 6.0f}, {276.0f, 64.0f}, ParseHexColor("#FBF4E0F0"), ParseHexColor("#5C3A1E"), 1.0f, 255.0f};
    d.panels["pixel_bottom_right_status"] = {{998.0f, 658.0f}, {276.0f, 58.0f}, ParseHexColor("#FBF4E0F0"), ParseHexColor("#5C3A1E"), 1.0f, 255.0f};
    d.panels["pixel_toolbar"] = {{332.0f, 662.0f}, {616.0f, 62.0f}, ParseHexColor("#FBF4E0E6"), ParseHexColor("#5C3A1E"), 1.0f, 255.0f};
    d.panels["pixel_dialogue_box"] = {{40.0f, 520.0f}, {1200.0f, 160.0f}, ParseHexColor("#FEFEF8E6"), ParseHexColor("#5C3A1E"), 1.0f, 255.0f};
    d.panels["pixel_inventory"] = {{320.0f, 120.0f}, {640.0f, 480.0f}, ParseHexColor("#FBF4E0F0"), ParseHexColor("#5C3A1E"), 1.0f, 255.0f};
    d.panels["pixel_quest_menu"] = {{820.0f, 150.0f}, {360.0f, 420.0f}, ParseHexColor("#FBF4E0F0"), ParseHexColor("#5C3A1E"), 1.0f, 255.0f};
    d.panels["pixel_minimap"] = {{384.0f, 104.0f}, {512.0f, 512.0f}, ParseHexColor("#E8D8A8F0"), ParseHexColor("#5C3A1E"), 1.0f, 255.0f};
    d.panels["pixel_minimap_inner"] = {{392.0f, 112.0f}, {496.0f, 496.0f}, ParseHexColor("#F7EFD2F0"), 0x00000000, 0.0f, 255.0f};
    d.panels["pixel_stamina_bar"] = {{1002.0f, 664.0f}, {272.0f, 12.0f}, ParseHexColor("#C8C0A8"), ParseHexColor("#6E6454"), 1.0f, 255.0f};
    d.panels["pixel_stamina_fill"] = {{1002.0f, 664.0f}, {272.0f, 12.0f}, ParseHexColor("#8FBC8F"), 0x00000000, 0.0f, 255.0f};
    d.panels["pixel_stamina_low"] = {{1002.0f, 664.0f}, {272.0f, 12.0f}, ParseHexColor("#FFB6C1"), 0x00000000, 0.0f, 255.0f};
    d.panels["pixel_stamina_full"] = {{1002.0f, 664.0f}, {272.0f, 12.0f}, ParseHexColor("#4C7AA8"), 0x00000000, 0.0f, 255.0f};

    d.texts["hud"] = {{850.0f, 34.0f}, 17u, ParseHexColor("#EFF7FF"), 0.0f, 0x000000FF};
    d.texts["inventory"] = {{850.0f, 224.0f}, 17u, ParseHexColor("#F5ECD6"), 0.0f, 0x000000FF};
    d.texts["hint"] = {{60.0f, 660.0f}, 18u, ParseHexColor("#F2F6FC"), 0.0f, 0x000000FF};
    d.texts["dialogue"] = {{850.0f, 408.0f}, 17u, ParseHexColor("#FFE8F4"), 0.0f, 0x000000FF};
    d.texts["debug"] = {{52.0f, 48.0f}, 15u, ParseHexColor("#C2FFB6"), 0.0f, 0x000000FF};
    d.texts["world_tip"] = {{56.0f, 610.0f}, 16u, ParseHexColor("#FFF8DC"), 0.0f, 0x000000FF};
    d.texts["festival_notice"] = {{52.0f, 22.0f}, 16u, ParseHexColor("#FFDA80"), 0.0f, 0x000000FF};
    d.texts["level_up"] = {{440.0f, 300.0f}, 28u, ParseHexColor("#FFDC64"), 1.5f, ParseHexColor("#785014")};
    d.texts["main_menu_title"] = {{520.0f, 220.0f}, 48u, ParseHexColor("#3D2517"), 0.0f, 0x000000FF};
    d.texts["main_menu_item0"] = {{590.0f, 320.0f}, 30u, ParseHexColor("#3D2517"), 0.0f, 0x000000FF};
    d.texts["main_menu_item1"] = {{590.0f, 384.0f}, 30u, ParseHexColor("#3D2517"), 0.0f, 0x000000FF};
    d.texts["main_menu_item2"] = {{590.0f, 448.0f}, 30u, ParseHexColor("#3D2517"), 0.0f, 0x000000FF};

    // Pixel HUD texts (PixelGameHud)
    d.texts["pixel_top_right_time"] = {{1008.0f, 14.0f}, 12u, ParseHexColor("#2D1B0E"), 0.0f, 0x000000FF};
    d.texts["pixel_top_right_season"] = {{1008.0f, 30.0f}, 12u, ParseHexColor("#2D1B0E"), 0.0f, 0x000000FF};
    d.texts["pixel_top_right_weather"] = {{1008.0f, 46.0f}, 12u, ParseHexColor("#2D1B0E"), 0.0f, 0x000000FF};
    d.texts["pixel_coin"] = {{1006.0f, 680.0f}, 14u, ParseHexColor("#D4A84B"), 0.5f, ParseHexColor("#5C3A1E")};
    d.texts["pixel_stamina_warning"] = {{1110.0f, 680.0f}, 14u, ParseHexColor("#2D1B0E"), 0.0f, 0x000000FF};

    d.cloud_colors["cloud_clear"] = {ParseHexColor("#4C7AA8"), 0x00000000};
    d.cloud_colors["cloud_mist"] = {ParseHexColor("#586C7A"), ParseHexColor("#586C7A14")};
    d.cloud_colors["cloud_dense"] = {ParseHexColor("#34404F"), ParseHexColor("#34404F1E")};
    d.cloud_colors["cloud_tide"] = {ParseHexColor("#2A2244"), ParseHexColor("#2A224428")};
    d.semantic_colors["button_default_text"] = ParseHexColor("#3D2517");
    d.semantic_colors["button_hover_text"] = ParseHexColor("#3D2517");
    d.semantic_colors["button_selected_text"] = ParseHexColor("#D4A84B");
    d.semantic_colors["button_disabled_text"] = ParseHexColor("#6E6454");
    d.semantic_colors["button_default_bg"] = ParseHexColor("#FEFEF8");
    d.semantic_colors["button_hover_bg"] = ParseHexColor("#FFFACD");
    d.semantic_colors["button_selected_bg"] = ParseHexColor("#90EE90");
    d.semantic_colors["button_disabled_bg"] = ParseHexColor("#C8C0A8");
    d.semantic_colors["main_menu_item_normal"] = ParseHexColor("#3D2517");
    d.semantic_colors["main_menu_item_selected"] = ParseHexColor("#D4A84B");
    d.semantic_numbers["world_tip_wave_amplitude"] = 2.0f;
    d.semantic_numbers["world_tip_wave_period"] = 0.5f;

    d.primary_font = "default";
    d.fallback_font = "auto";
    return d;
}

}  // namespace CloudSeamanor::infrastructure
