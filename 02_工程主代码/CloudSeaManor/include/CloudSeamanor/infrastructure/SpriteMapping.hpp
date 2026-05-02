#pragma once
// ============================================================================
// 【SpriteMapping】美术资源映射表管理器
// ============================================================================
// 功能：从 CSV 文件加载 sprite_id 到文件路径的映射关系
// 用途：解耦美术资源与游戏逻辑，支持零门槛换皮
// ============================================================================

#include <optional>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

namespace CloudSeamanor::infrastructure {

struct SpriteInfo {
    std::string sprite_id;     // 逻辑资源 ID (唯一)
    std::string category;       // 分类标签 (NPC/UI/Item/...)
    std::string file_path;     // 相对于 assets/sprites/ 的路径
    int frame_count = 1;       // 帧数 (单图为1)
    int fps = 0;               // 动画帧率 (单图为0)
    int width = 0;             // 单帧宽度
    int height = 0;            // 单帧高度
    std::string description;    // 用途说明
    std::string atlas_source;  // 来源 atlas (可选，为空表示独立图片

    // 完整资源路径 = base_path + "assets/sprites/" + file_path
    [[nodiscard]] std::string GetFullPath(const std::string& base_path = "") const {
        return base_path + "assets/sprites/" + file_path;
    }

    // 是否为动画
    [[nodiscard]] bool IsAnimation() const { return frame_count > 1; }
};

class SpriteMapping {
public:
    SpriteMapping() = default;

    // 加载映射表
    // @param csv_path CSV 文件路径 (可以是相对路径或绝对路径)
    // @param base_path 资源根目录 (默认为空，使用相对路径)
    // @return 加载是否成功
    bool LoadFromFile(const std::string& csv_path, const std::string& base_path = "");

    // 重新加载 (用于切换皮肤)
    bool Reload(const std::string& csv_path, const std::string& base_path = "");

    // 查询资源信息
    // @param sprite_id 逻辑资源 ID
    // @return 资源信息 (如果不存在返回 std::nullopt)
    [[nodiscard]] std::optional<SpriteInfo> Get(const std::string& sprite_id) const;

    // 查询是否存在
    [[nodiscard]] bool Contains(const std::string& sprite_id) const {
        return mappings_.find(sprite_id) != mappings_.end();
    }

    // 按分类查询
    [[nodiscard]] std::vector<SpriteInfo> GetByCategory(const std::string& category) const;

    // 获取所有分类
    [[nodiscard]] std::vector<std::string> GetAllCategories() const;

    // 获取所有资源
    [[nodiscard]] const std::unordered_map<std::string, SpriteInfo>& GetAll() const {
        return mappings_;
    }

    // 获取加载状态
    [[nodiscard]] bool IsLoaded() const { return loaded_; }
    [[nodiscard]] const std::string& GetLoadedPath() const { return loaded_path_; }
    [[nodiscard]] const std::string& GetBasePath() const { return base_path_; }

    // 获取统计信息
    [[nodiscard]] size_t Size() const { return mappings_.size(); }

    // 验证资源文件是否存在
    // @return 缺失的文件路径列表
    [[nodiscard]] std::vector<std::string> ValidateFiles() const;

    // 获取分类统计
    struct CategoryStats {
        std::string category;
        size_t count;
    };
    [[nodiscard]] std::vector<CategoryStats> GetCategoryStats() const;

private:
    // 解析 CSV 行
    static std::vector<std::string> ParseCsvLine(const std::string& line);

    // 解析整数值
    static int ParseInt(const std::string& str, int default_val = 0);

    // 去除字符串首尾空白
    static std::string Trim(const std::string& str);

    std::unordered_map<std::string, SpriteInfo> mappings_;
    std::unordered_map<std::string, std::vector<std::string>> category_index_;
    bool loaded_ = false;
    std::string loaded_path_;
    std::string base_path_;
};

}  // namespace CloudSeamanor::infrastructure
