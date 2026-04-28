#include "CloudSeamanor/PixelDialogueBox.hpp"

#include "CloudSeamanor/PixelFontRenderer.hpp"
#include "CloudSeamanor/UiVertexHelpers.hpp"

#include <SFML/Graphics/Sprite.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>

namespace CloudSeamanor::engine {

namespace {
using CloudSeamanor::engine::uivx::AddQuad;
}  // namespace

PixelDialogueBox::PixelDialogueBox()
    : rect_{DialogueBoxConfig::Position, DialogueBoxConfig::Size},
      panel_vertices_(sf::PrimitiveType::Triangles),
      avatar_vertices_(sf::PrimitiveType::Triangles) {
    RebuildGeometry_();
}

void PixelDialogueBox::SetAvatar(const sf::Texture* texture) {
    avatar_texture_ = texture;
    geometry_dirty_ = true;
}

void PixelDialogueBox::SetSpeakerName(const std::string& name) {
    speaker_name_ = name;
}

void PixelDialogueBox::SetText(const std::string& full_text) {
    full_text_ = full_text;
    current_text_.clear();
    typing_char_index_ = 0;
    typing_timer_ms_ = 0.0f;
    StartTyping_();
}

void PixelDialogueBox::SetChoices(const std::vector<DialogueChoice>& choices) {
    choices_ = choices;
    if (choices.empty()) {
        state_ = DialogueBoxState::WaitingConfirm;
    }
}

void PixelDialogueBox::Show() {
    visible_ = true;
    alpha_ = 0.0f;
    fade_timer_ = 0.0f;
}

void PixelDialogueBox::Hide() {
    visible_ = false;
    state_ = DialogueBoxState::Hidden;
    full_text_.clear();
    current_text_.clear();
    choices_.clear();
}

bool PixelDialogueBox::OnConfirm() {
    if (state_ == DialogueBoxState::Hidden) return false;

    if (state_ == DialogueBoxState::Typing) {
        SkipTyping_();
        return true;
    }

    if (state_ == DialogueBoxState::WaitingChoice) {
        return false;
    }

    if (state_ == DialogueBoxState::WaitingConfirm) {
        Hide();
        if (on_complete_) on_complete_();
        return true;
    }

    return false;
}

bool PixelDialogueBox::SelectChoice(std::size_t index) {
    if (state_ != DialogueBoxState::WaitingChoice) return false;
    if (index >= choices_.size()) return false;

    selected_choice_ = index;

    // 若绑定了 on_choice_（通常用于 DialogueEngine 驱动模式），则把选择交给外部处理。
    // 外部会推进对话状态，随后由 SyncFromDialogueEngine 刷新 UI；此处不主动 Hide，避免闪烁/状态不同步。
    if (on_choice_) {
        on_choice_(choices_[index].id);
        return true;
    }

    // standalone 模式：直接结束对话框。
    Hide();
    if (on_complete_) {
        on_complete_();
    }
    return true;
}

void PixelDialogueBox::HoverChoice(int index) {
    if (index >= 0 && index < static_cast<int>(choices_.size())) {
        hovered_choice_ = static_cast<std::size_t>(index);
    } else {
        hovered_choice_ = static_cast<std::size_t>(-1);
    }
}

void PixelDialogueBox::MoveChoiceHover(int delta) {
    if (state_ != DialogueBoxState::WaitingChoice) return;
    if (choices_.empty()) return;

    const int n = static_cast<int>(choices_.size());
    int cur = (hovered_choice_ == static_cast<std::size_t>(-1)) ? 0 : static_cast<int>(hovered_choice_);
    cur = (cur + delta) % n;
    if (cur < 0) cur += n;
    hovered_choice_ = static_cast<std::size_t>(cur);
}

bool PixelDialogueBox::ConfirmHoveredChoice() {
    if (state_ != DialogueBoxState::WaitingChoice) return false;
    if (choices_.empty()) return false;
    if (hovered_choice_ == static_cast<std::size_t>(-1)) {
        hovered_choice_ = 0;
    }
    return SelectChoice(hovered_choice_);
}

std::optional<std::size_t> PixelDialogueBox::OnMouseClick(float mx, float my) {
    if (!visible_ || !IsActive()) return std::nullopt;

    const float p = DialogueBoxConfig::TextPadding;
    const float av_size = DialogueBoxConfig::AvatarSize;
    const float y0 = rect_.position.y + rect_.size.y - DialogueBoxConfig::ChoiceAreaBottomOffset;

    if (state_ == DialogueBoxState::WaitingChoice && !choices_.empty()) {
        const float choice_h = DialogueBoxConfig::ChoiceRowHeight;
        for (std::size_t i = 0; i < choices_.size(); ++i) {
            const float cy = y0 + static_cast<float>(i) * choice_h;
            if (mx >= rect_.position.x + p + av_size && mx <= rect_.position.x + rect_.size.x - p &&
                my >= cy && my <= cy + choice_h) {
                SelectChoice(i);
                return i;
            }
        }
    }

    return std::nullopt;
}

void PixelDialogueBox::Update(float delta_seconds) {
    if (!visible_ || state_ == DialogueBoxState::Hidden) return;

    if (alpha_ < 1.0f) {
        fade_timer_ += delta_seconds;
        alpha_ = std::min(1.0f, fade_timer_ / AnimationConfig::FadeInDuration);
    }

    if (state_ == DialogueBoxState::Typing) {
        typing_timer_ms_ += delta_seconds * 1000.0f;
        while (typing_timer_ms_ >= typing_speed_ms_ && typing_char_index_ < full_text_.size()) {
            typing_timer_ms_ -= typing_speed_ms_;
            ++typing_char_index_;
            current_text_ = full_text_.substr(0, typing_char_index_);
        }
        if (typing_char_index_ >= full_text_.size()) {
            state_ = DialogueBoxState::WaitingChoice;
            if (choices_.empty()) {
                state_ = DialogueBoxState::WaitingConfirm;
            }
        }
    }
}

void PixelDialogueBox::Render(sf::RenderWindow& window) {
    if (!visible_ || alpha_ <= 0.0f) return;

    if (geometry_dirty_) {
        RebuildGeometry_();
        geometry_dirty_ = false;
    }

    sf::RenderStates alpha_states;
    alpha_states.blendMode = sf::BlendMode();
    window.draw(panel_vertices_, alpha_states);

    if (avatar_texture_ != nullptr) {
        sf::Sprite sprite(*avatar_texture_);
        const float avatar_size = DialogueBoxConfig::AvatarSize;
        const auto tex_size = avatar_texture_->getSize();
        const float sx = tex_size.x > 0 ? avatar_size / static_cast<float>(tex_size.x) : 1.0f;
        const float sy = tex_size.y > 0 ? avatar_size / static_cast<float>(tex_size.y) : 1.0f;
        const float scale = std::min(sx, sy);
        sprite.setScale({scale, scale});
        sprite.setTextureRect(sf::IntRect({0, 0},
                                          {static_cast<int>(std::min<unsigned>(tex_size.x, static_cast<unsigned>(avatar_size / std::max(scale, 1e-5f)))),
                                           static_cast<int>(std::min<unsigned>(tex_size.y, static_cast<unsigned>(avatar_size / std::max(scale, 1e-5f))))}));
        sprite.setPosition({rect_.position.x + DialogueBoxConfig::TextPadding,
                            rect_.position.y + DialogueBoxConfig::TextPadding});
        window.draw(sprite, alpha_states);
    }
}

void PixelDialogueBox::RebuildGeometry_() {
    panel_vertices_.clear();

    const sf::FloatRect r = rect_;

    AddQuad(panel_vertices_,
            {r.position.x, r.position.y},
            {r.position.x + r.size.x, r.position.y},
            {r.position.x + r.size.x, r.position.y + r.size.y},
            {r.position.x, r.position.y + r.size.y},
            panel_fill_color_, alpha_);

    const float cs = PixelBorderConfig::CornerBlockSize;
    const float t = PixelBorderConfig::BorderThickness;

    AddQuad(panel_vertices_,
            {r.position.x, r.position.y},
            {r.position.x + cs, r.position.y},
            {r.position.x + cs, r.position.y + cs},
            {r.position.x, r.position.y + cs},
            border_color_, alpha_);
    AddQuad(panel_vertices_,
            {r.position.x + r.size.x - cs, r.position.y},
            {r.position.x + r.size.x, r.position.y},
            {r.position.x + r.size.x, r.position.y + cs},
            {r.position.x + r.size.x - cs, r.position.y + cs},
            border_color_, alpha_);
    AddQuad(panel_vertices_,
            {r.position.x, r.position.y + r.size.y - cs},
            {r.position.x + cs, r.position.y + r.size.y - cs},
            {r.position.x + cs, r.position.y + r.size.y},
            {r.position.x, r.position.y + r.size.y},
            border_color_, alpha_);
    AddQuad(panel_vertices_,
            {r.position.x + r.size.x - cs, r.position.y + r.size.y - cs},
            {r.position.x + r.size.x, r.position.y + r.size.y - cs},
            {r.position.x + r.size.x, r.position.y + r.size.y},
            {r.position.x + r.size.x - cs, r.position.y + r.size.y},
            border_color_, alpha_);

    AddQuad(panel_vertices_,
            {r.position.x + cs, r.position.y},
            {r.position.x + r.size.x - cs, r.position.y},
            {r.position.x + r.size.x - cs, r.position.y + t},
            {r.position.x + cs, r.position.y + t},
            border_color_, alpha_);
    AddQuad(panel_vertices_,
            {r.position.x + cs, r.position.y + r.size.y - t},
            {r.position.x + r.size.x - cs, r.position.y + r.size.y - t},
            {r.position.x + r.size.x - cs, r.position.y + r.size.y},
            {r.position.x + cs, r.position.y + r.size.y},
            border_color_, alpha_);
    AddQuad(panel_vertices_,
            {r.position.x, r.position.y + cs},
            {r.position.x + t, r.position.y + cs},
            {r.position.x + t, r.position.y + r.size.y - cs},
            {r.position.x, r.position.y + r.size.y - cs},
            border_color_, alpha_);
    AddQuad(panel_vertices_,
            {r.position.x + r.size.x - t, r.position.y + cs},
            {r.position.x + r.size.x, r.position.y + cs},
            {r.position.x + r.size.x, r.position.y + r.size.y - cs},
            {r.position.x + r.size.x - t, r.position.y + r.size.y - cs},
            border_color_, alpha_);
}

void PixelDialogueBox::StartTyping_() {
    state_ = DialogueBoxState::Typing;
    typing_char_index_ = 0;
    typing_timer_ms_ = 0.0f;
    current_text_.clear();
}

void PixelDialogueBox::SkipTyping_() {
    typing_char_index_ = full_text_.size();
    current_text_ = full_text_;
    state_ = DialogueBoxState::WaitingChoice;
    if (choices_.empty()) {
        state_ = DialogueBoxState::WaitingConfirm;
    }
}

void PixelDialogueBox::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!visible_ || alpha_ <= 0.0f) return;
    target.draw(panel_vertices_, states);
}

