#pragma once

// ============================================================================
// 【SpriteRenderer】精灵渲染器
// ============================================================================
// Responsibilities:
// - 提供高性能精灵绘制接口（对应 SpriteAssetManager 的资源）
// - 支持静态精灵 + 动画状态机
// - 与现有 WorldRenderer / PixelGameHud 集成
// - 提供 SFML Sprite 对象复用（减少 draw call）
//
// 设计原则:
// - 不持有纹理（纹理归 SpriteAssetManager 管理）
// - 不持有游戏逻辑（逻辑归各系统管理）
// - 仅负责渲染：接收 SpriteAssetDesc + 当前状态 → draw
//
// 性能目标:
// - 每帧最多 1 个 SFML draw call per atlas（通过 sf::Sprite 复用）
// - 支持硬件实例化绘制（sf::VertexArray + texture）将来可扩展
// ============================================================================

#include "CloudSeamanor/SpriteAssetManager.hpp"

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/System/Vector2.hpp>

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::engine {

// ============================================================================
// 【SpriteDrawParams】精灵绘制参数
// ============================================================================
struct SpriteDrawParams {
    sf::Vector2f position = {0.0f, 0.0f};   // 世界坐标（底部对齐）
    float scale = 1.0f;                       // 缩放
    float rotation = 0.0f;                    // 旋转（弧度）
    sf::Color color = sf::Color::White;        // 颜色叠加
    bool flip_x = false;                      // 水平翻转
    bool flip_y = false;                      // 垂直翻转
    sf::IntRect custom_region;                 // 自定义区域（覆盖图集区域）
    float alpha = 1.0f;                       // 透明度
};

// ============================================================================
// 【SpriteAnimatorState】动画状态
// ============================================================================
struct SpriteAnimatorState {
    std::string atlas_id;          // 图集 ID
    std::string anim_id;           // 动画 ID
    std::size_t current_frame = 0;
    float elapsed_ms = 0.0f;
    float speed = 1.0f;
    bool finished = false;
    bool loop = true;
};

// ============================================================================
// 【Sprite】单个可绘制精灵
// ============================================================================
// 持有 SFML Sprite 对象 + 关联的资产信息
// 可独立使用，也可交给 SpriteBatchRenderer 批量绘制
class Sprite : public sf::Drawable, public sf::Transformable {
public:
    // ========================================================================
    // 【构造】
    // ========================================================================
    Sprite();
    Sprite(const infrastructure::SpriteAssetDesc& desc, const sf::Texture* texture);

    // ========================================================================
    // 【静态绘制】
    // ========================================================================

    /**
     * @brief 设置精灵帧
     * @param desc 精灵描述（来自 SpriteAssetManager）
     * @param texture 纹理指针（来自 SpriteAssetManager::GetAtlasTexture）
     */
    void SetSprite(const infrastructure::SpriteAssetDesc& desc, const sf::Texture* texture);

    /**
     * @brief 直接设置纹理区域（不经过资产管理器）
     */
    void SetTextureRegion(const sf::IntRect& region, const sf::Texture* texture);

    /**
     * @brief 设置绘制参数
     */
    void ApplyParams(const SpriteDrawParams& params);

    // ========================================================================
    // 【状态】
    // ========================================================================
    [[nodiscard]] const sf::FloatRect& GetLocalBounds() const { return local_bounds_; }
    [[nodiscard]] sf::FloatRect GetWorldBounds() const;
    [[nodiscard]] const sf::Vector2u& GetSize() const { return size_; }
    [[nodiscard]] const std::string& GetAssetId() const { return asset_id_; }

    // ========================================================================
    // 【便捷操作】
    // ========================================================================

    /** 水平翻转 */
    void SetFlipX(bool flip);

    /** 居中到底部对齐 */
    void SetBottomCenter(const sf::Vector2f& world_pos);

protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    void UpdateLocalBounds_();

    sf::Sprite sprite_;
    sf::Texture const* texture_ = nullptr;
    sf::IntRect texture_region_;
    std::string asset_id_;
    sf::Vector2u size_{0, 0};
    sf::FloatRect local_bounds_;
};

// ============================================================================
// 【SpriteAnimator】精灵动画状态机
// ============================================================================
// 封装动画播放逻辑
// 持有指向 SpriteAssetManager 的引用来查询动画元数据
class SpriteAnimator {
public:
    explicit SpriteAnimator(infrastructure::SpriteAssetManager* asset_manager);

    // ========================================================================
    // 【配置】
    // ========================================================================

    /**
     * @brief 绑定渲染用的 Sprite 对象
     */
    void BindSprite(Sprite* sprite);

    /**
     * @brief 绑定纹理（通过资产管理器查询）
     */
    void BindTexture(const sf::Texture* texture);

    // ========================================================================
    // 【播放控制】
    // ========================================================================

    /**
     * @brief 播放动画
     * @param atlas_id 图集 ID
     * @param anim_id 动画 ID（从 .atlas.json 的 animations 数组中读取 id 字段）
     */
    void Play(const std::string& atlas_id, const std::string& anim_id);

    /**
     * @brief 停止并重置
     */
    void Stop();

    /**
     * @brief 暂停
     */
    void Pause();

    /**
     * @brief 恢复
     */
    void Resume();

    // ========================================================================
    // 【更新】
    // ========================================================================

    /**
     * @brief 每帧更新（驱动动画帧切换）
     * @param delta_ms 帧时间（毫秒）
     */
    void Update(float delta_ms);

    // ========================================================================
    // 【状态查询】
    // ========================================================================
    [[nodiscard]] bool IsPlaying() const { return playing_; }
    [[nodiscard]] bool IsFinished() const { return state_.finished; }
    [[nodiscard]] const std::string& CurrentAnimId() const { return state_.anim_id; }
    [[nodiscard]] std::size_t CurrentFrame() const { return state_.current_frame; }
    [[nodiscard]] std::size_t TotalFrames() const;

    /**
     * @brief 获取当前帧在图集中的帧 ID
     */
    [[nodiscard]] std::string CurrentFrameId() const;

    // ========================================================================
    // 【8 方向快捷方法】
    // ========================================================================

    /**
     * @brief 8 方向动画播放（辅助方法）
     * @param base_anim_id 基础动画 ID，如 "player_idle"
     * @param direction 方向 0-7（0=下, 1=右下, 2=右, 3=右上, 4=上, 5=左上, 6=左, 7=左下）
     */
    void Play8Direction(const std::string& base_anim_id, int direction);

private:
    void AdvanceFrame_();

    infrastructure::SpriteAssetManager* asset_manager_ = nullptr;
    Sprite* sprite_ = nullptr;
    const sf::Texture* texture_ = nullptr;
    bool playing_ = false;
    SpriteAnimatorState state_;
};

// ============================================================================
// 【SpriteBatchRenderer】精灵批量渲染器（高性能）
// ============================================================================
// 使用 sf::VertexArray + 单次 draw 调用渲染同图集的所有精灵
// 适用于大量同类精灵（作物、粒子、地板瓦片）
class SpriteBatchRenderer : public sf::Drawable {
public:
    explicit SpriteBatchRenderer(const sf::Texture* texture);

    // ========================================================================
    // 【操作】
    // ========================================================================

    /** 清空所有批次精灵 */
    void Clear();

    /** 添加精灵到批次（不立即绘制，累积后一次 draw） */
    void Add(const sf::IntRect& region,
             const sf::Vector2f& position,
             const sf::Color& color = sf::Color::White,
             float scale = 1.0f,
             bool flip_x = false);

    /** 提交并构建顶点数组（Add 后必须调用） */
    void Build();

    /** 估算批次数量 */
    [[nodiscard]] std::size_t Count() const { return pending_count_; }

protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    const sf::Texture* texture_ = nullptr;
    mutable sf::VertexArray vertices_{sf::PrimitiveType::Triangles};
    std::size_t pending_count_ = 0;
    mutable bool built_ = false;
};

// ============================================================================
// 【SpriteDirectionHelper】方向工具
// ============================================================================
struct SpriteDirectionHelper {
    // 将 4 方向转为 8 方向
    static int Normalize8(int dir4);

    // 将向量转为 8 方向索引
    static int VecToDirection(const sf::Vector2f& dir);

    // 方向索引到动画后缀
    static const char* DirectionSuffix(int dir8);

    // 8 方向后缀表
    static constexpr const char* kDirSuffixes[8] = {
        "_down", "_down_right", "_right", "_up_right",
        "_up", "_up_left", "_left", "_down_left"
    };

    static constexpr const char* kDir4Suffixes[4] = {
        "_down", "_right", "_up", "_left"
    };
};

}  // namespace CloudSeamanor::engine
