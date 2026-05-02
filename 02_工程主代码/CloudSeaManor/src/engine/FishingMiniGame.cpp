#include "CloudSeamanor/engine/FishingMiniGame.hpp"
#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/GameConstants.hpp"
#include <cmath>

namespace CloudSeamanor::engine {

// ============================================================================
// FishingMiniGame 实现
// ============================================================================
FishingMiniGame::FishingMiniGame()
    : position_(0, 0), width_(400), height_(200) {
}

void FishingMiniGame::Initialize(const sf::Vector2f& position, float width, float height) {
    position_ = position;
    width_ = width;
    height_ = height;
}

void FishingMiniGame::Start(int fish_difficulty) {
    fish_difficulty_ = std::clamp(fish_difficulty, 1, 5);
    catch_chance_ = 0.5f + (5 - fish_difficulty_) * 0.1f;

    state_ = State::Casting;
    state_timer_ = 0.0f;

    fish_position_ = 0.5f;
    player_position_ = 0.5f;
    player_velocity_ = 0.0f;

    // 难度越高，目标区域越小
    float target_width = GetTargetWidth();
    target_min_ = 0.5f - target_width / 2.0f;
    target_max_ = 0.5f + target_width / 2.0f;
}

void FishingMiniGame::Update(float delta_seconds) {
    switch (state_) {
        case State::Casting:
            UpdateCasting(delta_seconds);
            break;
        case State::Waiting:
            UpdateWaiting(delta_seconds);
            break;
        case State::Biting:
            UpdateBiting(delta_seconds);
            break;
        case State::Reeling:
            UpdateReeling(delta_seconds);
            break;
        default:
            break;
    }

    state_timer_ += delta_seconds;
}

void FishingMiniGame::HandleInput(const sf::Event& event) {
    if (state_ == State::Inactive || state_ == State::Success || state_ == State::Failed) {
        return;
    }

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::A || event.key.code == sf::Keyboard::Left) {
            pressing_left_ = true;
        }
        if (event.key.code == sf::Keyboard::D || event.key.code == sf::Keyboard::Right) {
            pressing_right_ = true;
        }
    }

    if (event.type == sf::Event::KeyReleased) {
        if (event.key.code == sf::Keyboard::A || event.key.code == sf::Keyboard::Left) {
            pressing_left_ = false;
        }
        if (event.key.code == sf::Keyboard::D || event.key.code == sf::Keyboard::Right) {
            pressing_right_ = false;
        }
    }
}

void FishingMiniGame::Render(sf::RenderTarget& target) {
    if (state_ == State::Inactive) return;

    RenderBackground(target);
    RenderProgressBar(target);
    RenderTarget(target);
    RenderFish(target);
    RenderInstructions(target);

    if (state_ == State::Success || state_ == State::Failed) {
        RenderResult(target);
    }
}

float FishingMiniGame::GetTargetWidth() const {
    // 难度1: 0.4, 难度5: 0.15
    return 0.4f - (fish_difficulty_ - 1) * 0.0625f;
}

void FishingMiniGame::UpdateCasting(float delta_seconds) {
    if (state_timer_ >= 1.5f) {
        state_ = State::Waiting;
        state_timer_ = 0.0f;
    }
}

void FishingMiniGame::UpdateWaiting(float delta_seconds) {
    // 随机等待时间 1-4秒
    if (state_timer_ >= 2.0f + static_cast<float>(fish_difficulty_) * 0.5f) {
        state_ = State::Biting;
        state_timer_ = 0.0f;
    }
}

void FishingMiniGame::UpdateBiting(float delta_seconds) {
    // 鱼咬钩的时间窗口（难度越高越短）
    float bite_window = 3.0f - fish_difficulty_ * 0.4f;

    if (state_timer_ >= bite_window) {
        Complete(false);  // 超时失败
        return;
    }

    // 玩家移动
    const float move_speed = 0.6f;
    const float friction = 0.92f;

    if (pressing_left_) {
        player_velocity_ -= move_speed * delta_seconds;
    }
    if (pressing_right_) {
        player_velocity_ += move_speed * delta_seconds;
    }

    player_velocity_ *= friction;
    player_position_ += player_velocity_;

    // 边界限制
    player_position_ = std::clamp(player_position_, 0.0f, 1.0f);

    // 鱼移动
    float fish_speed = 0.2f + fish_difficulty_ * 0.1f;
    float random_offset = std::sin(state_timer_ * 5.0f) * 0.02f;
    fish_position_ += random_offset + (player_position_ - fish_position_) * fish_speed * delta_seconds;
    fish_position_ = std::clamp(fish_position_, 0.1f, 0.9f);

    // 检测成功
    if (pressing_left_ || pressing_right_) {
        CheckSuccess();
    }
}