// ============================================================================
// 【PixelDialogueBox::SyncFromDialogueEngine】
// ============================================================================
void PixelDialogueBox::SyncFromDialogueEngine(const DialogueEngine& engine,
                                               const std::string& speaker_name,
                                               const sf::Texture* avatar) {
    if (!engine.IsActive()) {
        if (state_ != DialogueBoxState::Hidden) {
            state_ = DialogueBoxState::Hidden;
            visible_ = false;
            choices_.clear();
            full_text_.clear();
            current_text_.clear();
            typing_char_index_ = 0;
        }
        return;
    }

    // 获取引擎当前状态
    const std::string full = engine.FullText();
    const std::string partial = engine.TypingText();
    const bool is_typing = engine.State() == DialogueState::Typing;

    // 如果对话框未激活，先启动
    if (state_ == DialogueBoxState::Hidden) {
        visible_ = true;
        state_ = DialogueBoxState::Typing;
        speaker_name_ = speaker_name;
        avatar_texture_ = avatar;
        full_text_ = full;
        current_text_ = partial;
        typing_char_index_ = partial.size();
        choices_ = engine.CurrentChoices();
        geometry_dirty_ = true;
        return;
    }

    // 如果文本内容发生变化（进入新节点），重置打字机
    if (full_text_ != full) {
        full_text_ = full;
        typing_char_index_ = 0;
        current_text_.clear();
        state_ = DialogueBoxState::Typing;
        choices_ = engine.CurrentChoices();
        geometry_dirty_ = true;
        return;
    }

    // 同步打字机进度
    if (is_typing && !partial.empty()) {
        if (current_text_ != partial) {
            current_text_ = partial;
            typing_char_index_ = partial.size();
            geometry_dirty_ = true;
        }
    } else if (!is_typing) {
        // 打字完成
        if (current_text_ != full_text_) {
            current_text_ = full_text_;
            typing_char_index_ = full_text_.size();
            geometry_dirty_ = true;
        }
        if (state_ == DialogueBoxState::Typing) {
            state_ = choices_.empty()
                ? DialogueBoxState::WaitingConfirm
                : DialogueBoxState::WaitingChoice;
        }
    }
}

}  // namespace CloudSeamanor::engine
