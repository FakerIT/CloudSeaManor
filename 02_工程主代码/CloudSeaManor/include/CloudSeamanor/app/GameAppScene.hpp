#pragma once

#include "CloudSeamanor/engine/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/engine/Interactable.hpp"
#include "CloudSeamanor/engine/PickupDrop.hpp"

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace CloudSeamanor::domain {
class Player;
}

namespace CloudSeamanor::infrastructure {
class TmxMap;
}

namespace CloudSeamanor::engine {

/**
 * @brief 验证关键数据资源是否存在且可读。
 *
 * @param path 资源文件路径。
 * @param asset_label 资源显示名称。
 * @param log_error 错误日志回调。
 * @param log_info 普通日志回调。
 * @return `true` 表示资源可用。
 */
bool ValidateDataAsset(const std::filesystem::path& path,
                       std::string_view asset_label,
                       const std::function<void(std::string_view)>& log_error,
                       const std::function<void(std::string_view)>& log_info);

/**
 * @brief 构建代码内置的后备场景。
 *
 * @param ground_tiles 输出地面图块列表。
 * @param obstacle_shapes 输出障碍物形状列表。
 * @param obstacle_bounds 输出障碍物碰撞边界列表。
 * @param interactables 输出可交互对象列表。
 * @param pickups 输出掉落物列表，函数会清空它。
 */
void BuildSceneFallback(std::vector<sf::RectangleShape>& ground_tiles,
                        std::vector<sf::RectangleShape>& obstacle_shapes,
                        std::vector<sf::FloatRect>& obstacle_bounds,
                        std::vector<CloudSeamanor::domain::Interactable>& interactables,
                        std::vector<CloudSeamanor::domain::PickupDrop>& pickups);

/**
 * @brief 根据 TMX 地图数据构建场景。
 *
 * @param tmx_map 当前地图对象。
 * @param ground_tiles 输出地面图块列表。
 * @param obstacle_shapes 输出障碍物形状列表。
 * @param obstacle_bounds 输出障碍物碰撞边界列表。
 * @param interactables 输出可交互对象列表。
 * @param pickups 输出掉落物列表，函数会清空它。
 * @param world_bounds 输出世界可活动范围。
 */
void BuildSceneFromMap(const CloudSeamanor::infrastructure::TmxMap& tmx_map,
                       std::vector<sf::RectangleShape>& ground_tiles,
                       std::vector<sf::RectangleShape>& obstacle_shapes,
                       std::vector<sf::FloatRect>& obstacle_bounds,
                       std::vector<CloudSeamanor::domain::Interactable>& interactables,
                       std::vector<CloudSeamanor::domain::PickupDrop>& pickups,
                       sf::FloatRect& world_bounds);

/**
 * @brief 优先使用 TMX 地图，否则回退到内置场景。
 *
 * @param tmx_map 当前地图对象。
 * @param ground_tiles 输出地面图块列表。
 * @param obstacle_shapes 输出障碍物形状列表。
 * @param obstacle_bounds 输出障碍物碰撞边界列表。
 * @param interactables 输出可交互对象列表。
 * @param pickups 输出掉落物列表。
 * @param world_bounds 输出世界可活动范围。
 * @param log_info 普通日志回调。
 * @param log_warning 警告日志回调。
 */
void BuildScene(CloudSeamanor::infrastructure::TmxMap& tmx_map,
                std::vector<sf::RectangleShape>& ground_tiles,
                std::vector<sf::RectangleShape>& obstacle_shapes,
                std::vector<sf::FloatRect>& obstacle_bounds,
                std::vector<CloudSeamanor::domain::Interactable>& interactables,
                std::vector<CloudSeamanor::domain::PickupDrop>& pickups,
                sf::FloatRect& world_bounds,
                const std::string& map_path,
                const std::function<void(std::string_view)>& log_info,
                const std::function<void(std::string_view)>& log_warning);

/**
 * @brief 根据玩家当前位置刷新所有可交互目标的高亮状态。
 *
 * @param player 当前玩家对象。
 * @param interactables 当前可交互对象列表。
 * @param tea_plots 当前茶田列表。
 * @param npcs 当前 NPC 列表。
 * @param spirit_beast 当前灵兽实体。
 * @param highlighted_index 输出高亮交互对象索引。
 * @param highlighted_plot_index 输出高亮茶田索引。
 * @param highlighted_npc_index 输出高亮 NPC 索引。
 * @param spirit_beast_highlighted 输出灵兽高亮状态。
 * @param refresh_tea_plot_visual 刷新茶田视觉的回调。
 * @param refresh_spirit_beast_visual 刷新灵兽视觉的回调。
 */
void UpdateHighlightedInteractable(const CloudSeamanor::domain::Player& player,
                                   std::vector<CloudSeamanor::domain::Interactable>& interactables,
                                   std::vector<TeaPlot>& tea_plots,
                                   std::vector<NpcActor>& npcs,
                                   SpiritBeast& spirit_beast,
                                   int& highlighted_index,
                                   int& highlighted_plot_index,
                                   int& highlighted_npc_index,
                                   bool& spirit_beast_highlighted,
                                   const std::function<void(TeaPlot&, bool)>& refresh_tea_plot_visual,
                                   const std::function<void(SpiritBeast&, bool)>& refresh_spirit_beast_visual);

} // namespace CloudSeamanor::engine
