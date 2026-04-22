#pragma once

#include <SFML/System/Vector2.hpp>

#include <string>
#include <vector>

namespace CloudSeamanor::infrastructure {

// TMX 中的矩形对象。
// 当前用于表示障碍物包围盒，采用最简单的矩形结构便于直接转成碰撞区域。
struct TmxObjectRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

// TMX 中的可交互对象数据。
// 除了矩形位置，还会记录名字、类型、奖励物品和数量，
// 以便地图数据能直接驱动交互对象生成。
struct TmxInteractableObject {
    TmxObjectRect rect;
    std::string name;
    std::string type;
    std::string item;
    int count = 1;
};

// 轻量 TMX 地图读取器。
// 当前只读取原型所需的少量信息：地面图块、障碍物、交互对象、地图尺寸。
// 这样做的好处是实现成本低，但已经足够支撑关卡原型和场景搭建。
class TmxMap {
public:
    // 从 TMX 文件读取地图。
    bool LoadFromFile(const std::string& path);

    [[nodiscard]] const std::vector<TmxObjectRect>& Obstacles() const noexcept { return obstacles_; }
    [[nodiscard]] const std::vector<TmxInteractableObject>& Interactables() const noexcept { return interactables_; }
    [[nodiscard]] const std::vector<int>& GroundTiles() const noexcept { return ground_tiles_; }
    [[nodiscard]] sf::Vector2f WorldSize() const noexcept { return world_size_; }
    [[nodiscard]] int MapWidth() const noexcept { return map_width_; }
    [[nodiscard]] int MapHeight() const noexcept { return map_height_; }
    [[nodiscard]] int TileWidth() const noexcept { return tile_width_; }
    [[nodiscard]] int TileHeight() const noexcept { return tile_height_; }

private:
    // 从文本中解析浮点属性。
    static bool ParseAttribute(const std::string& text, const std::string& key, float& out_value);

    // 在浮点解析基础上进一步读取整数属性。
    static bool ParseIntAttribute(const std::string& text, const std::string& key, int& out_value);

    // 读取字符串属性。
    static bool ParseStringAttribute(const std::string& text, const std::string& key, std::string& out_value);

    int map_width_ = 30;
    int map_height_ = 16;
    int tile_width_ = 40;
    int tile_height_ = 40;
    sf::Vector2f world_size_{1200.0f, 640.0f};
    std::vector<TmxObjectRect> obstacles_;
    std::vector<TmxInteractableObject> interactables_;
    std::vector<int> ground_tiles_;
};

} // namespace CloudSeamanor::infrastructure
