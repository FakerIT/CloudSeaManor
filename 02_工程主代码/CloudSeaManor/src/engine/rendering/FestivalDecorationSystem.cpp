#include "CloudSeamanor/engine/rendering/FestivalDecorationSystem.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"

#include <fstream>
#include <sstream>
#include <cmath>

namespace CloudSeamanor::engine::rendering {

namespace {

// ============================================================================
// 【ParseDecorationType】字符串转装饰类型
// ============================================================================
DecorationType ParseDecorationType(const std::string& type_str) {
    if (type_str == "banner") return DecorationType::Banner;
    if (type_str == "particles") return DecorationType::Particles;
    if (type_str == "moon") return DecorationType::Moon;
    if (type_str == "lantern") return DecorationType::Lantern;
    if (type_str == "peak") return DecorationType::Peak;
    if (type_str == "wave") return DecorationType::Wave;
    if (type_str == "mist") return DecorationType::Mist;
    if (type_str == "overlay") return DecorationType::Overlay;
    if (type_str == "steam") return DecorationType::Steam;
    if (type_str == "bundle") return DecorationType::Bundle;
    return DecorationType::Banner;
}

// ============================================================================
// 【ParseHexColor】解析 #RRGGBB 或 #RRGGBBAA 格式颜色
// ============================================================================
bool ParseHexColor(const std::string& hex, std::uint8_t& r, std::uint8_t& g, std::uint8_t& b, std::uint8_t& a) {
    if (hex.empty() || hex[0] != '#') return false;
    std::string digits = hex.substr(1);
    if (digits.size() != 6 && digits.size() != 8) return false;

    try {
        if (digits.size() == 6) {
            r = static_cast<std::uint8_t>(std::stoi(digits.substr(0, 2), nullptr, 16));
            g = static_cast<std::uint8_t>(std::stoi(digits.substr(2, 2), nullptr, 16));
            b = static_cast<std::uint8_t>(std::stoi(digits.substr(4, 2), nullptr, 16));
            a = 255;
        } else {
            r = static_cast<std::uint8_t>(std::stoi(digits.substr(0, 2), nullptr, 16));
            g = static_cast<std::uint8_t>(std::stoi(digits.substr(2, 2), nullptr, 16));
            b = static_cast<std::uint8_t>(std::stoi(digits.substr(4, 2), nullptr, 16));
            a = static_cast<std::uint8_t>(std::stoi(digits.substr(6, 2), nullptr, 16));
        }
        return true;
    } catch (...) {
        return false;
    }
}

// ============================================================================
// 【ParseFestivalIds】解析节日 ID 列表（逗号分隔）
// ============================================================================
std::vector<std::string> ParseFestivalIds(const std::string& ids_str) {
    std::vector<std::string> result;
    std::istringstream ss(ids_str);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (!token.empty()) {
            // 去除首尾空白
            std::size_t start = 0;
            while (start < token.size() && std::isspace(static_cast<unsigned char>(token[start]))) ++start;
            std::size_t end = token.size();
            while (end > start && std::isspace(static_cast<unsigned char>(token[end - 1]))) --end;
            if (start < end) {
                result.push_back(token.substr(start, end - start));
            }
        }
    }
    return result;
}

// ============================================================================
// 【TrimString】去除字符串首尾空白
// ============================================================================
std::string TrimString(const std::string& s) {
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

}  // namespace

bool FestivalDecorationSystem::LoadFromFile(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        infrastructure::Logger::Warning("FestivalDecorationSystem: 无法打开配置文件: " + config_path);
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();

    decorations_.clear();

    std::istringstream lines(content);
    std::string line;
    DecorationElement current;
    bool in_decoration = false;

    while (std::getline(lines, line)) {
        // 去除注释
        std::size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }

        line = TrimString(line);
        if (line.empty()) continue;

        // 检查是否为新的装饰元素开始
        if (line.find("festival_ids") != std::string::npos) {
            if (in_decoration) {
                decorations_.push_back(current);
                current = DecorationElement{};
            }
            in_decoration = true;
        }

        if (!in_decoration) continue;

        // 解析字段
        std::size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) continue;

        std::string key = TrimString(line.substr(0, colon_pos));
        std::string value = TrimString(line.substr(colon_pos + 1));

