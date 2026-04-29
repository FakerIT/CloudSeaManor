// ============================================================================
// 【SpriteRenderer】精灵渲染器实现
// ============================================================================

#include "CloudSeamanor/engine/SpriteRenderer.hpp"

#include <cmath>

namespace CloudSeamanor::engine {

// ============================================================================
// Sprite
// ============================================================================

Sprite::Sprite() = default;

Sprite::Sprite(const infrastructure::SpriteAssetDesc& desc, const sf::Texture* texture) {
    SetSprite(desc, texture);
}

void Sprite::SetSprite(const infrastructure::SpriteAssetDesc& desc, const sf::Texture* texture) {
    asset_id_ = desc.id;
    texture_ = texture;
    texture_region_ = desc.region;
    size_ = desc.size;
    sprite_.setTexture(*texture);
    sprite_.setTextureRect(desc.region);
    UpdateLocalBounds_();
}

void Sprite::SetTextureRegion(const sf::IntRect& region, const sf::Texture* texture) {
    texture_ = texture;
    texture_region_ = region;
    size_ = sf::Vector2u(static_cast<unsigned>(region.width),
                          static_cast<unsigned>(region.height));
    sprite_.setTexture(*texture, true);
    sprite_.setTextureRect(region);
    UpdateLocalBounds_();
}

void Sprite::ApplyParams(const SpriteDrawParams& params) {
    // 位置
    setPosition(params.position);

    // 缩放
    setScale(params.scale, params.scale);

    // 旋转
    setRotation(params.rotation * 180.0f / 3.14159265358979f);  // rad → deg

    // 颜色
    sprite_.setColor(params.color);

    // 翻转
    SetFlipX(params.flip_x);

    // 透明度
    alpha_ = params.alpha;
    if (params.alpha < 1.0f) {
        auto c = sprite_.getColor();
        c.a = static_cast<std::uint8_t>(params.alpha * 255.0f);
        sprite_.setColor(c);
    }
}

void Sprite::SetFlipX(bool flip) {
    // 翻转通过缩放实现
    auto s = getScale();
    setScale(flip ? -std::abs(s.x) : std::abs(s.x), s.y);
}

void Sprite::SetBottomCenter(const sf::Vector2f& world_pos) {
    // 底部对齐: y = world_pos.y - 高度
    setPosition(world_pos.x, world_pos.y - static_cast<float>(size_.y) * getScale().y);
}

void Sprite::UpdateLocalBounds_() {
    local_bounds_ = sf::FloatRect(0.0f, 0.0f,
                                    static_cast<float>(texture_region_.width),
                                    static_cast<float>(texture_region_.height));
}

sf::FloatRect Sprite::GetWorldBounds() const {
    auto bounds = getTransform().transformRect(local_bounds_);
    return bounds;
}

void Sprite::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    target.draw(sprite_, states);
}

// ============================================================================
// SpriteAnimator
// ============================================================================

SpriteAnimator::SpriteAnimator(infrastructure::SpriteAssetManager* asset_manager)
    : asset_manager_(asset_manager) {}

void SpriteAnimator::BindSprite(Sprite* sprite) {
    sprite_ = sprite;
}

void SpriteAnimator::BindTexture(const sf::Texture* texture) {
    texture_ = texture;
}

void SpriteAnimator::Play(const std::string& atlas_id, const std::string& anim_id) {
    if (!asset_manager_ || !sprite_) return;

    auto anim_opt = asset_manager_->GetAnimation(atlas_id, anim_id);
    if (!anim_opt) {
        // 动画不存在，尝试播放第一帧的静态精灵
        auto sprite_opt = asset_manager_->GetSprite(atlas_id + "_" + anim_id);
        if (sprite_opt) {
            sprite_->SetSprite(*sprite_opt, asset_manager_->GetAtlasTexture(atlas_id));
        }
        return;
    }

    const auto& anim = *anim_opt;
    state_.atlas_id = atlas_id;
    state_.anim_id = anim.id;
    state_.current_frame = 0;
    state_.elapsed_ms = 0.0f;
    state_.speed = anim.speed;
    state_.loop = anim.loop;
    state_.finished = false;
    playing_ = true;

    // 立即显示第一帧
    AdvanceFrame_();
}

void SpriteAnimator::Stop() {
    playing_ = false;
    state_.finished = false;
    state_.current_frame = 0;
}

void SpriteAnimator::Pause() {
    playing_ = false;
}

void SpriteAnimator::Resume() {
    if (!state_.finished) playing_ = true;
}

void SpriteAnimator::Update(float delta_ms) {
    if (!playing_ || !asset_manager_ || !sprite_) return;

    state_.elapsed_ms += delta_ms * state_.speed;

    // 获取当前动画
    auto anim_opt = asset_manager_->GetAnimation(state_.atlas_id, state_.anim_id);
    if (!anim_opt) return;

    const auto& anim = *anim_opt;
    if (state_.current_frame >= anim.frames.size()) return;

    const auto& frame = anim.frames[state_.current_frame];
    if (state_.elapsed_ms >= static_cast<float>(frame.duration_ms)) {
        state_.elapsed_ms -= static_cast<float>(frame.duration_ms);
        AdvanceFrame_();
    }
}

