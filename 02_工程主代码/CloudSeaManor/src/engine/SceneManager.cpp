#include "CloudSeamanor/engine/SceneManager.hpp"

#include <algorithm>

namespace CloudSeamanor::engine {

namespace {
float Clamp01_(float v) { return std::clamp(v, 0.0f, 1.0f); }
} // namespace

Scene* SceneManager::Top() const { return scenes_.empty() ? nullptr : scenes_.back().get(); }
Scene& SceneManager::TopRef() const { return *Top(); }
void SceneManager::Pop() {
    if (scenes_.empty()) return;
    scenes_.back()->OnExit();
    scenes_.pop_back();
    if (!scenes_.empty()) scenes_.back()->OnResume();
}
void SceneManager::Pop(std::size_t count) { for (std::size_t i = 0; i < count && !scenes_.empty(); ++i) Pop(); }
void SceneManager::Clear() {
    while (!scenes_.empty()) {
        scenes_.back()->OnExit();
        scenes_.pop_back();
    }
}
void SceneManager::Update(float delta) {
    if (scenes_.empty()) return;
    Scene& top = *scenes_.back();
    if (top.BlocksUpdate()) top.Update(delta);
}
void SceneManager::Render(sf::RenderWindow& window) {
    if (scenes_.empty()) return;
    Scene& top = *scenes_.back();
    if (top.BlocksRender()) {
        for (std::size_t i = 0; i + 1 < scenes_.size(); ++i) {
            if (!scenes_[i]->BlocksRender()) {
                scenes_[i]->Render(window);
            }
        }
        top.Render(window);
        return;
    }

    for (const auto& scene : scenes_) {
        scene->Render(window);
    }
}

void SceneTransition::Start(
    float fade_out_seconds,
    float fade_in_seconds,
    std::function<void()> on_midpoint,
    std::function<void()> on_complete
) {
    fade_out_seconds_ = std::max(0.01f, fade_out_seconds);
    fade_in_seconds_ = std::max(0.01f, fade_in_seconds);
    on_midpoint_ = std::move(on_midpoint);
    on_complete_ = std::move(on_complete);

    state_ = State::FadeOut;
    alpha_ = 0.0f;
    timer_ = 0.0f;
    midpoint_fired_ = false;
}

void SceneTransition::Reset() {
    state_ = State::Idle;
    alpha_ = 0.0f;
    timer_ = 0.0f;
    midpoint_fired_ = false;
    on_midpoint_ = nullptr;
    on_complete_ = nullptr;
}

void SceneTransition::Update(float delta_seconds) {
    if (state_ == State::Idle) {
        return;
    }

    timer_ += std::max(0.0f, delta_seconds);

    if (state_ == State::FadeOut) {
        alpha_ = Clamp01_(timer_ / fade_out_seconds_);
        if (alpha_ >= 1.0f) {
            if (!midpoint_fired_) {
                midpoint_fired_ = true;
                if (on_midpoint_) {
                    on_midpoint_();
                }
            }
            state_ = State::FadeIn;
            timer_ = 0.0f;
        }
        return;
    }

    if (state_ == State::FadeIn) {
        alpha_ = 1.0f - Clamp01_(timer_ / fade_in_seconds_);
        if (timer_ >= fade_in_seconds_) {
            if (on_complete_) {
                on_complete_();
            }
            Reset();
        }
        return;
    }
}

void SceneTransition::Render(sf::RenderWindow& window) const {
    if (state_ == State::Idle) {
        return;
    }
    const auto size = window.getSize();
    sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
    overlay.setPosition({0.0f, 0.0f});
    overlay.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(Clamp01_(alpha_) * 255.0f)));
    window.draw(overlay);
}
}  // namespace CloudSeamanor::engine