#pragma once

// ============================================================================
// 【AssetBridge】美术资源桥接器
// ============================================================================
// Responsibilities:
// - 作为美术团队与程序引擎之间的"干净接口"
// - 自动生成 .atlas.json 图集元数据（扫描目录 → 自动生成配置）
// - 自动生成 .ui.json 页面布局配置（基于模板生成）
// - 提供编辑器辅助工具：预览、导出、验证
// - 管理资源命名规范（确保美术导出与程序引用的命名一致）
//
// 工作流程（美术 ↔ 程序）:
// 1. 美术在 assets/sprites/ 下放置 PNG 图集
// 2. 运行 AssetBridge::ScanDirectory() 扫描目录
// 3. AssetBridge 自动生成 / 更新 .atlas.json 元数据文件
// 4. 程序通过 SpriteAssetManager::LoadAtlas() 加载
// 5. 美术通过 AssetBridge::ExportPageTemplate() 预览页面布局
//
// 命名规范（强制）:
// - 图集文件名: {category}_{subcategory}.png
//   例: player_main.png, npc_villagers.png, ui_icons.png, tiles_farm.png
// - 帧命名: {subject}_{action}_{direction}_{index}
//   例: player_idle_down_0, tree_cherry_blossom, tea_leaf_green
// - 动画命名: {subject}_{action}_{direction}
//   例: player_idle_down, player_walk_right, npc_acha_talk
// ============================================================================

#include "CloudSeamanor/infrastructure/SpriteAssetManager.hpp"
#include "CloudSeamanor/engine/UiLayoutSystem.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace CloudSeamanor::infrastructure {

// ============================================================================
// 【AssetBridgeConfig】桥接器配置
// ============================================================================
struct AssetBridgeConfig {
    std::string project_root;              // 项目根目录
    std::string sprites_dir = "assets/sprites";
    std::string pages_dir = "assets/ui/pages";
    std::string output_dir;               // 输出目录（默认同 sprites_dir）

    // 自动扫描选项
    bool auto_generate_atlas = true;      // 扫描时自动生成 .atlas.json
    bool validate_naming = true;          // 验证命名规范
    bool create_default_page = false;      // 扫描时自动生成页面模板

    // 命名规范
    int default_frame_width = 32;          // 默认帧宽（像素）
    int default_frame_height = 32;         // 默认帧高（像素）
    int grid_columns = 8;                 // 默认图集列数

    // 导出选项
    bool pretty_print_json = true;         // JSON 格式化输出
    bool backup_existing = true;           // 覆盖前备份
};

// ============================================================================
// 【AtlasGeneratorResult】图集生成结果
// ============================================================================
struct AtlasGeneratorResult {
    std::string atlas_id;
    std::string atlas_path;
    std::string png_path;
    int frame_count = 0;
    int animation_count = 0;
    std::vector<std::string> warnings;
    bool success = false;
};

// ============================================================================
// 【NamingViolation】命名违规
// ============================================================================
struct NamingViolation {
    std::string file_path;
    std::string rule_name;
    std::string suggestion;
    std::string current_name;
};

// ============================================================================
// 【AssetBridge】美术资源桥接器
// ============================================================================
class AssetBridge {
public:
    explicit AssetBridge(const AssetBridgeConfig& config);

    // ========================================================================
    // 【图集管理】
    // ========================================================================

    /**
     * @brief 扫描目录，自动生成图集元数据
     * @param sprites_dir 资源目录
     * @return 每张图的生成结果
     */
    std::vector<AtlasGeneratorResult> ScanDirectory(const std::string& sprites_dir);

    /**
     * @brief 扫描单个图集 PNG 文件并生成 .atlas.json
     */
    AtlasGeneratorResult ScanAndGenerateAtlas(const std::string& png_path);

    /**
     * @brief 生成图集元数据（不写文件，仅返回配置）
     */
    std::optional<SpriteMetadata> GenerateAtlasMetadata(const std::string& png_path);

    /**
     * @brief 验证命名规范
     * @return 所有违规项
     */
    std::vector<NamingViolation> ValidateNamingConventions();

    // ========================================================================
    // 【页面布局管理】
    // ========================================================================

    /**
     * @brief 生成空页面布局模板
     * @param page_id 页面 ID
     * @param category 页面分类（决定使用哪个模板）
     */
    UiPageConfig GeneratePageTemplate(const std::string& page_id,
                                       const std::string& category = "generic");

    /**
     * @brief 从现有 .ui.json 加载页面配置
     */
    std::optional<UiPageConfig> LoadPageConfig(const std::string& path);

