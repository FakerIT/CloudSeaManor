#pragma once

#include "CloudSeamanor/engine/ImageGenerationManager.hpp"
#include "CloudSeamanor/engine/PixelImageGeneratorPanel.hpp"
#include "CloudSeamanor/infrastructure/GameConfig.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>

namespace CloudSeamanor::integration {

class GameImageGenerationIntegration {
public:
    static void InitializeImageGeneration(const std::string& config_path = "configs/nvidia_image_generation.cfg") {
        infrastructure::GameConfig config;
        if (config.LoadFromFile(config_path)) {
            std::string api_key = config.GetString("nvidia_api_key", "");
            if (!api_key.empty() && api_key != "nvapi-xxxxxxxxxxxx") {
                engine::ImageGenerationManager::Instance().Initialize(api_key);
                Logger::Info("ImageGenerationManager initialized from config file");
            } else {
                Logger::Warning("Invalid or missing NVIDIA API key in config file");
            }
        } else {
            Logger::Warning("Failed to load image generation config file: " + config_path);
        }
    }

    static void ShutdownImageGeneration() {
        engine::ImageGenerationManager::Instance().Shutdown();
    }

    static void Example_IntegrateWithGameApp() {
        std::cout << "=== Image Generation Integration Example ===" << std::endl;

        std::string api_key = "nvapi-xxxxxxxxxxxx";
        engine::ImageGenerationManager::Instance().Initialize(api_key);

        std::string request_id = engine::ImageGenerationManager::Instance().SubmitCharacterPortraitRequest(
            "阿茶",
            "武侠仙侠风格",
            [](const engine::GeneratedImage& image) {
                if (image.status == engine::ImageGenerationStatus::Success) {
                    std::cout << "Character portrait generated successfully!" << std::endl;
                    std::cout << "Image ID: " << image.id << std::endl;
                    std::cout << "Data size: " << image.image_data.size() << " bytes" << std::endl;
                } else {
                    std::cerr << "Failed to generate portrait: " << image.error_message << std::endl;
                }
            }
        );

        std::cout << "Submitted request: " << request_id << std::endl;

        int wait_count = 0;
        while (engine::ImageGenerationManager::Instance().GetStatus(request_id) == 
               engine::ImageGenerationStatus::Generating) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            wait_count++;
            if (wait_count > 120) {
                std::cout << "Timeout waiting for generation" << std::endl;
                break;
            }
        }

        auto texture = engine::ImageGenerationManager::Instance().GetTexture(request_id);
        if (texture) {
            std::cout << "Texture loaded successfully, size: " 
                      << texture->getSize().x << "x" << texture->getSize().y << std::endl;
        }

        engine::ImageGenerationManager::Instance().SaveImageToFile(request_id, "output_portrait.png");
        std::cout << "Image saved to output_portrait.png" << std::endl;

        engine::ImageGenerationManager::Instance().Shutdown();
    }

    static void Example_BatchSceneGeneration() {
        std::cout << "=== Batch Scene Generation Example ===" << std::endl;

        std::string api_key = "nvapi-xxxxxxxxxxxx";
        engine::ImageGenerationManager::Instance().Initialize(api_key);

        std::vector<std::pair<std::string, std::string>> scenes = {
            {"主屋", "主屋大厅场景"},
            {"茶园", "茶园场景"},
            {"温泉", "温泉场景"},
            {"灵兽园", "灵兽园场景"},
            {"观云台", "观云台日落场景"}
        };

        std::vector<std::string> request_ids;
        for (const auto& [name, template_id] : scenes) {
            std::string request_id = engine::ImageGenerationManager::Instance().SubmitGameSceneRequest(
                name, template_id
            );
            request_ids.push_back(request_id);
            std::cout << "Submitted scene request: " << name << " (ID: " << request_id << ")" << std::endl;
        }

        std::cout << "Waiting for all scenes to generate..." << std::endl;

        int completed = 0;
        while (completed < static_cast<int>(request_ids.size())) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            engine::ImageGenerationManager::Instance().ProcessAsyncResults();
            
            for (const auto& id : request_ids) {
                auto status = engine::ImageGenerationManager::Instance().GetStatus(id);
                if (status == engine::ImageGenerationStatus::Success) {
                    auto image = engine::ImageGenerationManager::Instance().GetGeneratedImage(id);
                    if (image && image->category == "game_scene") {
                        std::string filename = "scene_" + id + ".png";
                        engine::ImageGenerationManager::Instance().SaveImageToFile(id, filename);
                        completed++;
                        std::cout << "Scene completed: " << id << " saved to " << filename << std::endl;
                    }
                }
            }
        }

