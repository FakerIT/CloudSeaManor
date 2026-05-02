#include "engine/PixelImageGeneratorPanel.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"
#include <SFML/Graphics/Font.hpp>
#include <sstream>
#include <iomanip>

namespace CloudSeamanor::engine {

PixelImageGeneratorPanel::PixelImageGeneratorPanel()
    : PixelUiPanel() {
    InitializeDefaultOptions_();
}

PixelImageGeneratorPanel::PixelImageGeneratorPanel(const sf::FloatRect& rect)
    : PixelUiPanel(rect, "AI Image Generator", true) {
    InitializeDefaultOptions_();
    LayoutComponents_();
}

void PixelImageGeneratorPanel::InitializeDefaultOptions_() {
    available_options_ = {
        {"portrait_wuxia", "武侠仙侠风格女性", "生成武侠仙侠风格的女性角色立绘", 
         GenerationType::CharacterPortrait, "武侠仙侠风格"},
        {"portrait_lively", "活泼可爱风格", "生成活泼可爱风格的角色立绘", 
         GenerationType::CharacterPortrait, "活泼可爱风格"},
        {"portrait_cold", "冷艳高傲风格", "生成冷艳高傲风格的女性角色立绘", 
         GenerationType::CharacterPortrait, "冷艳高傲风格"},
        {"portrait_male", "男性剑客风格", "生成男性剑客角色立绘", 
         GenerationType::CharacterPortrait, "男性剑客风格"},
        {"portrait_child", "儿童角色风格", "生成可爱儿童角色立绘", 
         GenerationType::CharacterPortrait, "儿童角色风格"},
        
        {"scene_teagarden", "茶园场景", "生成云海茶园游戏场景", 
         GenerationType::GameScene, "茶园场景"},
        {"scene_mainhouse", "主屋大厅场景", "生成云海山庄主屋大厅场景", 
         GenerationType::GameScene, "主屋大厅场景"},
        {"scene_hotspring", "温泉场景", "生成仙气缭绕的温泉场景", 
         GenerationType::GameScene, "温泉场景"},
        {"scene_spiritbeast", "灵兽园场景", "生成灵兽园场景", 
         GenerationType::GameScene, "灵兽园场景"},
        {"scene_observation", "观云台日落场景", "生成观云台日落场景", 
         GenerationType::GameScene, "观云台日落场景"},
        {"scene_workshop", "工坊场景", "生成工坊场景", 
         GenerationType::GameScene, "工坊场景"},
        {"scene_inn", "客栈场景", "生成客栈场景", 
         GenerationType::GameScene, "客栈场景"},
        
        {"item_tea", "灵茶图标", "生成灵茶物品图标", 
         GenerationType::ItemIcon, "灵茶图标"},
        {"item_equipment", "装备图标", "生成装备图标", 
         GenerationType::ItemIcon, "装备图标"},
        {"item_medicine", "药材图标", "生成药材物品图标", 
         GenerationType::ItemIcon, "药材图标"},
        
        {"custom", "自定义生成", "使用自定义prompt生成图像", 
         GenerationType::Custom, ""}
    };

    if (!available_options_.empty()) {
        selected_option_id_ = available_options_[0].id;
    }
}

void PixelImageGeneratorPanel::SetApiKey(const std::string& api_key) {
    api_key_ = api_key;
    if (!api_key_.empty()) {
        ImageGenerationManager::Instance().Initialize(api_key_);
        Logger::Info("ImageGenerationManager initialized with API key");
    }
}

void PixelImageGeneratorPanel::StartGeneration(const std::string& option_id, const std::string& custom_prompt) {
    if (is_generating_) {
        Logger::Warning("Already generating an image, please wait");
        return;
    }

    if (api_key_.empty()) {
        Logger::Error("API key not set, cannot generate image");
        return;
    }

    GenerationOption selected_option;
    bool found = false;
    for (const auto& option : available_options_) {
        if (option.id == option_id) {
            selected_option = option;
            found = true;
            break;
        }
    }

    if (!found) {
        Logger::Error("Invalid option ID: " + option_id);
        return;
    }

    is_generating_ = true;
    generation_progress_ = 0.0f;
    generation_start_time_ = std::chrono::system_clock::now();

    auto callback = [this](const GeneratedImage& image) {
        is_generating_ = false;
        if (image.status == ImageGenerationStatus::Success) {
            preview_image_id_ = image.id;
            if (on_image_generated_) {
                on_image_generated_(image.id);
            }
            Logger::Info("Image generation completed successfully");
        } else {
            Logger::Error("Image generation failed: " + image.error_message);
        }
    };

    switch (selected_option.type) {
        case GenerationType::CharacterPortrait:
            current_request_id_ = ImageGenerationManager::Instance().SubmitCharacterPortraitRequest(
                selected_option.display_name, selected_option.template_id, callback);
            break;
        case GenerationType::GameScene:
            current_request_id_ = ImageGenerationManager::Instance().SubmitGameSceneRequest(
                selected_option.display_name, selected_option.template_id, callback);
            break;
        case GenerationType::ItemIcon: {
            ImageGenerationRequest request;
            request.id = "item_" + selected_option.id;
            request.category = "item_icon";
            request.prompt = custom_prompt.empty() ? 
                "beautiful game item icon, " + selected_option.display_name + 
                ", glowing with magical energy, clean white background, high quality icon design" 
                : custom_prompt;
            request.negative_prompt = "low quality, blurry, watermark, text, logo, signature";
            request.width = 512;
            request.height = 512;
            request.aspect_ratio = "1:1";
            request.callback = callback;
            current_request_id_ = ImageGenerationManager::Instance().SubmitRequest(request);
            break;
        }
        case GenerationType::Custom: {
            if (custom_prompt.empty()) {
                Logger::Error("Custom prompt is required for custom generation");
                is_generating_ = false;
                return;
            }
            ImageGenerationRequest request;
            request.id = "custom_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
            request.category = "custom";
            request.prompt = custom_prompt;
            request.negative_prompt = "low quality, blurry, bad anatomy, deformed, ugly";
            request.width = 1024;
            request.height = 1024;
            request.aspect_ratio = "1:1";
            request.callback = callback;
            current_request_id_ = ImageGenerationManager::Instance().SubmitRequest(request);
            break;
        }
    }

    Logger::Info("Started image generation: " + current_request_id_);
}

void PixelImageGeneratorPanel::CancelCurrentGeneration() {
    if (!is_generating_ || current_request_id_.empty()) {
        return;
    }

    ImageGenerationManager::Instance().CancelRequest(current_request_id_);
    is_generating_ = false;
    current_request_id_.clear();
    generation_progress_ = 0.0f;
    Logger::Info("Image generation cancelled");
}

void PixelImageGeneratorPanel::Update(float delta_seconds) {
    PixelUiPanel::Update(delta_seconds);
    elapsed_time_ += delta_seconds;

    if (is_generating_) {
        UpdateGenerationStatus_();
    }

    ImageGenerationManager::Instance().ProcessAsyncResults();

    if (!preview_image_id_.empty() && !preview_texture_) {
        preview_texture_ = ImageGenerationManager::Instance().GetTexture(preview_image_id_);
        if (preview_texture_) {
            preview_sprite_.setTexture(*preview_texture_);
            LayoutComponents_();
        }
    }
}

void PixelImageGeneratorPanel::UpdateGenerationStatus_() {
    if (!current_request_id_.empty()) {
        auto status = ImageGenerationManager::Instance().GetStatus(current_request_id_);
        if (status == ImageGenerationStatus::Success || status == ImageGenerationStatus::Failed) {
            is_generating_ = false;
        } else {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now() - generation_start_time_).count();
            generation_progress_ = std::min(0.95f, elapsed / 60.0f);
        }
    }
}