void FishingMiniGame::UpdateReeling(float delta_seconds) {
    // 收竿动画
    if (state_timer_ >= 1.0f) {
        Complete(true);
    }
}

void FishingMiniGame::CheckSuccess() {
    // 检查玩家位置是否在目标区域内
    if (player_position_ >= target_min_ && player_position_ <= target_max_) {
        state_ = State::Reeling;
        state_timer_ = 0.0f;
    }
}

void FishingMiniGame::Complete(bool success) {
    state_ = success ? State::Success : State::Failed;

    if (on_complete_) {
        on_complete_(success, success ? "cloudCarp" : "");
    }
}

void FishingMiniGame::RenderBackground(sf::RenderTarget& target) {
    // 背景面板
    sf::RectangleShape bg;
    bg.setPosition(position_);
    bg.setSize({width_, height_});
    bg.setFillColor(sf::Color(20, 30, 50, 220));
    target.draw(bg);

    // 边框
    PixelArtStyle::DrawPixelButton(
        *target.draw_list(),
        {position_.x, position_.y, width_, height_},
        0
    );
}

void FishingMiniGame::RenderProgressBar(sf::RenderTarget& target) {
    const float bar_y = position_.y + 40.0f;
    const float bar_height = 40.0f;
    const float bar_margin = 20.0f;
    const float bar_width = width_ - bar_margin * 2;

    // 背景
    sf::RectangleShape bar_bg;
    bar_bg.setPosition({position_.x + bar_margin, bar_y});
    bar_bg.setSize({bar_width, bar_height});
    bar_bg.setFillColor(sf::Color(30, 40, 60));
    target.draw(bar_bg);
}

void FishingMiniGame::RenderTarget(sf::RenderTarget& target) {
    const float bar_y = position_.y + 40.0f;
    const float bar_height = 40.0f;
    const float bar_margin = 20.0f;
    const float bar_width = width_ - bar_margin * 2;

    // 目标区域
    float target_x = position_.x + bar_margin + target_min_ * bar_width;
    float target_w = (target_max_ - target_min_) * bar_width;

    sf::RectangleShape target_area;
    target_area.setPosition({target_x, bar_y});
    target_area.setSize({target_w, bar_height});
    target_area.setFillColor(sf::Color(80, 200, 120, 100));
    target_area.setOutlineThickness(2);
    target_area.setOutlineColor(sf::Color(100, 255, 150));
    target.draw(target_area);

    // 玩家指示器
    float player_x = position_.x + bar_margin + player_position_ * bar_width;
    sf::RectangleShape player_indicator;
    player_indicator.setPosition({player_x - 3.0f, bar_y - 5.0f});
    player_indicator.setSize({6.0f, bar_height + 10.0f});
    player_indicator.setFillColor(sf::Color(255, 220, 100));
    target.draw(player_indicator);
}

void FishingMiniGame::RenderFish(sf::RenderTarget& target) {
    const float bar_y = position_.y + 40.0f;
    const float bar_height = 40.0f;
    const float bar_margin = 20.0f;
    const float bar_width = width_ - bar_margin * 2;

    float fish_x = position_.x + bar_margin + fish_position_ * bar_width;
    float fish_y = bar_y + bar_height / 2.0f;

    // 鱼图标（用圆代替）
    sf::CircleShape fish(12.0f);
    fish.setPosition({fish_x - 12.0f, fish_y - 12.0f});
    fish.setFillColor(GetFishColor());
    target.draw(fish);

    // 鱼眼睛
    sf::CircleShape eye(3.0f);
    eye.setPosition({fish_x + 2.0f, fish_y - 4.0f});
    eye.setFillColor(sf::Color::White);
    target.draw(eye);
}

