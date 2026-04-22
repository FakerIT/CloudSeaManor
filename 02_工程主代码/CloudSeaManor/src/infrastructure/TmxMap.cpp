#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/TmxMap.hpp"
#include "CloudSeamanor/Logger.hpp"

#include <cstdint>
#include <exception>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace CloudSeamanor::infrastructure {

namespace {

// 把 CSV 文本解析成整数列表。
// 当前 TMX 的地面图层使用 csv 编码，因此需要先把这一段文本转成 tile id 数组。
std::vector<int> ParseCsvInts(const std::string& text) {
    std::vector<int> values;
    std::string token;
    for (const char ch : text) {
        // 碰到分隔符时，如果 token 里已经有数字，就提交一个整数。
        if (ch == ',' || ch == '\n' || ch == '\r' || ch == ' ' || ch == '\t') {
            if (!token.empty()) {
                try {
                    values.push_back(std::stoi(token));
                } catch (...) {
                    values.push_back(0);
                }
                token.clear();
            }
            continue;
        }
        token.push_back(ch);
    }

    // 处理最后一个 token，避免末尾没有分隔符时漏掉数据。
    if (!token.empty()) {
        try {
            values.push_back(std::stoi(token));
        } catch (...) {
            values.push_back(0);
        }
    }
    return values;
}

void BuildPlaceholderGround(
    int width,
    int height,
    std::vector<int>& ground_tiles) {
    ground_tiles.clear();
    if (width <= 0 || height <= 0) return;
    ground_tiles.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 1);
}

} // namespace