        if (key == "type") {
            current.type = ParseDecorationType(value);
        } else if (key == "festival_ids") {
            current.festival_ids = ParseFestivalIds(value);
        } else if (key == "position") {
            std::size_t comma = value.find(',');
            if (comma != std::string::npos) {
                try {
                    current.position_x = std::stof(TrimString(value.substr(0, comma)));
                    current.position_y = std::stof(TrimString(value.substr(comma + 1)));
                } catch (...) {}
            }
        } else if (key == "size") {
            std::size_t comma = value.find(',');
            if (comma != std::string::npos) {
                try {
                    current.width = std::stof(TrimString(value.substr(0, comma)));
                    current.height = std::stof(TrimString(value.substr(comma + 1)));
                } catch (...) {}
            }
        } else if (key == "radius") {
            try { current.radius = std::stof(value); } catch (...) {}
        } else if (key == "point_count") {
            try { current.point_count = std::stof(value); } catch (...) {}
        } else if (key == "fill_color") {
            ParseHexColor(value, current.fill_r, current.fill_g, current.fill_b, current.fill_a);
        } else if (key == "outline_color") {
            ParseHexColor(value, current.outline_r, current.outline_g, current.outline_b, current.outline_a);
        } else if (key == "outline_thickness") {
            try { current.outline_thickness = std::stof(value); } catch (...) {}
        } else if (key == "anim_amplitude") {
            try { current.anim_amplitude = std::stof(value); } catch (...) {}
        } else if (key == "anim_frequency") {
            try { current.anim_frequency = std::stof(value); } catch (...) {}
        } else if (key == "anim_phase") {
            try { current.anim_phase_offset = std::stof(value); } catch (...) {}
        }
    }

    // 添加最后一个装饰元素
    if (in_decoration && !current.festival_ids.empty()) {
        decorations_.push_back(current);
    }

    infrastructure::Logger::Info("FestivalDecorationSystem: 已加载 " +
        std::to_string(decorations_.size()) + " 个装饰元素");
    return !decorations_.empty();
}

std::vector<std::unique_ptr<sf::Drawable>> FestivalDecorationSystem::GetDecorations(
    const std::string& festival_id,
    float session_time) const
{
    std::vector<std::unique_ptr<sf::Drawable>> result;

    for (const auto& element : decorations_) {
        bool matches = std::any_of(element.festival_ids.begin(), element.festival_ids.end(),
            [&festival_id](const std::string& id) { return id == festival_id; });

        if (!matches) continue;

        auto drawable = CreateDrawable_(element, session_time);
        if (drawable) {
            result.push_back(std::move(drawable));
        }
    }

    return result;
}

bool FestivalDecorationSystem::HasFestival(const std::string& festival_id) const {
    return std::any_of(decorations_.begin(), decorations_.end(),
        [&festival_id](const DecorationElement& elem) {
            return std::any_of(elem.festival_ids.begin(), elem.festival_ids.end(),
                [&festival_id](const std::string& id) { return id == festival_id; });
        });
}

std::vector<std::string> FestivalDecorationSystem::GetSupportedFestivals() const {
    std::vector<std::string> festivals;
    for (const auto& elem : decorations_) {
        for (const auto& id : elem.festival_ids) {
            if (std::find(festivals.begin(), festivals.end(), id) == festivals.end()) {
                festivals.push_back(id);
            }
        }
    }
    return festivals;
}

std::unique_ptr<sf::Drawable> FestivalDecorationSystem::CreateDrawable_(
    const DecorationElement& element,
    float session_time) const
{
    // 计算动画偏移
    float anim_offset = 0.0f;
    if (element.anim_amplitude > 0.0f) {
        anim_offset = std::sin(session_time * element.anim_frequency + element.anim_phase_offset)
                     * element.anim_amplitude;
    }

    switch (element.type) {
    case DecorationType::Banner:
    case DecorationType::Wave:
    case DecorationType::Mist:
    case DecorationType::Overlay:
    case DecorationType::Steam: {
        auto rect = std::make_unique<sf::RectangleShape>();
        rect->setSize(sf::Vector2f(element.width, element.height));
        rect->setPosition(sf::Vector2f(element.position_x, element.position_y + anim_offset));
        rect->setFillColor(sf::Color(element.fill_r, element.fill_g, element.fill_b, element.fill_a));
        rect->setOutlineThickness(element.outline_thickness);
        rect->setOutlineColor(sf::Color(element.outline_r, element.outline_g, element.outline_b, element.outline_a));
        return rect;
    }

    case DecorationType::Particles:
    case DecorationType::Moon:
    case DecorationType::Lantern:
    case DecorationType::Bundle: {
        auto circle = std::make_unique<sf::CircleShape>(element.radius,
            static_cast<std::size_t>(element.point_count));
        circle->setPosition(sf::Vector2f(element.position_x, element.position_y + anim_offset));
        circle->setFillColor(sf::Color(element.fill_r, element.fill_g, element.fill_b, element.fill_a));
        circle->setOutlineThickness(element.outline_thickness);
        circle->setOutlineColor(sf::Color(element.outline_r, element.outline_g, element.outline_b, element.outline_a));
        return circle;
    }

    case DecorationType::Peak: {
        auto rect = std::make_unique<sf::RectangleShape>();
        rect->setSize(sf::Vector2f(element.width, element.height));
        rect->setPosition(sf::Vector2f(element.position_x, element.position_y + anim_offset));
        rect->setFillColor(sf::Color(element.fill_r, element.fill_g, element.fill_b, element.fill_a));
        return rect;
    }

    default:
        return nullptr;
    }
}

}  // namespace CloudSeamanor::engine::rendering
