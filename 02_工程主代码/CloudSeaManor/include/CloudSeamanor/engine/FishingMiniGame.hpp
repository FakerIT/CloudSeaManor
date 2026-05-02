#pragma once

#include <string>
#include <functional>
#include "SFML/Graphics.hpp"

namespace CloudSeamanor::engine {

// ============================================================================
// 【FishingMiniGame】云海钓鱼小游戏
// ============================================================================
class FishingMiniGame {
public:
    // ========================================================================
    // 状态枚举
    // ========================================================================
    enum class State {
        Inactive,      // 未激活
        Casting,       // 抛竿中
        Waiting,       // 等待鱼上钩
        Biting,        // 鱼咬钩
        Reeling,       // 收竿中
        Success,        // 成功
        Failed          // 失败
    };

    // ========================================================================
    // 回调定义
    // ========================================================================
    using OnCompleteCallback = std::function<void(bool success, const std::string& fish_id)>;

    // ========================================================================
    // 构造与初始化
    // ========================================================================
    FishingMiniGame();
    void Initialize(const sf::Vector2f& position, float width, float height);

    // ========================================================================
    // 状态控制
    // ========================================================================
    void Start(int fish_difficulty);
    void Update(float delta_seconds);
    void HandleInput(const sf::Event& event);
    void Render(sf::RenderTarget& target);

    bool IsActive() const { return state_ != State::Inactive; }
    State GetState() const { return state_; }

    void SetOnComplete(OnCompleteCallback callback) { on_complete_ = callback; }

private:
    // ========================================================================
    // 内部状态
    // ========================================================================
    State state_ = State::Inactive;
    float state_timer_ = 0.0f;

    // 钓鱼参数
    int fish_difficulty_ = 1;
    float catch_chance_ = 0.5f;

    // UI 位置
    sf::Vector2f position_;
    float width_;
    float height_;

    // 进度条
    float fish_position_ = 0.5f;      // 0.0 - 1.0
    float player_position_ = 0.5f;    // 0.0 - 1.0
    float player_velocity_ = 0.0f;
    bool pressing_left_ = false;
    bool pressing_right_ = false;

    // 目标区域
    float target_min_ = 0.35f;
    float target_max_ = 0.65f;

    // 难度区域宽度（随难度变化）
    float GetTargetWidth() const;

    // 回调
    OnCompleteCallback on_complete_;

    // ========================================================================
    // 辅助方法
    // ========================================================================
    void UpdateCasting(float delta_seconds);
    void UpdateWaiting(float delta_seconds);
    void UpdateBiting(float delta_seconds);
    void UpdateReeling(float delta_seconds);

    void RenderBackground(sf::RenderTarget& target);
    void RenderProgressBar(sf::RenderTarget& target);
    void RenderFish(sf::RenderTarget& target);
    void RenderTarget(sf::RenderTarget& target);
    void RenderInstructions(sf::RenderTarget& target);
    void RenderResult(sf::RenderTarget& target);

    void CheckSuccess();
    void Complete(bool success);

    sf::Color GetFishColor() const;
};

// ============================================================================
// 【FishingUI】钓鱼UI管理器
// ============================================================================
class FishingUI {
public:
    static FishingUI& Instance();

    void ShowFishingSpot(const sf::Vector2f& world_pos);
    void HideFishingSpot();
    void StartFishing(int difficulty);
    void Update(float delta_seconds);
    void HandleInput(const sf::Event& event);
    void Render(sf::RenderTarget& target);

    bool IsFishing() const { return mini_game_.IsActive(); }

    void SetOnCatchCallback(FishingMiniGame::OnCompleteCallback callback) {
        mini_game_.SetOnComplete(callback);
    }

private:
    FishingUI() = default;
    ~FishingUI() = default;
    FishingUI(const FishingUI&) = delete;
    FishingUI& operator=(const FishingUI&) = delete;

    FishingMiniGame mini_game_;
    sf::Vector2f fishing_spot_position_;
    bool show_spot_ = false;
};

}  // namespace CloudSeamanor::engine
