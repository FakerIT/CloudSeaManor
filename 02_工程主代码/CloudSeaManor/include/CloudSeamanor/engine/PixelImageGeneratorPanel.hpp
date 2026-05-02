#pragma once

#include "CloudSeamanor/engine/PixelUiPanel.hpp"
#include "CloudSeamanor/engine/ImageGenerationManager.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <vector>
#include <functional>

namespace CloudSeamanor::engine {

class PixelImageGeneratorPanel : public PixelUiPanel {
public:
    enum class GenerationType {
        CharacterPortrait,
        GameScene,
        ItemIcon,
        Custom
    };

    struct GenerationOption {
        std::string id;
        std::string display_name;
        std::string description;
        GenerationType type;
        std::string template_id;
    };

    PixelImageGeneratorPanel();
    explicit PixelImageGeneratorPanel(const sf::FloatRect& rect);

    void Update(float delta_seconds) override;
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    void SetOnImageGenerated(std::function<void(const std::string&)> callback) {
        on_image_generated_ = std::move(callback);
    }

    void SetApiKey(const std::string& api_key);

    void StartGeneration(const std::string& option_id, const std::string& custom_prompt = "");
    void CancelCurrentGeneration();

    bool IsGenerating() const { return is_generating_; }
    float GetProgress() const { return generation_progress_; }
    std::string GetCurrentStatus() const;

    void SetPreviewImage(const std::string& image_id);
    void ClearPreview();

    const std::vector<GenerationOption>& GetAvailableOptions() const { return available_options_; }
    void AddGenerationOption(const GenerationOption& option);

    void SetCustomPrompt(const std::string& prompt) { custom_prompt_ = prompt; }
    const std::string& GetCustomPrompt() const { return custom_prompt_; }

    void SetSelectedOption(const std::string& option_id) { selected_option_id_ = option_id; }
    const std::string& GetSelectedOption() const { return selected_option_id_; }

    void SaveCurrentImage(const std::string& file_path);

private:
    void InitializeDefaultOptions_();
    void UpdateGenerationStatus_();
    void LayoutComponents_();
    void DrawPreview_(sf::RenderTarget& target, const sf::RenderStates& states) const;
    void DrawStatusText_(sf::RenderTarget& target, const sf::RenderStates& states) const;
    void DrawOptionsList_(sf::RenderTarget& target, const sf::RenderStates& states) const;

    std::string api_key_;
    std::vector<GenerationOption> available_options_;
    std::string selected_option_id_;
    std::string custom_prompt_;

    bool is_generating_ = false;
    float generation_progress_ = 0.0f;
    std::string current_request_id_;
    std::chrono::system_clock::time_point generation_start_time_;

    std::string preview_image_id_;
    std::shared_ptr<sf::Texture> preview_texture_;
    sf::Sprite preview_sprite_;

    std::function<void(const std::string&)> on_image_generated_;

    mutable sf::Text status_text_;
    mutable sf::Text options_text_;
    mutable sf::Text preview_label_;

    float elapsed_time_ = 0.0f;
};

} // namespace CloudSeamanor::engine