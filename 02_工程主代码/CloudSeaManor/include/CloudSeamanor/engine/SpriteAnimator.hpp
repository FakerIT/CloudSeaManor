#pragma once
// 【SpriteAnimator】精灵动画器
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <string>
#include <unordered_map>
#include <vector>
namespace CloudSeamanor::engine::animation {

struct AnimationFrame {
    sf::IntRect rect;
    float duration_seconds = 0.1f;
};

struct Animation {
    std::string name;
    std::vector<AnimationFrame> frames;
    bool loop = true;
    float speed_multiplier = 1.0f;
};

class SpriteAnimator {
public:
    void LoadSpritesheet(const sf::Texture& texture, int frame_width, int frame_height);
    void AddAnimation(const std::string& name, const Animation& anim);
    void AddAnimation(const std::string& name, int col, int row, int frame_count, float duration, bool loop=true);
    void Play(const std::string& name);
    void Resume(const std::string& name);
    void Stop();
    void Pause();
    void SetSpeed(float multiplier);
    void Update(float delta_seconds);
    [[nodiscard]] const sf::IntRect& CurrentRect() const;
    [[nodiscard]] bool IsPlaying() const { return playing_; }
    [[nodiscard]] const std::string& CurrentAnimation() const { return current_name_; }
    [[nodiscard]] std::size_t CurrentFrameIndex() const { return current_frame_index_; }
    [[nodiscard]] std::size_t TotalFrames() const;
    [[nodiscard]] bool IsFinished() const { return finished_; }
    void ApplyTo(sf::Sprite& sprite) const;
private:
    const sf::Texture* spritesheet_ = nullptr;
    int frame_width_ = 48;
    int frame_height_ = 48;
    std::unordered_map<std::string, Animation> animations_;
    std::string current_name_;
    Animation* current_anim_ = nullptr;
    std::size_t current_frame_index_ = 0;
    float frame_timer_ = 0.0f;
    bool playing_ = false;
    bool paused_ = false;
    bool finished_ = false;
    float speed_multiplier_ = 1.0f;
};

SpriteAnimator Create8DirectionAnimator(const sf::Texture& texture, int frame_width=48, int frame_height=48);
SpriteAnimator Create4FrameAnimator(const sf::Texture& texture, int row=0, int frame_width=48, int frame_height=48);

}  // namespace CloudSeamanor::engine::animation