        std::cout << "All scenes generated successfully!" << std::endl;
        engine::ImageGenerationManager::Instance().Shutdown();
    }

    static void Example_UIPanelIntegration() {
        std::cout << "=== UI Panel Integration Example ===" << std::endl;

        sf::RenderWindow window(sf::VideoMode(1200, 800), "Image Generation Panel Demo");
        window.setFramerateLimit(60);

        std::string api_key = "nvapi-xxxxxxxxxxxx";
        engine::ImageGenerationManager::Instance().Initialize(api_key);

        engine::PixelImageGeneratorPanel panel(sf::FloatRect({100, 50}, {1000, 700}));
        panel.SetApiKey(api_key);
        panel.SetVisible(true);
        panel.FadeIn();

        panel.SetOnImageGenerated([](const std::string& image_id) {
            std::cout << "Image generated: " << image_id << std::endl;
        });

        sf::Clock clock;
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
                
                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::G && !panel.IsGenerating()) {
                        panel.StartGeneration(panel.GetSelectedOption());
                        std::cout << "Started generation for: " << panel.GetSelectedOption() << std::endl;
                    }
                    
                    if (event.key.code == sf::Keyboard::C && panel.IsGenerating()) {
                        panel.CancelCurrentGeneration();
                        std::cout << "Cancelled generation" << std::endl;
                    }
                    
                    if (event.key.code == sf::Keyboard::S && !panel.IsGenerating()) {
                        panel.SaveCurrentImage("generated_image.png");
                        std::cout << "Saved current image" << std::endl;
                    }
                }
            }

            float delta = clock.restart().asSeconds();
            panel.Update(delta);

            window.clear(sf::Color(50, 50, 50));
            window.draw(panel);
            window.display();
        }

        engine::ImageGenerationManager::Instance().Shutdown();
    }

    static void Example_ResourceIntegration() {
        std::cout << "=== Resource Manager Integration Example ===" << std::endl;

        std::string api_key = "nvapi-xxxxxxxxxxxx";
        engine::ImageGenerationManager::Instance().Initialize(api_key);

        std::string portrait_id = engine::ImageGenerationManager::Instance().SubmitCharacterPortraitRequest(
            "小满", "活泼可爱风格"
        );

        std::cout << "Generating character portrait..." << std::endl;
        while (engine::ImageGenerationManager::Instance().GetStatus(portrait_id) == 
               engine::ImageGenerationStatus::Generating) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        auto texture = engine::ImageGenerationManager::Instance().GetTexture(portrait_id);
        if (texture) {
            std::cout << "Texture ready for use in game" << std::endl;
            std::cout << "Texture size: " << texture->getSize().x << "x" << texture->getSize().y << std::endl;

            sf::Sprite character_sprite(*texture);
            character_sprite.setPosition({100, 100});
            character_sprite.setScale({0.5f, 0.5f});

            std::cout << "Character sprite created and ready for rendering" << std::endl;
        }

        engine::ImageGenerationManager::Instance().Shutdown();
    }

    static void Example_PromptTemplateUsage() {
        std::cout << "=== Prompt Template Usage Example ===" << std::endl;

        const auto& portrait_templates = prompt_templates::PromptTemplateLibrary::GetCharacterPortraitTemplates();
        std::cout << "Available character portrait templates:" << std::endl;
        for (const auto& tmpl : portrait_templates) {
            std::cout << "  - " << tmpl.category << ": " << tmpl.description_zh << std::endl;
        }

        const auto& scene_templates = prompt_templates::PromptTemplateLibrary::GetGameSceneTemplates();
        std::cout << "\nAvailable game scene templates:" << std::endl;
        for (const auto& tmpl : scene_templates) {
            std::cout << "  - " << tmpl.category << ": " << tmpl.description_zh << std::endl;
        }

        std::string custom_prompt = prompt_templates::PromptTemplateLibrary::BuildCustomCharacterPrompt(
            "female", "young adult", "long black hair with silver ornaments",
            "elegant hanfu with cloud patterns", "standing gracefully",
            "gentle smile", "jade pendant", "beautiful anime style"
        );
        std::cout << "\nCustom generated prompt:\n" << custom_prompt << std::endl;
    }
};

} // namespace CloudSeamanor::integration

int main() {
    std::cout << "CloudSeaManor - Image Generation Integration Examples" << std::endl;
    std::cout << "======================================================" << std::endl;

    CloudSeamanor::integration::GameImageGenerationIntegration::Example_IntegrateWithGameApp();

    CloudSeamanor::integration::GameImageGenerationIntegration::Example_PromptTemplateUsage();

    std::cout << "\nAll examples completed!" << std::endl;
    return 0;
}