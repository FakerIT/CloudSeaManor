#pragma once

// ============================================================================
// 【UiLayoutSystem】UI 页面布局与美术资源接口
// ============================================================================
// Responsibilities:
// - 数据驱动的 UI 页面布局（XML/JSON 配置）
// - UI 元素与美术资源的绑定（纹理、图标、背景图）
// - 页面生命周期管理（打开/关闭/切换）
// - 布局描述语言：支持锚点、间距、比例、嵌套
//
// 核心概念:
// - UiPage: 一个完整 UI 页面（背包面板、设置面板等）
// - UiElement: 页面内的元素（按钮、图片、进度条等）
// - ElementStyle: 元素的视觉样式（来自图集或配置）
// - LayoutRule: 布局规则（绝对位置、流式、网格）
//
// 设计原则:
// - 美术资源通过 ElementStyle 引用 SpriteAssetManager 的资源 ID
// - 所有布局数据来自 .ui.json 配置文件，支持热重载
// - 布局描述与渲染逻辑完全分离
// ============================================================================

#include "CloudSeamanor/SpriteAssetManager.hpp"
#include "CloudSeamanor/SpriteRenderer.hpp"
#include "CloudSeamanor/PixelArtStyle.hpp"
#include "CloudSeamanor/PixelFontRenderer.hpp"

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace CloudSeamanor::engine {

// ============================================================================
// 【LayoutRule】布局规则类型
// ============================================================================
enum class LayoutRule : std::uint8_t {
    Absolute,    // 绝对像素坐标
    Anchor,      // 锚点布局（相对父容器）
    Flow,        // 流式布局（横向/纵向排列）
    Grid,        // 网格布局
    Stack        // 堆叠布局（同位叠加）
};

// ============================================================================
// 【AnchorPoint】锚点
// ============================================================================
enum class AnchorPoint : std::uint8_t {
    TopLeft, TopCenter, TopRight,
    MiddleLeft, MiddleCenter, MiddleRight,
    BottomLeft, BottomCenter, BottomRight
};

// ============================================================================
// 【UiElementType】UI 元素类型
// ============================================================================
enum class UiElementType : std::uint8_t {
    Empty,          // 占位空白
    Image,          // 静态图片（来自图集）
    Image9Patch,    // 9宫格图片（UI 面板背景）
    AnimatedSprite, // 动画精灵
    Text,           // 文本
    Button,         // 按钮
    ProgressBar,    // 进度条
    IconSlot,       // 图标槽
    ScrollPanel,    // 滚动面板
    TabBar,         // 标签栏
    HeartBar,       // 好感心条
    Checkbox,       // 复选框
    Slider,         // 滑动条
    Container       // 容器（嵌套用）
};

// ============================================================================
// 【UiElementStyle】元素视觉样式（美术资源绑定）
// ============================================================================
struct UiElementStyle {
    // ----- 背景/图片 -----
    std::string sprite_id;              // 精灵帧 ID（来自 SpriteAssetManager）
    std::string atlas_id;               // 图集 ID
    bool use_9patch = false;            // 是否 9 宫格
    // 9 宫格各区域尺寸（像素）
    int nine_patch_border[4] = {0, 0, 0, 0}; // {left, top, right, bottom}

    // ----- 颜色 -----
    sf::Color tint = sf::Color::White;  // 颜色叠加
    float alpha = 1.0f;                  // 透明度

    // ----- 边框 -----
    bool draw_border = false;
    sf::Color border_color = {92, 58, 30}; // BrownOutline
    float border_thickness = 1.0f;

    // ----- 状态图片（按钮/图标槽用） -----
    std::string sprite_default;          // 默认状态
    std::string sprite_hover;            // Hover 状态
    std::string sprite_pressed;          // 按下状态
    std::string sprite_disabled;         // 禁用状态
    std::string sprite_selected;         // 选中状态

    // ----- 文本样式 -----
    TextStyle text_style;

    // ----- 动画 -----
    bool animate_fade_in = true;        // 打开时淡入
    float animation_duration = 0.15f;
};

// ============================================================================
// 【UiElement】UI 元素
// ============================================================================
struct UiElement {
    std::string id;                      // 元素唯一 ID
    UiElementType type = UiElementType::Empty;

    // ----- 布局 -----
    LayoutRule layout = LayoutRule::Absolute;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float min_width = 0.0f;
    float min_height = 0.0f;
    float max_width = 0.0f;
    float max_height = 0.0f;

    // 锚点（LayoutRule::Anchor 时生效）
    AnchorPoint anchor = AnchorPoint::TopLeft;
    float anchor_offset_x = 0.0f;
    float anchor_offset_y = 0.0f;

    // 间距（LayoutRule::Flow/Grid 时生效）
    float spacing_x = 0.0f;
    float spacing_y = 0.0f;
    int grid_columns = 1;              // Grid 列数

    // 边距
    float margin_left = 0.0f;
    float margin_top = 0.0f;
    float margin_right = 0.0f;
    float margin_bottom = 0.0f;
    float padding_left = 0.0f;
    float padding_top = 0.0f;
    float padding_right = 0.0f;
    float padding_bottom = 0.0f;

    // ----- 视觉样式 -----
    UiElementStyle style;

    // ----- 特殊属性 -----
    // 进度条
    float progress_value = 0.0f;
    float progress_max = 1.0f;
    // 文本
    std::string text_content;
    // 心条
    int heart_count = 0;
    int heart_max = 10;
    // 图标槽
    std::string item_id;               // 物品 ID（绑定到背包数据）
    int item_count = 0;
    // TabBar
    std::vector<std::string> tab_ids;
    int active_tab = 0;
    // 容器
    std::vector<std::string> children_ids;  // 子元素 ID 列表

    // ----- 交互 -----
    bool interactive = true;
    bool visible = true;
    bool enabled = true;
    std::string tooltip_text;
    std::string tooltip_description;

    // ----- 业务绑定（回调） -----
    std::function<void(const std::string& element_id)> on_click;
    std::function<void(const std::string& element_id)> on_hover;
    std::function<void(const std::string& element_id)> on_value_change;

    // ----- 运行时状态 -----
    bool hovered = false;
    bool pressed = false;
    bool focused = false;  // 手柄焦点
};

// ============================================================================
// 【UiPageConfig】页面配置文件（JSON Schema）
// ============================================================================
// 对应 assets/ui/pages/*.ui.json
struct UiPageConfig {
    std::string page_id;             // 页面 ID，如 "inventory", "workshop"
    std::string title;               // 显示标题
    int width = 640;                // 页面宽度
    int height = 480;               // 页面高度
    std::string background_style;    // 背景样式 ID
    bool modal = false;              // 是否模态（打开时其他交互禁止）
    bool pause_game = false;         // 打开时是否暂停游戏
    std::string close_key = "Escape"; // 关闭快捷键

    // 根级元素 ID 列表
    std::vector<std::string> root_element_ids;
};

// ============================================================================
// 【UiPage】可绘制 UI 页面
// ============================================================================
class UiPage : public sf::Drawable {
public:
    // ========================================================================
    // 【构造】
    // ========================================================================
    UiPage();
    explicit UiPage(const UiPageConfig& config);

    // ========================================================================
    // 【配置】
    // ========================================================================
    void LoadFromConfig(const UiPageConfig& config);
    void AddElement(const UiElement& element);
    void RemoveElement(const std::string& element_id);

    /**
     * @brief 获取元素
     */
    [[nodiscard]] UiElement* GetElement(const std::string& id);
    [[nodiscard]] const UiElement* GetElement(const std::string& id) const;

    /**
     * @brief 绑定资产管理器（用于加载精灵）
     */
    void SetAssetManager(infrastructure::SpriteAssetManager* mgr);

    /**
     * @brief 绑定字体渲染器（用于渲染文字）
     */
    void SetFontRenderer(PixelFontRenderer* renderer);

    /**
     * @brief 设置位置（页面中心对齐）
     */
    void SetCenter(const sf::Vector2f& center);

    // ========================================================================
    // 【页面生命周期】
    // ========================================================================
    void Open();
    void Close();
    void Toggle();
    [[nodiscard]] bool IsOpen() const { return open_; }
    [[nodiscard]] bool IsAnimating() const { return anim_state_ != AnimationState::Idle; }

    // ========================================================================
    // 【数据绑定】
    // ========================================================================

    /**
     * @brief 绑定元素数据（如背包物品列表、任务列表）
     * @param element_id 元素 ID
     * @param data_key 数据键名
     * @param data 数据
     */
    template <typename T>
    void BindData(const std::string& element_id, const std::string& data_key, T&& data) {
        data_bindings_[element_id][data_key] = std::forward<T>(data);
    }

    /**
     * @brief 刷新绑定的数据（驱动 UI 重新渲染）
     */
    void RefreshBindings();

    /**
     * @brief 清除所有数据绑定
     */
    void ClearBindings();

    // ========================================================================
    // 【交互处理】
    // ========================================================================
    void HandleMouseMove(float mx, float my);
    void HandleMouseClick(float mx, float my);
    bool HandleKeyPressed(const sf::Event::KeyPressed& key);

    /**
     * @brief 设置手柄焦点导航
     */
    void SetFocusNavigation(const std::string& from_id,
                           const std::string& to_id,
                           const std::string& direction);
    void NavigateFocus(const std::string& direction);

    // ========================================================================
    // 【更新】
    // ========================================================================
    void Update(float delta_seconds);

    // ========================================================================
    // 【渲染】
    // ========================================================================
    void Render(sf::RenderWindow& window);
    sf::FloatRect GetPageRect() const;

protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    void ComputeLayout_();
    void UpdateAnimation_(float delta_seconds);
    void RebuildGeometry_();
    void HandleElementHover_(const std::string& element_id);
    void HandleElementClick_(const std::string& element_id);

    UiPageConfig config_;
    bool open_ = false;

    // 元素字典
    std::unordered_map<std::string, UiElement> elements_;
    // 根级元素顺序
    std::vector<std::string> root_ids_;

    // 资产管理器引用
    infrastructure::SpriteAssetManager* asset_manager_ = nullptr;
    PixelFontRenderer* font_renderer_ = nullptr;

    // 数据绑定
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> data_bindings_;

    // 焦点导航
    std::unordered_map<std::string,
        std::unordered_map<std::string, std::string>> focus_navigation_; // from_id -> direction -> to_id

    // 当前焦点元素
    std::string focused_element_id_;

    // 动画
    enum class AnimationState : std::uint8_t { Idle, FadeIn, FadeOut };
    AnimationState anim_state_ = AnimationState::Idle;
    float anim_elapsed_ = 0.0f;
    float anim_duration_ = 0.15f;
    float alpha_ = 1.0f;

    // 几何缓存
    mutable sf::VertexArray bg_vertices_;
    mutable sf::VertexArray border_vertices_;
    mutable std::unordered_map<std::string, sf::VertexArray> element_vertices_;
    mutable bool geometry_dirty_ = true;

    // 位置缓存
    sf::FloatRect page_rect_;
};

// ============================================================================
// 【UiPageManager】UI 页面管理器
// ============================================================================
// 持有所有 UiPage 实例，管理页面开关逻辑
class UiPageManager {
public:
    UiPageManager();

    /**
     * @brief 绑定依赖
     */
    void SetAssetManager(infrastructure::SpriteAssetManager* mgr);
    void SetFontRenderer(PixelFontRenderer* renderer);

    // ========================================================================
    // 【页面注册】
    // ========================================================================

    /**
     * @brief 注册页面（从配置文件加载）
     * @param page_config 页面配置
     * @return 注册的页面指针
     */
    UiPage* RegisterPage(const UiPageConfig& page_config);

    /**
     * @brief 注册页面（手动构造）
     */
    UiPage* RegisterPage(const std::string& page_id, std::unique_ptr<UiPage> page);

    /**
     * @brief 批量从目录加载所有页面配置
     */
    int LoadPagesFromDirectory(const std::string& pages_dir);

    // ========================================================================
    // 【页面操作】
    // ========================================================================

    /** 打开页面 */
    void OpenPage(const std::string& page_id);

    /** 关闭页面 */
    void ClosePage(const std::string& page_id);

    /** 切换页面 */
    void TogglePage(const std::string& page_id);

    /** 关闭所有页面 */
    void CloseAllPages();

    /** 关闭所有页面（模态页除外） */
    void CloseNonModalPages();

    // ========================================================================
    // 【查询】
    // ========================================================================
    [[nodiscard]] UiPage* GetPage(const std::string& page_id);
    [[nodiscard]] const UiPage* GetPage(const std::string& page_id) const;
    [[nodiscard]] bool IsPageOpen(const std::string& page_id) const;
    [[nodiscard]] std::vector<std::string> GetOpenPageIds() const;
    [[nodiscard]] bool HasOpenPage() const;

    // ========================================================================
    // 【更新】
    // ========================================================================
    void Update(float delta_seconds);

    // ========================================================================
    // 【渲染】
    // ========================================================================
    void Render(sf::RenderWindow& window);

    // ========================================================================
    // 【全局事件】
    // ========================================================================
    void HandleMouseMove(float mx, float my);
    void HandleMouseClick(float mx, float my);
    bool HandleKeyPressed(const sf::Event::KeyPressed& key);

    // ========================================================================
    // 【页面访问器】
    // ========================================================================
    // 返回所有已注册页面（用于迭代）
    [[nodiscard]] auto GetAllPages() -> std::vector<UiPage*>;

private:
    std::unordered_map<std::string, std::unique_ptr<UiPage>> pages_;
    infrastructure::SpriteAssetManager* asset_manager_ = nullptr;
    PixelFontRenderer* font_renderer_ = nullptr;

    // 页面关闭队列（用于延迟关闭，避免动画中途关闭问题）
    std::vector<std::string> pending_close_;
};

// ============================================================================
// 【UiLayoutSerializer】页面配置序列化
// ============================================================================
// 将 UiPageConfig / UiElement 序列化为 .ui.json
// 同时提供反序列化——从 .ui.json 加载页面
class UiLayoutSerializer {
public:
    // ========================================================================
    // 【序列化】
    // ========================================================================

    /**
     * @brief 将页面配置序列化为 JSON 字符串
     */
    static std::string SerializePage(const UiPageConfig& config);

    /**
     * @brief 将页面配置写入 .ui.json 文件
     */
    static bool WritePageToFile(const std::string& path, const UiPageConfig& config);

    // ========================================================================
    // 【反序列化】
    // ========================================================================

    /**
     * @brief 从 JSON 字符串解析页面配置
     */
    static std::optional<UiPageConfig> ParsePage(const std::string& json);

    /**
     * @brief 从 .ui.json 文件加载页面配置
     */
    static std::optional<UiPageConfig> LoadPageFromFile(const std::string& path);

    /**
     * @brief 扫描目录加载所有 .ui.json 文件
     */
    static std::vector<UiPageConfig> ScanPagesFromDirectory(const std::string& dir);

    // ========================================================================
    // 【编辑器支持】
    // ========================================================================

    /**
     * @brief 生成空页面模板
     */
    static UiPageConfig CreateEmptyPage(const std::string& page_id);

    /**
     * @brief 从现有页面生成 UI JSON 模板（用于编辑器导出）
     */
    static std::string ExportPageAsJson(const UiPage* page);
};

}  // namespace CloudSeamanor::engine
