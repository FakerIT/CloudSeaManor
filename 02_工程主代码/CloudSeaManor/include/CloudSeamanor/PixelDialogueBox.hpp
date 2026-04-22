#pragma once

#include "CloudSeamanor/DialogueEngine.hpp"
#include "CloudSeamanor/PixelArtStyle.hpp"
#include "CloudSeamanor/PixelUiConfig.hpp"
#include "CloudSeamanor/PixelUiPanel.hpp"

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>

#include <memory>
#include <string>
#include <vector>

namespace CloudSeamanor::engine {

enum class DialogueBoxState : std::uint8_t {
    Hidden,
    Typing,
    WaitingChoice,
    WaitingConfirm
};

class PixelDialogueBox : public sf::Drawable {
public:
    PixelDialogueBox();
    void SetRect(const sf::FloatRect& rect) { rect_ = rect; geometry_dirty_ = true; }
    void SetColors(const sf::Color& fill, const sf::Color& border) { panel_fill_color_ = fill; border_color_ = border; geometry_dirty_ = true; }

    void SetAvatar(const sf::Texture* texture);
    void SetSpeakerName(const std::string& name);
    void SetText(const std::string& full_text);
    void SetChoices(const std::vector<DialogueChoice>& choices);
    void Show();
    void Hide();

    [[nodiscard]] bool IsVisible() const { return visible_; }
    [[nodiscard]] bool IsActive() const {
        return state_ != DialogueBoxState::Hidden;
    }
    [[nodiscard]] bool HasChoices() const {
        return state_ == DialogueBoxState::WaitingChoice && !choices_.empty();
    }
    [[nodiscard]] const std::vector<DialogueChoice>& GetChoices() const { return choices_; }

    /**
     * @brief 从 DialogueEngine 同步状态（引擎驱动 HUD 渲染）
     */
    void SyncFromDialogueEngine(const DialogueEngine& engine, const std::string& speaker_name,
                                const sf::Texture* avatar);

    bool OnConfirm();
    bool SelectChoice(std::size_t index);
    void HoverChoice(int index);
    void MoveChoiceHover(int delta);
    bool ConfirmHoveredChoice();
    std::optional<std::size_t> OnMouseClick(float mx, float my);

    void Update(float delta_seconds);
    void SetTypingSpeed(float ms_per_char) { typing_speed_ms_ = ms_per_char; }

    void Render(sf::RenderWindow& window);

    using OnCompleteCallback = std::function<void()>;
    using OnChoiceCallback = std::function<void(const std::string& choice_id)>;

    void SetOnComplete(OnCompleteCallback cb) { on_complete_ = std::move(cb); }
    void SetOnChoice(OnChoiceCallback cb) { on_choice_ = std::move(cb); }

    [[nodiscard]] const sf::FloatRect& GetRect() const { return rect_; }
    [[nodiscard]] float TypingProgress() const {
        return full_text_.empty() ? 1.0f
            : static_cast<float>(typing_char_index_) / static_cast<float>(full_text_.size());
    }
    [[nodiscard]] std::size_t GetHoveredChoice() const { return hovered_choice_; }
    [[nodiscard]] std::string GetCurrentDisplayedText() const { return current_text_; }
    [[nodiscard]] std::string GetSpeakerName() const { return speaker_name_; }
    [[nodiscard]] DialogueBoxState GetState() const { return state_; }

protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    void RebuildGeometry_();
    void StartTyping_();
    void SkipTyping_();

    sf::FloatRect rect_;
    bool visible_ = false;

    const sf::Texture* avatar_texture_ = nullptr;
    std::string speaker_name_;
    std::string full_text_;
    std::string current_text_;
    std::vector<DialogueChoice> choices_;

    DialogueBoxState state_ = DialogueBoxState::Hidden;
    std::size_t typing_char_index_ = 0;
    float typing_timer_ms_ = 0.0f;
    float typing_speed_ms_ = DialogueBoxConfig::TypingSpeedMs;

    float fade_timer_ = 0.0f;
    float alpha_ = 1.0f;

    std::size_t hovered_choice_ = static_cast<std::size_t>(-1);
    std::size_t selected_choice_ = static_cast<std::size_t>(-1);

    OnCompleteCallback on_complete_;
    OnChoiceCallback on_choice_;

    mutable sf::VertexArray panel_vertices_;
    mutable sf::VertexArray avatar_vertices_;
    mutable bool geometry_dirty_ = true;

    sf::Color panel_fill_color_ = ColorPalette::Cream;
    sf::Color border_color_ = ColorPalette::BrownOutline;
};

}  // namespace CloudSeamanor::engine