bool TmxMap::LoadFromFile(const std::string& path) {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        Logger::Warning("TmxMap: 无法打开地图文件，回退到占位地图: " + path);
        map_width_ = 64;
        map_height_ = 64;
        tile_width_ = 16;
        tile_height_ = 16;
        world_size_ = {static_cast<float>(map_width_ * tile_width_), static_cast<float>(map_height_ * tile_height_)};
        obstacles_.clear();
        interactables_.clear();
        BuildPlaceholderGround(map_width_, map_height_, ground_tiles_);
        return true;
    }

    // 每次重新读取地图前先清空旧结果，避免残留数据污染新场景。
    obstacles_.clear();
    interactables_.clear();
    ground_tiles_.clear();

    // 把整个文件先读成一段字符串，后面统一做轻量解析。
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    const std::string text = buffer.str();

    // 先解析地图基础尺寸。
    ParseIntAttribute(text, "width", map_width_);
    ParseIntAttribute(text, "height", map_height_);
    ParseIntAttribute(text, "tilewidth", tile_width_);
    ParseIntAttribute(text, "tileheight", tile_height_);
    world_size_ = {static_cast<float>(map_width_ * tile_width_), static_cast<float>(map_height_ * tile_height_)};
    if (map_width_ <= 0 || map_height_ <= 0 || tile_width_ <= 0 || tile_height_ <= 0) {
        Logger::Warning("TmxMap: 地图尺寸字段异常，使用占位默认尺寸。 path=" + path);
        map_width_ = std::max(1, map_width_);
        map_height_ = std::max(1, map_height_);
        tile_width_ = std::max(1, tile_width_);
        tile_height_ = std::max(1, tile_height_);
        world_size_ = {static_cast<float>(map_width_ * tile_width_), static_cast<float>(map_height_ * tile_height_)};
    }

    // 解析地面图层的 CSV 数据。
    const auto data_start_tag = text.find("<data encoding=\"csv\">");
    if (data_start_tag != std::string::npos) {
        const auto data_start = data_start_tag + std::string("<data encoding=\"csv\">").size();
        const auto data_end = text.find("</data>", data_start);
        if (data_end != std::string::npos) {
            ground_tiles_ = ParseCsvInts(text.substr(data_start, data_end - data_start));
        }
    }
    if (ground_tiles_.empty()) {
        Logger::Warning("TmxMap: Ground 图层缺失或解析失败，使用占位地面。 path=" + path);
        BuildPlaceholderGround(map_width_, map_height_, ground_tiles_);
    }

    // 按行扫描对象组，提取障碍物和交互对象。
    std::istringstream lines(text);
    std::string line;
    enum class Mode { None, Obstacles, Interactables } mode = Mode::None;
    int skipped_obstacles = 0;
    int skipped_interactables = 0;

    while (std::getline(lines, line)) {
        if (line.find("<objectgroup name=\"Obstacles\"") != std::string::npos) {
            mode = Mode::Obstacles;
            continue;
        }
        if (line.find("<objectgroup name=\"Interactables\"") != std::string::npos) {
            mode = Mode::Interactables;
            continue;
        }
        if (line.find("</objectgroup>") != std::string::npos) {
            mode = Mode::None;
            continue;
        }
        if (line.find("<object ") == std::string::npos) {
            continue;
        }

        if (mode == Mode::Obstacles) {
            TmxObjectRect rect;
            if (!ParseAttribute(line, "x", rect.x) ||
                !ParseAttribute(line, "y", rect.y) ||
                !ParseAttribute(line, "width", rect.width) ||
                !ParseAttribute(line, "height", rect.height)) {
                ++skipped_obstacles;
                continue;
            }
            obstacles_.push_back(rect);
            continue;
        }

        if (mode == Mode::Interactables) {
            TmxInteractableObject interactable;
            if (!ParseAttribute(line, "x", interactable.rect.x) ||
                !ParseAttribute(line, "y", interactable.rect.y) ||
                !ParseAttribute(line, "width", interactable.rect.width) ||
                !ParseAttribute(line, "height", interactable.rect.height)) {
                ++skipped_interactables;
                continue;
            }

            // 额外读取名字、类型、物品、数量，让地图文件本身具备一定数据驱动能力。
            ParseStringAttribute(line, "name", interactable.name);
            ParseStringAttribute(line, "type", interactable.type);
            ParseStringAttribute(line, "item", interactable.item);
            ParseIntAttribute(line, "count", interactable.count);
            if (interactable.count <= 0) {
                interactable.count = 1;
            }
            interactables_.push_back(interactable);
        }
    }
    if (skipped_obstacles > 0 || skipped_interactables > 0) {
        Logger::Warning(
            "TmxMap: 已跳过无效对象，obstacles=" + std::to_string(skipped_obstacles)
            + ", interactables=" + std::to_string(skipped_interactables));
    }

    return true;
}

bool TmxMap::ParseAttribute(const std::string& text, const std::string& key, float& out_value) {
    const std::string token = key + "=\"";
    const auto start = text.find(token);
    if (start == std::string::npos) {
        return false;
    }

    const auto value_start = start + token.size();
    const auto value_end = text.find('"', value_start);
    if (value_end == std::string::npos) {
        return false;
    }

    try {
        out_value = std::stof(text.substr(value_start, value_end - value_start));
        return true;
    } catch (...) {
        return false;
    }
}

bool TmxMap::ParseIntAttribute(const std::string& text, const std::string& key, int& out_value) {
    // 复用浮点解析逻辑，再转成整数。
    // 这样可以减少重复代码，让属性读取行为更一致。
    float value = static_cast<float>(out_value);
    if (!ParseAttribute(text, key, value)) {
        return false;
    }
    out_value = static_cast<int>(value);
    return true;
}

bool TmxMap::ParseStringAttribute(const std::string& text, const std::string& key, std::string& out_value) {
    const std::string token = key + "=\"";
    const auto start = text.find(token);
    if (start == std::string::npos) {
        return false;
    }

    const auto value_start = start + token.size();
    const auto value_end = text.find('"', value_start);
    if (value_end == std::string::npos) {
        return false;
    }

    out_value = text.substr(value_start, value_end - value_start);
    return true;
}

} // namespace CloudSeamanor::infrastructure