void FishingMiniGame::RenderInstructions(sf::RenderTarget& target) {
    sf::Text text;
    text.setFont(GameConstants::GetFont());
    text.setCharacterSize(16);

    std::string instruction;
    switch (state_) {
        case State::Casting:
            instruction = "抛竿中...";
            text.setFillColor(sf::Color(150, 180, 220));
            break;
        case State::Waiting:
            instruction = "等待鱼儿上钩...";
            text.setFillColor(sf::Color(150, 180, 220));
            break;
        case State::Biting:
            instruction = "鱼咬钩了！按 A/D 保持位置！";
            text.setFillColor(sf::Color(255, 220, 100));
            break;
        case State::Reeling:
            instruction = "收竿中...";
            text.setFillColor(sf::Color(100, 255, 150));
            break;
        default:
            return;
    }

    text.setString(instruction);

    sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin({bounds.size.x / 2.0f, bounds.size.y / 2.0f});
    text.setPosition({
        position_.x + width_ / 2.0f,
        position_.y + height_ - 30.0f
    });

    target.draw(text);
}

void FishingMiniGame::RenderResult(sf::RenderTarget& target) {
    // 半透明遮罩
    sf::RectangleShape overlay;
    overlay.setPosition(position_);
    overlay.setSize({width_, height_});
    overlay.setFillColor(sf::Color(0, 0, 0, 150));
    target.draw(overlay);

    sf::Text text;
    text.setFont(GameConstants::GetFont());
    text.setCharacterSize(24);

    if (state_ == State::Success) {
        text.setString("钓鱼成功！");
        text.setFillColor(sf::Color(100, 255, 150));
    } else {
        text.setString("鱼跑了...");
        text.setFillColor(sf::Color(255, 100, 100));
    }

    sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin({bounds.size.x / 2.0f, bounds.size.y / 2.0f});
    text.setPosition({
        position_.x + width_ / 2.0f,
        position_.y + height_ / 2.0f - 10.0f
    });

    target.draw(text);

    // 提示
    sf::Text hint;
    hint.setFont(GameConstants::GetFont());
    hint.setCharacterSize(14);
    hint.setString("按任意键继续");
    hint.setFillColor(sf::Color(150, 150, 150));

    sf::FloatRect hint_bounds = hint.getLocalBounds();
    hint.setOrigin({hint_bounds.size.x / 2.0f, hint_bounds.size.y / 2.0f});
    hint.setPosition({
        position_.x + width_ / 2.0f,
        position_.y + height_ / 2.0f + 30.0f
    });

    target.draw(hint);
}

sf::Color FishingMiniGame::GetFishColor() const {
    // 根据难度返回不同颜色
    switch (fish_difficulty_) {
        case 1: return sf::Color(100, 180, 220);  // 普通
        case 2: return sf::Color(120, 200, 140);  // 不错
        case 3: return sf::Color(180, 160, 220);  // 稀有
        case 4: return sf::Color(255, 200, 100);  // 传说
        case 5: return sf::Color(255, 100, 255);  // 太初
        default: return sf::Color(100, 180, 220);
    }
}

// ============================================================================
// FishingUI 实现
// ============================================================================
FishingUI& FishingUI::Instance() {
    static FishingUI instance;
    return instance;
}

void FishingUI::ShowFishingSpot(const sf::Vector2f& world_pos) {
    fishing_spot_position_ = world_pos;
    show_spot_ = true;
}

void FishingUI::HideFishingSpot() {
    show_spot_ = false;
}

void FishingUI::StartFishing(int difficulty) {
    sf::Vector2f screen_pos(400, 300);
    mini_game_.Initialize(screen_pos, 400, 200);
    mini_game_.Start(difficulty);
    show_spot_ = false;
}

void FishingUI::Update(float delta_seconds) {
    mini_game_.Update(delta_seconds);
}

void FishingUI::HandleInput(const sf::Event& event) {
    mini_game_.HandleInput(event);
}

void FishingUI::Render(sf::RenderTarget& target) {
    mini_game_.Render(target);
}

}  // namespace CloudSeamanor::engine