    /**
     * @brief 保存页面配置到 .ui.json
     */
    bool SavePageConfig(const std::string& path, const UiPageConfig& config);

    // ========================================================================
    // 【批量操作】
    // ========================================================================

    /**
     * @brief 生成所有图集元数据（扫描 sprites 目录下所有 PNG）
     * @return 成功/失败统计
     */
    struct BatchResult {
        int total = 0;
        int success = 0;
        int skipped = 0;
        int failed = 0;
        std::vector<AtlasGeneratorResult> results;
    };
    BatchResult GenerateAllAtlases();

    // ========================================================================
    // 【资源验证】
    // ========================================================================

    /**
     * @brief 验证图集元数据的一致性（PNG 尺寸 vs JSON 定义）
     */
    struct ValidationResult {
        bool valid = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };
    ValidationResult ValidateAtlas(const std::string& atlas_id);

    /**
     * @brief 验证页面配置引用的资源是否存在
     */
    ValidationResult ValidatePageReferences(const UiPageConfig& page_config);

    // ========================================================================
    // 【配置】
    // ========================================================================
    [[nodiscard]] const AssetBridgeConfig& GetConfig() const { return config_; }
    void SetConfig(const AssetBridgeConfig& config);

    // ========================================================================
    // 【预置模板】
    // ========================================================================

    /** 获取所有预置页面模板 */
    static std::vector<UiPageConfig> GetPresetPageTemplates();

    /** 获取指定类型的预置模板 */
    static std::optional<UiPageConfig> GetPresetTemplate(const std::string& category);

private:
    AssetBridgeConfig config_;

    // ========================================================================
    // 【内部工具】
    // ========================================================================

    std::string DeriveAtlasId_(const std::string& png_filename) const;
    std::string BuildAtlasJsonPath_(const std::string& png_path) const;

    std::vector<SpriteFrame> ParseGridLayout_(const std::string& png_path,
                                                int frame_w, int frame_h,
                                                int cols, const std::string& frame_prefix);

    std::optional<SpriteAnimation> BuildAnimation_(const std::string& name,
                                                   const std::vector<SpriteFrame>& frames,
                                                   bool loop);

    std::string GetCategoryFromPath_(const std::string& png_path) const;

    NamingViolation CheckFrameNaming_(const std::string& frame_name) const;

    static std::string NormalizePath_(const std::string& path);
};

// ============================================================================
// 【预置页面模板工厂】
// ============================================================================
// 美术可调用这些模板快速生成页面骨架
class PresetPageFactory {
public:
    // 生成背包页面模板
    static UiPageConfig MakeInventoryPage();

    // 生成工坊页面模板
    static UiPageConfig MakeWorkshopPage();

    // 生成契约书卷模板
    static UiPageConfig MakeContractPage();

    // 生成 NPC 详情页模板
    static UiPageConfig MakeNpcDetailPage();

    // 生成灵兽面板模板
    static UiPageConfig MakeSpiritBeastPage();

    // 生成存档读档模板
    static UiPageConfig MakeSaveLoadPage();

    // 生成设置面板模板
    static UiPageConfig MakeSettingsPage();

    // 生成云海预报模板
    static UiPageConfig MakeCloudForecastPage();

    // 生成主菜单模板
    static UiPageConfig MakeMainMenuPage();

    // 生成玩家状态面板模板
    static UiPageConfig MakePlayerStatusPage();

    // 生成茶园面板模板
    static UiPageConfig MakeTeaGardenPage();

    // 生成节日面板模板
    static UiPageConfig MakeFestivalPage();

    // 生成灵界探索模板
    static UiPageConfig MakeSpiritRealmPage();

    // 生成建筑升级模板
    static UiPageConfig MakeBuildingPage();
};

// ============================================================================
// 【AssetNamingConvention】命名规范工具
// ============================================================================
struct AssetNamingConvention {
    // 帧 ID 格式: {subject}_{action}_{direction}_{index}
    static bool IsValidFrameId(const std::string& id);
    static std::pair<std::string, std::string> SplitFrameId(const std::string& id);

    // 图集 ID 格式: {category}_{subcategory}
    static bool IsValidAtlasId(const std::string& id);

    // 方向后缀验证
    static bool IsValidDirection(const std::string& suffix);
    static int DirectionSuffixToIndex(const std::string& suffix);

    // 标准化: 将文件名转为符合规范的 ID
    static std::string NormalizeToFrameId(const std::string& raw_name);

    // 方向后缀表
    static constexpr const char* kValidDirections[8] = {
        "down", "down_right", "right", "up_right",
        "up", "up_left", "left", "down_left"
    };
};

}  // namespace CloudSeamanor::infrastructure
