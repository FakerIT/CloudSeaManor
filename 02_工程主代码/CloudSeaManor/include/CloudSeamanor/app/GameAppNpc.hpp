#pragma once

#include "CloudSeamanor/engine/GameAppRuntimeTypes.hpp"

#include <SFML/System/Vector2.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::engine {

/**
 * @brief 从 `HH:MM` 文本解析出当天分钟数。
 *
 * @param time_text 形如 `06:30` 的时间字符串。
 * @return 对应分钟数；格式不合法时返回 0。
 */
int ParseTimeToMinutes(const std::string& time_text);

/**
 * @brief 按逗号拆分一行 CSV 文本。
 *
 * @param line 一整行 CSV 文本。
 * @return 拆分后的单元格列表。
 */
std::vector<std::string> SplitCsvLine(const std::string& line);

/**
 * @brief 根据地点标识返回世界中的目标锚点。
 *
 * @param location 地点标识。
 * @return NPC 应移动到的世界坐标。
 */
sf::Vector2f AnchorForLocation(const std::string& location);

/**
 * @brief 从 JSON 片段中读取指定数组字段。
 *
 * @param object_text 单个对象的 JSON 文本。
 * @param key 目标字段名。
 * @return 数组中所有字符串元素。
 */
std::vector<std::string> ParseJsonArray(const std::string& object_text, const std::string& key);

/**
 * @brief 解析指定 NPC 的礼物偏好。
 *
 * @param json_text 完整礼物偏好 JSON 文本。
 * @param npc_id 目标 NPC 标识。
 * @return 该 NPC 的礼物偏好集合。
 */
NpcGiftPrefs ParseGiftPrefsForNpc(const std::string& json_text, const std::string& npc_id);

/**
 * @brief 从 JSON 文本中解析字符串映射表。
 *
 * @param json_text 完整 JSON 文本。
 * @param key 目标对象字段名。
 * @return 字符串到字符串的映射表。
 */
std::unordered_map<std::string, std::string> ParseJsonObjectMap(const std::string& json_text, const std::string& key);

/**
 * @brief 读取 NPC 文本映射表。
 *
 * @param path NPC 文本映射 JSON 路径。
 * @param npc_text_mappings 输出文本映射结构。
 * @return `true` 表示读取成功。
 */
bool LoadNpcTextMappings(const std::string& path, NpcTextMappings& npc_text_mappings);

/**
 * @brief 根据映射表返回 NPC 显示名称。
 *
 * @param npc_text_mappings 当前文本映射表。
 * @param npc_id NPC 唯一标识。
 * @return 对应显示名称；不存在时回退到原标识。
 */
std::string NpcDisplayName(const NpcTextMappings& npc_text_mappings, const std::string& npc_id);

/**
 * @brief 根据映射表返回地点显示名称。
 *
 * @param npc_text_mappings 当前文本映射表。
 * @param location 地点标识。
 * @return 对应显示名称；不存在时回退到原标识。
 */
std::string LocationDisplayName(const NpcTextMappings& npc_text_mappings, const std::string& location);

/**
 * @brief 根据映射表返回活动显示名称。
 *
 * @param npc_text_mappings 当前文本映射表。
 * @param activity 活动标识。
 * @return 对应显示名称；不存在时回退到原标识。
 */
std::string ActivityDisplayName(const NpcTextMappings& npc_text_mappings, const std::string& activity);

/**
 * @brief 判断某个物品是否存在于偏好列表中。
 *
 * @param items 目标物品列表。
 * @param item 目标物品标识。
 * @return `true` 表示存在。
 */
bool ContainsItem(const std::vector<std::string>& items, const std::string& item);

struct NpcDataRow {
    std::string id;
    std::string display_name;
    std::string home_location;
    std::string work_location;
    std::string schedule_profile_id;
    std::string gift_profile_id;
    std::string life_stage_profile_id;
    float position_x = 0.0f;
    float position_y = 0.0f;
};

bool LoadNpcDataTable(const std::string& path,
                      std::unordered_map<std::string, NpcDataRow>& out_rows);

/**
 * @brief 从 CSV 与 JSON 原型数据构建 NPC 列表。
 *
 * @param schedule_path 日程 CSV 文件路径。
 * @param gift_path 礼物偏好 JSON 文件路径。
 * @param npc_text_mappings 当前文本映射表。
 * @param npcs 输出 NPC 列表。
 */
void BuildNpcs(const std::string& schedule_path,
               const std::string& gift_path,
               const std::string& npc_data_path,
               const NpcTextMappings& npc_text_mappings,
               std::vector<NpcActor>& npcs);

} // namespace CloudSeamanor::engine