std::string PixelImageGeneratorPanel::GetCurrentStatus() const {
    if (!is_generating_) {
        return "Ready";
    }

    std::ostringstream oss;
    oss << "Generating... " << std::fixed << std::setprecision(1) 
        << (generation_progress_ * 100.0f) << "%";
    return oss.str();
}

void PixelImageGeneratorPanel::SetPreviewImage(const std::string& image_id) {
    preview_image_id_ = image_id;
    preview_texture_ = ImageGenerationManager::Instance().GetTexture(image_id);
    if (preview_texture_) {
        preview_sprite_.setTexture(*preview_texture_);
        LayoutComponents_();
    }
}

void PixelImageGeneratorPanel::ClearPreview() {
    preview_image_id_.clear();
    preview_texture_.reset();
}

void PixelImageGeneratorPanel::AddGenerationOption(const GenerationOption& option) {
    available_options_.push_back(option);
}

void PixelImageGeneratorPanel::SaveCurrentImage(const std::string& file_path) {
    if (!preview_image_id_.empty()) {
        ImageGenerationManager::Instance().SaveImageToFile(preview_image_id_, file_path);
    }
}

void PixelImageGeneratorPanel::LayoutComponents_() {
    auto rect = GetRect();
    if (rect.size.x <= 0 || rect.size.y <= 0) return;

    float preview_width = rect.size.x * 0.4f;
    float preview_height = rect.size.y * 0.6f;
    float preview_x = rect.position.x + 20;
    float preview_y = rect.position.y + 60;

    if (preview_texture_) {
        auto tex_size = preview_texture_->getSize();
        float scale_x = preview_width / static_cast<float>(tex_size.x);
        float scale_y = preview_height / static_cast<float>(tex_size.y);
        float scale = std::min(scale_x, scale_y);

        preview_sprite_.setScale({scale, scale});
        preview_sprite_.setPosition({preview_x, preview_y});
    }
}

void PixelImageGeneratorPanel::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!IsVisible()) return;

    PixelUiPanel::draw(target, states);

    DrawPreview_(target, states);
    DrawStatusText_(target, states);
    DrawOptionsList_(target, states);
}

void PixelImageGeneratorPanel::DrawPreview_(sf::RenderTarget& target, const sf::RenderStates& states) const {
    if (preview_texture_) {
        target.draw(preview_sprite_, states);
    }
}

void PixelImageGeneratorPanel::DrawStatusText_(sf::RenderTarget& target, const sf::RenderStates& states) const {
    status_text_.setString(GetCurrentStatus());
    status_text_.setCharacterSize(16);
    status_text_.setFillColor(is_generating_ ? sf::Color::Yellow : sf::Color::White);
    status_text_.setPosition({GetRect().position.x + 20, GetRect().position.y + GetRect().size.y - 40});
    target.draw(status_text_, states);
}

void PixelImageGeneratorPanel::DrawOptionsList_(sf::RenderTarget& target, const sf::RenderStates& states) const {
    auto rect = GetRect();
    float options_x = rect.position.x + rect.size.x * 0.45f;
    float options_y = rect.position.y + 60;
    float line_height = 25.0f;

    std::ostringstream oss;
    oss << "Available Options:\n\n";
    for (const auto& option : available_options_) {
        if (option.id == selected_option_id_) {
            oss << "[*] ";
        } else {
            oss << "[ ] ";
        }
        oss << option.display_name << "\n";
    }

    options_text_.setString(oss.str());
    options_text_.setCharacterSize(14);
    options_text_.setFillColor(sf::Color::White);
    options_text_.setPosition({options_x, options_y});
    target.draw(options_text_, states);
}

} // namespace CloudSeamanor::engine