void SpriteAnimator::AdvanceFrame_() {
    if (!asset_manager_ || !sprite_) return;

    auto anim_opt = asset_manager_->GetAnimation(state_.atlas_id, state_.anim_id);
    if (!anim_opt) return;

    const auto& anim = *anim_opt;
    if (state_.current_frame >= anim.frames.size()) {
        if (state_.loop) {
            state_.current_frame = 0;
        } else {
            state_.finished = true;
            playing_ = false;
        }
        return;
    }

    const auto& frame_ref = anim.frames[state_.current_frame];
    auto sprite_opt = asset_manager_->GetSprite(state_.atlas_id + "_" + frame_ref.id);
    if (sprite_opt) {
        sprite_->SetSprite(*sprite_opt, texture_);
    }
}

std::size_t SpriteAnimator::TotalFrames() const {
    if (!asset_manager_) return 0;
    auto anim_opt = asset_manager_->GetAnimation(state_.atlas_id, state_.anim_id);
    return anim_opt ? anim_opt->frames.size() : 0;
}

std::string SpriteAnimator::CurrentFrameId() const {
    if (!asset_manager_) return "";
    auto anim_opt = asset_manager_->GetAnimation(state_.atlas_id, state_.anim_id);
    if (!anim_opt) return "";
    if (state_.current_frame >= anim_opt->frames.size()) return "";
    return state_.atlas_id + "_" + anim_opt->frames[state_.current_frame].id;
}

void SpriteAnimator::Play8Direction(const std::string& base_anim_id, int direction) {
    std::string suffix = SpriteDirectionHelper::DirectionSuffix(
        SpriteDirectionHelper::Normalize8(direction));
    std::string full_anim_id = base_anim_id + suffix;
    Play(state_.atlas_id, full_anim_id);
}

// ============================================================================
// SpriteDirectionHelper
// ============================================================================

int SpriteDirectionHelper::Normalize8(int dir4) {
    // 4 方向索引: 0=下, 1=右, 2=上, 3=左
    // 8 方向: 0=下, 2=右, 4=上, 6=左
    switch (dir4 & 0x3) {
        case 0: return 0;  // 下
        case 1: return 2;  // 右
        case 2: return 4;  // 上
        case 3: return 6;  // 左
    }
    return 0;
}

int SpriteDirectionHelper::VecToDirection(const sf::Vector2f& dir) {
    float x = dir.x;
    float y = dir.y;
    float len = std::sqrt(x * x + y * y);
    if (len < 0.001f) return 0;

    x /= len;
    y /= len;

    // 判断 8 方向
    if (y >= 0.707f) return 0;   // 下
    if (y <= -0.707f) return 4;  // 上
    if (x >= 0.707f) return 2;   // 右
    if (x <= -0.707f) return 6;  // 左

    // 4 个斜方向
    if (y > 0 && x > 0) return 1;  // 右下
    if (y > 0 && x < 0) return 7;  // 左下
    if (y < 0 && x > 0) return 3;  // 右上
    return 5;                         // 左上
}

const char* SpriteDirectionHelper::DirectionSuffix(int dir8) {
    dir8 = ((dir8 % 8) + 8) % 8;  // 归一化到 0-7
    return kDirSuffixes[dir8];
}

// ============================================================================
// SpriteBatchRenderer
// ============================================================================

SpriteBatchRenderer::SpriteBatchRenderer(const sf::Texture* texture)
    : texture_(texture), vertices_(sf::PrimitiveType::Quads) {}

void SpriteBatchRenderer::Clear() {
    vertices_.clear();
    pending_count_ = 0;
    built_ = false;
}

void SpriteBatchRenderer::Add(const sf::IntRect& region,
                               const sf::Vector2f& position,
                               const sf::Color& color,
                               float scale,
                               bool flip_x) {
    (void)scale;  // 缩放通过传入 position 控制

    float w = static_cast<float>(region.width);
    float h = static_cast<float>(region.height);
    float x = position.x;
    float y = position.y;

    if (flip_x) {
        // 水平翻转
        vertices_.append(sf::Vertex({x + w, y}, color, {region.left + w, region.top}));
        vertices_.append(sf::Vertex({x, y}, color, {region.left, region.top}));
        vertices_.append(sf::Vertex({x, y + h}, color, {region.left, region.top + h}));
        vertices_.append(sf::Vertex({x + w, y + h}, color, {region.left + w, region.top + h}));
    } else {
        vertices_.append(sf::Vertex({x, y}, color, {region.left, region.top}));
        vertices_.append(sf::Vertex({x + w, y}, color, {region.left + w, region.top}));
        vertices_.append(sf::Vertex({x + w, y + h}, color, {region.left + w, region.top + h}));
        vertices_.append(sf::Vertex({x, y + h}, color, {region.left, region.top + h}));
    }

    pending_count_++;
}

void SpriteBatchRenderer::Build() {
    built_ = true;
}

void SpriteBatchRenderer::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!built_ || pending_count_ == 0) return;
    states.texture = texture_;
    target.draw(vertices_, states);
}

}  // namespace CloudSeamanor::engine
